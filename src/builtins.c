#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <readline/history.h>
#include "builtins.h"
#include "shell.h"

const char *builtin_commands[] = {"exit", "echo", "type", "pwd", "cd", "history"};
char current_history_file[MAX_PATH_LEN] = "history.txt";

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

void handle_history(int count, char *arg, char *path_env, char *history_file) {
  // Handle history -r <path> to set the history file location
  if (arg != NULL && strcmp(arg, "-r") == 0 && history_file != NULL) {
    // Read existing contents from the new history file
    char *content_buffer[MAX_HISTORY];
    int existing_lines = 0;
    
    FILE *file = fopen(history_file, "r");
    if (file != NULL) {
      char line[1024];
      while (fgets(line, sizeof(line), file) != NULL && existing_lines < MAX_HISTORY) {
        line[strcspn(line, "\n")] = 0;
        content_buffer[existing_lines] = strdup(line);
        existing_lines++;
      }
      fclose(file);
    }
    
    // Reopen file in write mode and write history -r command first
    file = fopen(history_file, "w");
    if (file != NULL) {
      fprintf(file, "history -r %s\n", history_file);
      
      // Write back all existing content
      for (int i = 0; i < existing_lines; i++) {
        fprintf(file, "%s\n", content_buffer[i]);
      }
      fclose(file);
    }
    
    // Cleanup buffer
    for (int i = 0; i < existing_lines; i++) {
      free(content_buffer[i]);
    }
    
    // Update the current history file path
    strncpy(current_history_file, history_file, MAX_PATH_LEN - 1);
    current_history_file[MAX_PATH_LEN - 1] = '\0';
    return;
  }

  else if (arg != NULL && strcmp(arg, "-w") == 0 && history_file != NULL) {
    char *content_buffer[MAX_HISTORY];
    int existing_lines = 0;

    if (strcmp(history_file, current_history_file) != 0) {

    FILE *file = fopen(current_history_file, "r");
      if (file != NULL) {
        char line[1024];
        while (fgets(line, sizeof(line), file) != NULL && existing_lines < MAX_HISTORY) {
          line[strcspn(line, "\n")] = 0;
          content_buffer[existing_lines] = strdup(line);
          existing_lines++;
        }
        fclose(file);
      }
    }

    FILE *file = fopen(history_file, "a");
    if (file != NULL) {
      if (existing_lines == 0) {
        fprintf(file, "history -w %s\n", history_file);
      }
      else {
        for (int i = 0; i < existing_lines; i++) {
          fprintf(file, "%s\n", content_buffer[i]);
        }
        fprintf(file, "history -w %s\n", history_file);
      }
      fclose(file);
    }

    for (int i = 0; i < existing_lines; i++) {
      free(content_buffer[i]);
    }
    existing_lines = 0;

    strncpy(current_history_file, history_file, MAX_PATH_LEN - 1);
    current_history_file[MAX_PATH_LEN - 1] = '\0';
    return;

  }
  
  // Regular history display - read from the current history file
  FILE *file = fopen(current_history_file, "r");
  if (file == NULL) {
    return;
  }
  
  char line[1024];
  char *temp_buffer[MAX_HISTORY];
  int line_number = 0;
  
  // Read all lines from history file
  while (fgets(line, sizeof(line), file) != NULL && line_number < MAX_HISTORY) {
    line[strcspn(line, "\n")] = 0;
    temp_buffer[line_number] = strdup(line);
    line_number++;
  }
  
  // Determine starting index for display
  int start_index = 0;
  if (count > 0) {
    start_index = line_number - count;
    if (start_index < 0) start_index = 0;
  }

  // Display entries
  for (int i = start_index; i < line_number; i++) {
    printf("%5d  %s\n", i + 1, temp_buffer[i]);
  }
  
  // Cleanup
  for (int i = 0; i < line_number; i++) {
    free(temp_buffer[i]);
  }
  
  fclose(file);
}