#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"
#include "builtins.h"
#include "executor.h"

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
#endif

int STDOUT_REDIRECT = 0;

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

    STDOUT_REDIRECT = 0;
    char *output_file = NULL;
    int redirect_index = -1;
    
    for (int i = 0; i < arg_count; i++) {
      if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0) {
        STDOUT_REDIRECT = 1;
        redirect_index = i;
        if (i < arg_count - 1) {
          output_file = tokens[i + 1];
        }
        break;
      }
    }

    int saved_stdout = -1;
    FILE *redirect_file = NULL;
    
    if (STDOUT_REDIRECT && output_file != NULL) {
#ifdef _WIN32
      saved_stdout = _dup(1);
      redirect_file = fopen(output_file, "w");
      if (redirect_file != NULL) {
        _dup2(_fileno(redirect_file), 1);
      }
#else
      saved_stdout = dup(1);
      redirect_file = fopen(output_file, "w");
      if (redirect_file != NULL) {
        dup2(fileno(redirect_file), 1);
      }
#endif
    }

    if (strcmp(tokens[0], "exit") == 0) {
      break;
    } else if (strcmp(tokens[0], "echo") == 0) {
      handle_echo(tokens, redirect_index == -1 ? arg_count : redirect_index);
    } else if (strcmp(tokens[0], "type") == 0) {
      handle_type(tokens, redirect_index == -1 ? arg_count : redirect_index, path_env);
    } else if (strcmp(tokens[0], "pwd") == 0) {
      handle_pwd();
    } else if (strcmp(tokens[0], "cd") == 0) {
      handle_cd(arg_count > 1 ? tokens[1] : "~", home_env);
    } else if (find_file(tokens[0], path_env)) {
      execute_command(tokens, arg_count);
    } else {
      printf("%s: command not found\n", tokens[0]);
    }
    
    if (saved_stdout != -1) {
#ifdef _WIN32
      fflush(stdout);
      _dup2(saved_stdout, 1);
      _close(saved_stdout);
#else
      fflush(stdout);
      dup2(saved_stdout, 1);
      close(saved_stdout);
#endif
      if (redirect_file != NULL) {
        fclose(redirect_file);
      }
    }
    
    for (int i = 0; i < arg_count; i++) {
      free(tokens[i]);
    }
    free(tokens);
  }

  return 0;
}
