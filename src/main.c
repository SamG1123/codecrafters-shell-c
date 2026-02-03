#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  char command[1024];

  fgets(command, sizeof(command), stdin);
  command[strcspn(command, "\n")] = 0;
  printf("%s: command not found", command);
  

  return 0;
}
