#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define N 10
#define RAND_DIVISOR 1000000000
#define BuffNo 3

int BufferSize[] = {10,8,6};
int BufferSynch[] = {3,2,1};

struct Semaphores{
  sem_t empty[BuffNo];
  sem_t full[BuffNo];
  sem_t mutex[BuffNo];
};

struct Semaphores * getSemaphores()
{

    static int shmid = 0;

    if(shmid == 0)
            shmid = shmget(IPC_PRIVATE,BuffNo * (sizeof(struct Semaphores) + sizeof(sem_t)),SHM_W|SHM_R);

    if(shmid < 0)
    {
        printf("shmget error\n");
        abort();
    }

    void * Data = shmat(shmid, NULL, 0);
    struct Semaphores * semaphores = (struct Semaphores *) Data;

    return semaphores;
}

struct Buffer{
  char buff[N][BuffNo];
  unsigned int bufferSize[BuffNo];
};

void printBuffer(struct Buffer *buff, int nr) {
    printf("[ ");
    for(int i=0; i<buff->bufferSize[nr]; i++) {
       printf("(%c) ",buff->buff[i][nr]);
    }
    printf(" ]\n");
}

void check_full(struct Buffer *buff, int nr) {
	if(buff->bufferSize[nr] >= BufferSize[nr])
		printf("ERROR! BUFFER FULL!!!");
}

void check_empty(struct Buffer *buff, int nr) {
	if(buff->bufferSize[nr] <=0)
		printf("ERROR! BUFFER EMPTY!!!");
}

struct Buffer * getBuffers()
{
    static int shmid = 0;
    unsigned int i;

    if(shmid == 0)
    {
        shmid = shmget(IPC_PRIVATE, sizeof(struct Buffer) + N * sizeof(unsigned int), SHM_W|SHM_R);
    }

    if(shmid < 0)
    {
        printf("shmget error\n");
        abort();
    }

    void * Data = shmat(shmid,NULL,0);

    struct Buffer * buffers = (struct Buffer *)Data;

    return buffers;
}

void init(){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();

  for(int i=0; i< BuffNo; i++) {
  buffer->bufferSize[i] = 0;

  sem_init(&semaphores->empty[i], 1, BufferSize[i]);
  sem_init(&semaphores->full[i], 1, 0);
  sem_init(&semaphores->mutex[i], 1, 1);
  }

}


void producer(int id){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();

  while(1){
    int rNum = rand() / RAND_DIVISOR;
    sleep(rNum);
    char item;
    for(int i=0; i<BufferSynch[id]; i++){
	    sem_wait(&semaphores->empty[id]);
	    sem_wait(&semaphores->mutex[id]);
            check_full(buffer, id);
	    switch(id) {
	    case 0:
		item = (char)(48 + (rand() % 10));
            break;
            case 1:
		item = (char)('a' + (rand() % 26));
            break;
            case 2:
		item = (char)('A' + (rand() % 26));
            break;
	    }
	    printf("Producer %d: Insert Item %c to Buffer %d\n", id+1, item, id+1);
	    buffer->buff[buffer->bufferSize[id]][id] = item;
	    ++buffer->bufferSize[id];
            printf("Buffer %d length: %d \n",id+1, buffer->bufferSize[id]);
            printf("Buffer %d :", id+1);
	    printBuffer(buffer,id);
	    sem_post(&semaphores->mutex[id]);
	    sem_post(&semaphores->full[id]);
    }
    usleep(900000);
  }
}

void consumer(int id){
  struct Buffer * buffer = getBuffers();
  struct Semaphores * semaphores = getSemaphores();
  char item;

  while(1){
    int rNum = rand() / RAND_DIVISOR;
    sleep(rNum);

    for(int i=0; i<BufferSynch[id]; i++){

      sem_wait(&semaphores->full[id]);
      sem_wait(&semaphores->mutex[id]);
      check_empty(buffer, id);
      switch(id) {
      case 0:
      case 2:
	      item = buffer->buff[0][id];
	      printf("Consumer %d: Remove Item %c from Buffer %d\n",id+1,item, id+1);
	      for(int j=0; j<buffer->bufferSize[id]; j++){
		buffer->buff[j][id] = buffer->buff[j+1][id];
	      }
      break;
      case 1:
              item = buffer->buff[(buffer->bufferSize[id])-1][id];
              printf("Consumer %d: Remove Item %c from Buffer %d\n",id+1,item, id+1);
      }
      --buffer->bufferSize[id];
      printf("Buffer %d length: %d \n",id+1, buffer->bufferSize[id]);
      printf("Buffer %d :", id+1);
      printBuffer(buffer,id);
      sem_post(&semaphores->mutex[id]);
      sem_post(&semaphores->empty[id]);
    }
    usleep(1000000);
  }
}

void createConsumer(int id){
  int created = fork();
  if(created == 0){
    consumer(id);
    exit(0);
  }
}
void createProducent(int id){
  int created = fork();
  if(created == 0){
    producer(id);
  }
}

int main(int argc, char const *argv[]) {
  srand (time(NULL));
  init();

  for(int i=0; i<BuffNo; i++) {
     createConsumer(i);
     createProducent(i);
  }

  while(1){}

  return 0;
}
