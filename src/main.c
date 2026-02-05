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
int STDERR_REDIRECT = 0;

// Helper to redirect a file descriptor
int redirect_fd(int fd, const char *filename, const char *file_mode) {
  int saved = -1;
  FILE *file = fopen(filename, file_mode);
  if (file == NULL) {
    return -1;
  }
#ifdef _WIN32
  saved = _dup(fd);
  _dup2(_fileno(file), fd);
#else
  saved = dup(fd);
  dup2(fileno(file), fd);
#endif
  fclose(file);
  return saved;
}

// Helper to restore a file descriptor
void restore_fd(int fd, int saved) {
  if (saved == -1) return;
#ifdef _WIN32
  fflush(fd == 1 ? stdout : stderr);
  _dup2(saved, fd);
  _close(saved);
#else
  fflush(fd == 1 ? stdout : stderr);
  dup2(saved, fd);
  close(saved);
#endif
}

// Check if command is builtin
int is_builtin_cmd(const char *cmd) {
  return strcmp(cmd, "echo") == 0 || strcmp(cmd, "type") == 0 ||
         strcmp(cmd, "pwd") == 0 || strcmp(cmd, "cd") == 0;
}

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
    STDERR_REDIRECT = 0;
    char *output_file = NULL;
    char *error_file = NULL;
    int redirect_index = -1;
    char *file_mode = "w";
    
    // Parse redirection operators
    for (int i = 0; i < arg_count; i++) {
      if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0) {
        STDOUT_REDIRECT = 1;
        redirect_index = i;
        output_file = (i < arg_count - 1) ? tokens[i + 1] : NULL;
        file_mode = "w";
        break;
      } else if (strcmp(tokens[i], ">>") == 0 || strcmp(tokens[i], "1>>") == 0) {
        STDOUT_REDIRECT = 1;
        redirect_index = i;
        output_file = (i < arg_count - 1) ? tokens[i + 1] : NULL;
        file_mode = "a";
        break;
      }
      else if (strcmp(tokens[i], "2>") == 0) {
        STDERR_REDIRECT = 1;
        redirect_index = i;
        error_file = (i < arg_count - 1) ? tokens[i + 1] : NULL;
        break;
      } else if (strcmp(tokens[i], "2>>") == 0) {
        STDERR_REDIRECT = 1;
        redirect_index = i;
        error_file = (i < arg_count - 1) ? tokens[i + 1] : NULL;
        file_mode = "a";
        break;
      }
    }

    // Setup redirections for builtin commands
    int saved_stdout = -1, saved_stderr = -1;
    if (is_builtin_cmd(tokens[0])) {
      if (STDOUT_REDIRECT && output_file) {
        saved_stdout = redirect_fd(1, output_file, file_mode);
      }
      if (STDERR_REDIRECT && error_file) {
        saved_stderr = redirect_fd(2, error_file, file_mode);
      }
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
    
    restore_fd(1, saved_stdout);
    restore_fd(2, saved_stderr);
    
    for (int i = 0; i < arg_count; i++) {
      free(tokens[i]);
    }
    free(tokens);
  }

  return 0;
}
