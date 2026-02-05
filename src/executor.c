#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <process.h>
  #define access _access
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
  char *file_mode;
  if (tokens == NULL || token_count == 0) {
    return;
  }
  
  int redirect_index = -1;
  char *output_file = NULL;
  char *error_file = NULL;
  
  if (STDOUT_REDIRECT) {
    for (int i = 0; i < token_count; i++) {
      if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0) {
        redirect_index = i;
        if (i < token_count - 1) {
          output_file = tokens[i + 1];
        }
        file_mode = "w";
        break;
      } else if (strcmp(tokens[i], ">>") == 0 || strcmp(tokens[i], "1>>") == 0) {
        redirect_index = i;
        if (i < token_count - 1) {
          output_file = tokens[i + 1];
        }
        file_mode = "a";
        break;
      }
    }
    
    if (redirect_index == -1 || output_file == NULL) {
      printf("Syntax error: no file specified for redirection\n");
      return;
    }
  }
  
  if (STDERR_REDIRECT) {
    for (int i = 0; i < token_count; i++) {
      if (strcmp(tokens[i], "2>") == 0) {
        redirect_index = i;
        if (i < token_count - 1) {
          error_file = tokens[i + 1];
        }
        break;
      }
    }
    
    if (redirect_index == -1 || error_file == NULL) {
      printf("Syntax error: no file specified for error redirection\n");
      return;
    }
  }
  
  int arg_count = redirect_index == -1 ? token_count : redirect_index;
  char **args = malloc(sizeof(char*) * (arg_count + 1));
  for (int i = 0; i < arg_count; i++) {
    args[i] = tokens[i];
  }
  args[arg_count] = NULL;
  
#ifdef _WIN32
  int saved_stdout = -1;
  int saved_stderr = -1;
  FILE *stdout_file = NULL;
  FILE *stderr_file = NULL;
  
  // Redirect stdout
  if (STDOUT_REDIRECT && output_file != NULL) {
    saved_stdout = _dup(1);
    stdout_file = fopen(output_file, "w");
    if (stdout_file == NULL) {
      perror("fopen");
      free(args);
      return;
    }
    _dup2(_fileno(stdout_file), 1);
  }
  
  // Redirect stderr
  if (STDERR_REDIRECT && error_file != NULL) {
    saved_stderr = _dup(2);
    stderr_file = fopen(error_file, "w");
    if (stderr_file == NULL) {
      perror("fopen");
      free(args);
      return;
    }
    _dup2(_fileno(stderr_file), 2);
  }
  
  int result = _spawnvp(_P_WAIT, args[0], (const char* const*)args);
  
  // Restore stdout
  if (saved_stdout != -1) {
    _dup2(saved_stdout, 1);
    _close(saved_stdout);
    if (stdout_file != NULL) {
      fclose(stdout_file);
    }
  }
  
  // Restore stderr
  if (saved_stderr != -1) {
    _dup2(saved_stderr, 2);
    _close(saved_stderr);
    if (stderr_file != NULL) {
      fclose(stderr_file);
    }
  }
  
  free(args);
#else
  
  pid_t pid = fork();
  if (pid == 0) {
    // Redirect stdout in child
    if (STDOUT_REDIRECT && output_file != NULL) {
      FILE *file = fopen(output_file, file_mode);
      if (file == NULL) {
        perror("fopen");
        exit(1);
      }
      dup2(fileno(file), 1);
      fclose(file);
    }
    
    // Redirect stderr in child
    if (STDERR_REDIRECT && error_file != NULL) {
      FILE *file = fopen(error_file, "w");
      if (file == NULL) {
        perror("fopen");
        exit(1);
      }
      dup2(fileno(file), 2);
      fclose(file);
    }
    
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
                    prev_state = state;
                    state = STATE_IN_ESCAPE;
                } else {
                    current_arg[arg_pos++] = c;
                }
                break;

            case STATE_IN_ESCAPE:
                if (prev_state == STATE_IN_DOUBLE_QUOTE && c == '\'') {
                    current_arg[arg_pos++] = '\\';
                    current_arg[arg_pos++] = c;
                } else {
                    current_arg[arg_pos++] = c;
                }
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

