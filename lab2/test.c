#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <lib.h>
#include <minix/type.h>
#include <time.h>

int main(int argc, char* argv[]) {
  time_t timer = time(NULL);
  int N=100;
  int M=99999999;
  message msg;
  pid_t pid = getpid();
  int option = atoi(argv[1]);
  int value = atoi(argv[2]);
  int i, ii;

  msg.m1_i1 = pid;
  msg.m1_i2 = option;
  msg.m1_i3 = value;
  _syscall(MM, SETPRI, &msg);

  for(i=0; i<N; i++) {
    for(ii=1; ii<M; ii++) {;}
  }
  printf("\n");
  printf("PID: %d, PRI: %d, TIME: %d\n",pid,value,time(NULL)-timer);
  return 0;
}
