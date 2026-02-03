#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  char command[1024];

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
    else{
    printf("%s: command not found\n", command);
    }
  }

  return 0;
}
