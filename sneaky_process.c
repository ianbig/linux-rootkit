#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define BUFFER_SIZE 1024
#define INPUT_STR_SIZE 100
void readCharacter() {
  char ch = 0;
  while ((ch = getchar()) != 'q') {
  }
}

void packSystemString(char * string, pid_t pid) {
  snprintf(string, INPUT_STR_SIZE, "insmod sneaky_mod.ko sneaky_pid=%d", (int)pid);
}

int main() {
  // load module
  pid_t pid = getpid();
  printf("sneaky process with pid %d\n", (int)pid);
  char system_str[INPUT_STR_SIZE] = {0};
  packSystemString(system_str, pid);
  system(system_str);
  readCharacter();
  system("rmmod sneaky_mod");
}
