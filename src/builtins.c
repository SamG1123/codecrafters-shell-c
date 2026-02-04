#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "builtins.h"
#include "shell.h"

const char *builtin_commands[] = {"exit", "echo", "type", "pwd"};

int is_builtin(const char *command) {
  for (int i = 0; i < NUM_BUILTINS; i++) {
    if (strcmp(command, builtin_commands[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

void handle_echo(const char *command) {
  printf("%s\n", command + 5);
}

void handle_type(const char *command, const char *path_env) {
  char *arg = (char *)(command + 5);
  
  if (is_builtin(arg)) {
    printf("%s is a shell builtin\n", arg);
  } else {
    if (path_env != NULL) {
      char *path_copy = strdup(path_env);
      char *dir = strtok(path_copy, PATH_LIST_SEPARATOR_STR);
      bool found = false;
      
      while (dir != NULL && !found) {
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, arg);
        
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
