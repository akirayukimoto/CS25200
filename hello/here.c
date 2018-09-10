#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSG_LEN 128
int main(int argc, char **argv) {
  char msg[MSG_LEN];

  int *pipeFd = malloc(8);
  pipe(pipeFd);
  dup2(pipeFd[0], 0);
  dup2(pipeFd[1], 1);

  int fdpipe[2];
  pipe(fdpipe);

  if (fork() == 0) {
    puts("Hello?");
    fflush(stdout);
    dup2(fdpipe[0], 0);
//    close(fdpipe[0]);
    fgets(msg, MSG_LEN, stdin);
  } else {
    fgets(msg, MSG_LEN, stdin);
    dup2(fdpipe[1], 1);
//    close(fdpipe[1]);
    puts("Can you here me now?");
    fflush(stdout);
  }
  fprintf(stderr, "%s\n", msg);
}
