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
  
  #ifdef _WIN32
  home_env = getenv("USERPROFILE");
  #else
  home_env = getenv("HOME");
  #endif

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
    int arg_count = 0;
    char **tokens = arg_processor(input, &arg_count);

    if (tokens == NULL || arg_count == 0) {
      continue;
    }

    if (strcmp(tokens[0], "exit") == 0) {
      break;
    } else if (strcmp(tokens[0], "echo") == 0) {
      handle_echo(tokens, arg_count);
    } else if (strcmp(tokens[0], "type") == 0) {
      handle_type(tokens, arg_count, path_env);
    } else if (strcmp(tokens[0], "pwd") == 0) {
      handle_pwd();
    } else if (strcmp(tokens[0], "cd") == 0) {
      handle_cd(arg_count > 1 ? tokens[1] : "~", home_env);
    } else if (strcmp(tokens[0], "cat") == 0) {
      handle_cat(tokens, arg_count);
    } else if (find_file(tokens[0], path_env)) {
      execute_command(command);
    } else {
      printf("%s: command not found\n", tokens[0]);
    }
    
    for (int i = 0; i < arg_count; i++) {
      free(tokens[i]);
    }
    free(tokens);
  }

  return 0;
}
