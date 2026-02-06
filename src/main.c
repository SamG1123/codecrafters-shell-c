#include <stdio.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include "shell.h"
#include "builtins.h"
#include "executor.h"

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <process.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
#endif

int STDOUT_REDIRECT = 0;
int STDERR_REDIRECT = 0;
char **completion_list = NULL;
int completion_count = 0;
char *current_path_env = NULL;

static char history[MAX_HISTORY][MAX_COMMAND_LEN];
static int history_count = 0;

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

int is_builtin_cmd(const char *cmd) {
  return strcmp(cmd, "echo") == 0 || strcmp(cmd, "type") == 0 ||
         strcmp(cmd, "pwd") == 0 || strcmp(cmd, "cd") == 0 || 
          strcmp(cmd, "history") == 0;
}


void add_executables_from_dir(const char *dir, const char *prefix, int prefix_len) {
  DIR *dirp = opendir(dir);
  if (dirp == NULL) return;

  struct dirent *entry;
  while ((entry = readdir(dirp)) != NULL) {
    if (strncmp(entry->d_name, prefix, prefix_len) != 0) {
      continue;
    }


    int found = 0;
    for (int i = 0; i < completion_count; i++) {
      if (strcmp(completion_list[i], entry->d_name) == 0) {
        found = 1;
        break;
      }
    }
    if (found) continue;


    if (completion_count % 10 == 0 && completion_count > 0) {
      completion_list = realloc(completion_list, (completion_count + 10) * sizeof(char *));
    }

    completion_list[completion_count] = strdup(entry->d_name);
    if (completion_list[completion_count] != NULL) {
      completion_count++;
    }
  }
  closedir(dirp);
}


void build_completion_list(const char *text) {
  int text_len = strlen(text);

  // Free old list
  for (int i = 0; i < completion_count; i++) {
    free(completion_list[i]);
  }
  free(completion_list);
  completion_list = NULL;
  completion_count = 0;

  
  completion_list = malloc(10 * sizeof(char *));
  if (completion_list == NULL) return;

  // Add matching builtins
  for (int i = 0; i < NUM_BUILTINS; i++) {
    if (strncmp(builtin_commands[i], text, text_len) == 0) {
      completion_list[completion_count] = strdup(builtin_commands[i]);
      if (completion_list[completion_count] != NULL) {
        completion_count++;
      }
    }
  }

  // Add matching executables from PATH
  if (current_path_env != NULL) {
    char *path_copy = strdup(current_path_env);
    if (path_copy != NULL) {
      char *dir = strtok(path_copy, PATH_LIST_SEPARATOR_STR);
      while (dir != NULL) {
        add_executables_from_dir(dir, text, text_len);
        dir = strtok(NULL, PATH_LIST_SEPARATOR_STR);
      }
      free(path_copy);
    }
  }
}

char *autocomplete(const char *text, int state) {
  if (!state) {
    build_completion_list(text);
    return (completion_count > 0) ? strdup(completion_list[0]) : NULL;
  }

  if (state < completion_count) {
    return strdup(completion_list[state]);
  }

  return NULL;
}

char **autocomplete_setup(const char *text, int start, int end) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, autocomplete);
}



int main(int argc, char *argv[]) {
  char command[MAX_COMMAND_LEN];
  char input[MAX_COMMAND_LEN];
  char *path_env = getenv("PATH");
  current_path_env = path_env;
  const char *home_env;
  
  #ifdef _WIN32
  home_env = getenv("USERPROFILE");
  #else
  home_env = getenv("HOME");
  #endif

  setbuf(stdout, NULL);

  while (1) {
    rl_attempted_completion_function = autocomplete_setup;
    int pipe_index[MAX_PIPES];
    int pipe_count = 0;
    char *command = readline("$ ");
    
    if (command == NULL) {
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

    add_history(command);

    STDOUT_REDIRECT = 0;
    STDERR_REDIRECT = 0;
    char *output_file = NULL;
    char *error_file = NULL;
    int redirect_index = -1;
    char *file_mode = "w";
    
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
      } else if (strcmp(tokens[i], "|") == 0) {
        pipe_index[pipe_count++] = i;
      }
    }

    

    char ***commands = malloc((pipe_count + 1) * sizeof(char **));

    for (int i = 0; i <= pipe_count; i++) {
      int start = i == 0 ? 0 : pipe_index[i - 1] + 1;
      int end = i == pipe_count ? arg_count : pipe_index[i];
      int count = end - start;
      commands[i] = malloc((count + 1) * sizeof(char *));
      for (int j = 0; j < count; j++) {
        commands[i][j] = tokens[start + j];
      }
      commands[i][count] = NULL;
    }
    
    if (pipe_count > 0) {
      int pipefd[MAX_PIPES][2];
      pid_t children[MAX_COMMANDS];
      
      char ***commands = malloc((pipe_count + 1) * sizeof(char**));
      for (int j = 0; j <= pipe_count; j++) {
        int cmd_start = (j == 0) ? 0 : pipe_index[j-1] + 1;
        int cmd_end = (j == pipe_count) ? arg_count : pipe_index[j];
        int cmd_token_count = cmd_end - cmd_start;
        
        commands[j] = malloc((cmd_token_count + 1) * sizeof(char*));
        for (int i = 0; i < cmd_token_count; i++) {
          commands[j][i] = tokens[cmd_start + i];
        }
        commands[j][cmd_token_count] = NULL;
      }
      
      for (int j = 0; j < pipe_count; j++) {
        if (pipe(pipefd[j]) == -1) {
          perror("pipe");
          for (int x = 0; x <= pipe_count; x++) {
            free(commands[x]);
          }
          free(commands);
          for (int i = 0; i < arg_count; i++) {
            free(tokens[i]);
          }
          free(tokens);
          continue;
        }
      }
      
      for (int j = 0; j <= pipe_count; j++) {
        pid_t child = fork();
        
        if (child < 0) {
          perror("fork");
          for (int x = 0; x < pipe_count; x++) {
            close(pipefd[x][0]);
            close(pipefd[x][1]);
          }
          for (int x = 0; x <= pipe_count; x++) {
            free(commands[x]);
          }
          free(commands);
          for (int i = 0; i < arg_count; i++) {
            free(tokens[i]);
          }
          free(tokens);
          continue;
        }
        
        if (child == 0) {
          if (j > 0) {
            close(pipefd[j-1][1]);
            dup2(pipefd[j-1][0], 0);
            close(pipefd[j-1][0]);
          }
          
          if (j < pipe_count) {
            close(pipefd[j][0]);
            dup2(pipefd[j][1], 1);
            close(pipefd[j][1]);
          }
          
          for (int x = 0; x < pipe_count; x++) {
            if (x != j-1 && x != j) {
              close(pipefd[x][0]);
              close(pipefd[x][1]);
            }
          }
          
          int cmd_start = (j == 0) ? 0 : pipe_index[j-1] + 1;
          int cmd_end = (j == pipe_count) ? arg_count : pipe_index[j];
          int cmd_token_count = cmd_end - cmd_start;
          
          if (is_builtin_cmd(commands[j][0])) {
            if (strcmp(commands[j][0], "echo") == 0) {
              handle_echo(commands[j], cmd_token_count);
              handle_history(commands[j][1]);
            } else if (strcmp(commands[j][0], "type") == 0) {
              handle_type(commands[j], cmd_token_count, path_env);
              handle_history(commands[j][1]);
            } else if (strcmp(commands[j][0], "pwd") == 0) {
              handle_pwd();
              handle_history(commands[j][1]);
            } else if (strcmp(commands[j][0], "cd") == 0) {
              handle_cd(cmd_token_count > 1 ? commands[j][1] : "~", home_env);
              handle_history(commands[j][1]);
            } else if (strcmp(commands[j][0], "history") == 0) {
              handle_history(commands[j][1]);
              display_history(history_count, history);
            }
            exit(0);
          } else if (find_file(commands[j][0], path_env)) {
            execute_command(commands[j], cmd_token_count);
            handle_history(commands[j][0]);
            exit(0);
          } else {
            execvp(commands[j][0], commands[j]);
            perror(commands[j][0]);
            exit(1);
          }
        }
        
        children[j] = child;
      }
      
      for (int x = 0; x < pipe_count; x++) {
        close(pipefd[x][0]);
        close(pipefd[x][1]);
      }
      
      int statuses[MAX_COMMANDS];
      for (int j = 0; j <= pipe_count; j++) {
        waitpid(children[j], &statuses[j], 0);
      }
      
      for (int j = 0; j <= pipe_count; j++) {
        free(commands[j]);
      }
      free(commands);
      
      for (int i = 0; i < arg_count; i++) {
        free(tokens[i]);
      }
      free(tokens);
      continue;
    }

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
      handle_history(tokens[0]);
    } else if (strcmp(tokens[0], "type") == 0) {
      handle_type(tokens, redirect_index == -1 ? arg_count : redirect_index, path_env);
      handle_history(tokens[0]);
    } else if (strcmp(tokens[0], "pwd") == 0) {
      handle_pwd();
      handle_history(tokens[0]);
    } else if (strcmp(tokens[0], "cd") == 0) {
      handle_cd(arg_count > 1 ? tokens[1] : "~", home_env);
      handle_history(tokens[0]);
    } else if (strcmp(tokens[0], "history") == 0) {
      handle_history(tokens[0]);
      display_history(history_count, history);
    } else if (find_file(tokens[0], path_env)) {
      execute_command(tokens, arg_count);
      handle_history(tokens[0]);
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

  for (int i = 0; i < completion_count; i++) {
    free(completion_list[i]);
  }
  free(completion_list);

  return 0;
}
