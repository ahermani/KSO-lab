#include </usr/include/stdio.h>
#include </usr/include/lib.h>

int getprocnr(int pid) {
	message msg;
	msg.m1_i1 = pid; 
	return (_syscall(0, 78, &msg));
}

int main (int argc, char* argv[]) {
	int i, index;

	if (argc <= 1) {
		printf("Please specify PID as argument\n");
	} else { 
		int pid = atoi(argv[1]);

		for (i=10; i>=0; i--) {
      			index = getprocnr(pid-i);

			if (index == -1)
				printf("Error: %d. Process %d does not exist\n", errno, pid-i);	
      			else
         			printf("PID: %d, Index: %d\n", pid-i, index);
		}
  	}

  	return 0;
}