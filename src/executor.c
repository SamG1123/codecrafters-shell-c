#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "executor.h"
#include "shell.h"

typedef enum {
    STATE_NORMAL,
    STATE_IN_SINGLE_QUOTE,
    STATE_IN_DOUBLE_QUOTE,
    STATE_IN_ESCAPE
} ArgState;

int find_file(char *command, char *path_env) {
  if (command == NULL || path_env == NULL) {
    return 0;
  }
  
  char *path_copy = strdup(path_env);
  char *dir = strtok(path_copy, PATH_LIST_SEPARATOR_STR);
  int found = 0;
  
  while (dir != NULL && !found) {
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
    if (access(full_path, X_OK) == 0) {
      found = 1;
    }
    dir = strtok(NULL, PATH_LIST_SEPARATOR_STR);
  }
  
  free(path_copy);
  return found;
}

void execute_command(char *command) {
  system(command);
}

char **arg_processor(char *arg, int *argc){
    ArgState state = STATE_NORMAL;
    ArgState prev_state = STATE_NORMAL;
    char **args = malloc(sizeof(char*) * MAX_COMMAND_LEN);
    char current_arg[MAX_COMMAND_LEN] = {0};
    int arg_pos = 0;
    *argc = 0;

    for (int i = 0; arg[i]; i++) {
        char c = arg[i];

        switch (state) {
            case STATE_NORMAL:
                if (c == ' ' || c == '\t') {
                    if (arg_pos > 0) {
                        current_arg[arg_pos] = '\0';
                        args[*argc] = strdup(current_arg);
                        (*argc)++;
                        arg_pos = 0;
                        memset(current_arg, 0, sizeof(current_arg));
                    }
                } else if (c == '\'') {
                    state = STATE_IN_SINGLE_QUOTE;
                } else if (c == '\"') {
                    state = STATE_IN_DOUBLE_QUOTE;
                } else if (c == '\\') {
                    prev_state = state;
                    state = STATE_IN_ESCAPE;
                } else {
                    current_arg[arg_pos++] = c;
                }
                break;

            case STATE_IN_SINGLE_QUOTE:
                if (c == '\'') {
                    state = STATE_NORMAL;
                } else {
                    current_arg[arg_pos++] = c;
                }
                break;

            case STATE_IN_DOUBLE_QUOTE:
                if (c == '\"') {
                    state = STATE_NORMAL;
                } else if (c == '\\') {
                    prev_state = state;
                    state = STATE_IN_ESCAPE;
                } else {
                    current_arg[arg_pos++] = c;
                }
                break;

            case STATE_IN_ESCAPE:
                current_arg[arg_pos++] = c;
                state = prev_state;
                break;
        }
    }
    if (arg_pos > 0) {
        current_arg[arg_pos] = '\0';
        args[*argc] = strdup(current_arg);
        (*argc)++;
    }
    
    return args;
}
