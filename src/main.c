#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"
#include "builtins.h"
#include "executor.h"

int main(int argc, char *argv[]) {
  char command[MAX_COMMAND_LEN];
  char input[MAX_COMMAND_LEN];
  char *path_env = getenv("PATH");
  const char *home_env;
  home_env = getenv("HOME");

  // Flush after every printf
  setbuf(stdout, NULL);

  while (1) {
    printf("$ ");
    
    if (fgets(command, sizeof(command), stdin) == NULL) {
      break;
    }
    
    command[strcspn(command, "\n")] = 0;
    
    if (strlen(command) == 0) {
      continue;
    }

    strcpy(input, command);
    char *token = strtok(input, " ");

    if (token == NULL) {
      continue;
    }

    if (strcmp(token, "exit") == 0) {
      break;
    } else if (strcmp(token, "echo") == 0) {
      handle_echo(command);
    } else if (strcmp(token, "type") == 0) {
      handle_type(command, path_env);
    } else if (strcmp(token, "pwd") == 0) {
      handle_pwd();
    } else if (find_file(token, path_env)) {
      execute_command(command);
    } else if (strcmp(token, "cd") == 0) {
      handle_cd(command + 3, home_env);
    }
     else {
      printf("%s: command not found\n", token);
    }
  }

  return 0;
}
