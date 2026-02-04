#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <process.h>
  #define access _access
  #define X_OK 0
  typedef int pid_t;
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>
#endif
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
    snprintf(full_path, sizeof(full_path), "%s%c%s", dir, PATH_SEPARATOR, command);
    if (access(full_path, X_OK) == 0) {
      found = 1;
    }
    dir = strtok(NULL, PATH_LIST_SEPARATOR_STR);
  }
  
  free(path_copy);
  return found;
}

void execute_command(char **tokens, int token_count) {
  if (tokens == NULL || token_count == 0) {
    return;
  }
  
  char **args = malloc(sizeof(char*) * (token_count + 1));
  for (int i = 0; i < token_count; i++) {
    args[i] = tokens[i];
  }
  args[token_count] = NULL;
  
#ifdef _WIN32
  int result = _spawnvp(_P_WAIT, args[0], (const char* const*)args);
  free(args);
#else
  pid_t pid = fork();
  if (pid == 0) {

    execvp(args[0], args);
    exit(1);
  } else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    free(args);
  } else {
    perror("fork");
    free(args);
  }
#endif
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
                    // Look ahead to next character - if it's a quote, skip both
                    prev_state = state;
                    state = STATE_IN_ESCAPE;
                } else {
                    current_arg[arg_pos++] = c;
                }
                break;

            case STATE_IN_ESCAPE:
                // Skip the backslash, but only add char if it's not a quote or space
                if (c != '\'' && c != '\"' && c != ' ' && c != '\t') {
                    current_arg[arg_pos++] = c;
                }
                // If it's a quote or space, we skip it entirely (removing \' or \")
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
