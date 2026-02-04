#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef _WIN32
#define PATH_LIST_SEPARATOR ';'
#else
#define PATH_LIST_SEPARATOR ':'
#endif


int main(int argc, char *argv[]) {
  char command[1024];
  char *built_in_commands[] = {"exit", "echo", "type"};

  while (1){
    char *path_env = getenv("PATH");
    // Flush after every printf
    setbuf(stdout, NULL);
    printf("$ ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = 0;

    if (strcmp(command, "exit") == 0) {
      break;
    }
    else if (strncmp("echo", command, 4) == 0) {
      printf("%s\n", command+5);
    }
    else if (strncmp("type", command, 4) == 0) {
      char *arg = command + 5;
      if (strcmp(command+5, "exit") == 0 || strcmp(command+5, "echo") == 0 || strcmp(command+5, "type") == 0)
        printf("%s is a shell builtin\n", command+5);
      else{
        if (path_env != NULL) {
          char *path_copy = strdup(path_env);
          char *dir = strtok(path_copy, ":");
          bool found = false;
          while (dir != NULL && !found)
          {
            char full_path[1024];
            
          snprintf(full_path, sizeof(full_path), "%s/%s", dir, arg);
            if (access(full_path, X_OK) == 0) {
              printf("%s is %s\n", arg, full_path);
              found = true;
            }
            dir = strtok(NULL, ":");
          }
          free(path_copy);
          if (!found){
            printf("%s: not found\n", command+5);
          }
        }
        else{
          printf("%s: not found\n", command+5);
        }
        
      }
    }
    else {
      if (path_env != NULL) {
          char *path_copy = strdup(path_env);
          char *dir = strtok(path_copy, ":");
          bool found = false;
          while (dir != NULL && !found)
          {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
            if (access(full_path, X_OK) == 0) {
              system(full_path);
              found = true;
            }
            dir = strtok(NULL, ":");
          }
          free(path_copy);
      
      printf("%s: command not found\n", command);
    }
  }
}

  return 0;
}
