#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <readline/history.h>
#include "builtins.h"
#include "shell.h"

const char *builtin_commands[] = {"exit", "echo", "type", "pwd", "cd", "history"};

const char *buffer[MAX_HISTORY];

int is_builtin(const char *command) {
  for (int i = 0; i < NUM_BUILTINS; i++) {
    if (strcmp(command, builtin_commands[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

void handle_echo(char **tokens, int token_count) {
  for (int i = 1; i < token_count; i++) {
    if (i > 1) {
      printf(" ");
    }
    printf("%s", tokens[i]);
  }
  printf("\n");
}

void handle_type(char **tokens, int token_count, const char *path_env) {
  if (token_count < 2) {
    printf("type: missing argument\n");
    return;
  }
  char *arg = tokens[1];
  
  if (is_builtin(arg)) {
    printf("%s is a shell builtin\n", arg);
  } else {
    if (path_env != NULL) {
      char *path_copy = strdup(path_env);
      char *dir = strtok(path_copy, PATH_LIST_SEPARATOR_STR);
      bool found = false;
      
      while (dir != NULL && !found) {
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s%c%s", dir, PATH_SEPARATOR, arg);
        
        if (access(full_path, X_OK) == 0) {
          printf("%s is %s\n", arg, full_path);
          found = true;
        }
        dir = strtok(NULL, PATH_LIST_SEPARATOR_STR);
      }
      free(path_copy);
      
      if (!found) {
        printf("%s: not found\n", arg);
      }
    } else {
      printf("%s: not found\n", arg);
    }
  }
}

void handle_pwd(void) {
  char cwd[MAX_PATH_LEN];
  printf("%s\n", getcwd(cwd, sizeof(cwd)));
}

void handle_cd(const char *path, const char *HOME) {
    const char *target_path = path;

    if (path == NULL || strlen(path) == 0 || strcmp(path, "~") == 0) {
        target_path = HOME;
    }
    if (chdir(target_path) != 0) {
        printf("cd: %s: No such file or directory\n", target_path);
    }
}

void handle_history(int count) {
  FILE *history_file = fopen("history.txt", "r");
  if (history_file == NULL) {
    return;
  }
  
  char line[1024];
  int line_number = 1;
  
  while (fgets(line, sizeof(line), history_file) != NULL) {
    line[strcspn(line, "\n")] = 0;
    buffer[line_number - 1] = strdup(line);
    line_number++;
  }
  int len_buffer = sizeof(buffer);
  for (int i = len_buffer - count; i < len_buffer; i++) {
    if (i >= 0) {
      printf("%d %s\n", i + 1, buffer[i]);
    }
  }
  
  fclose(history_file);
}