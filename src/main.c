#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  char command[1024];
  char *built_in_commands[] = {"exit", "echo", "type"};

  while (1){
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
      if (strcmp(command+5, "exit") == 0 || strcmp(command+5, "echo") == 0 || strcmp(command+5, "type") == 0)
        printf("%s is a shell builtin\n", command+5);
      else{
        printf("%s: command not found\n", command);
      }
    }
    else{
    printf("%s: command not found\n", command);
    }
  }

  return 0;
}
