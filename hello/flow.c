#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#define BUF_SIZE 128
int main(int argc, char ** argv) {
char buf[BUF_SIZE];
int * pipeFd = malloc(2 * 4);
pipe(pipeFd);
if (!fork()) {
close(pipeFd[0]);
dup2(pipeFd[1], 1);
close(pipeFd[1]);
for (int i = 0; i < 100; i++) {
printf("Something!\n");
}
//close(pipeFd[1]);
//close(pipeFd[0]);
} else {
close(pipeFd[1]);
dup2(pipeFd[0], 0);
close(pipeFd[0]);
while(fgets(buf, BUF_SIZE, stdin) != NULL) {
printf("Pipeline: %s", buf);
}
printf("Pipeline: Empty!\n");
//close(pipeFd[0]);
}
}
//close(pipeFd[0]);
//close(pipeFd[1]);
