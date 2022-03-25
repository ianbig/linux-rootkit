#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

void readCharacter() {
  char ch = 0;
  while ((ch = getchar()) != 'q') {
  }
}

int main() {
  // load module
  system("insmod sneaky_mod.ko");
  readCharacter();
  system("rmmod sneaky_mod");
}
