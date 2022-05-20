#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/mman.h>
#include "monitor.h"

#define BuffNo 3
#define N 10

int BufferSize[] = {10,8,6};
int BufferSynch[] = {3,2,1};

int pidsC[BuffNo];
int pidsP[BuffNo];

int i = 20;

void killAllChildren(int x) {
    for (int i = 0; i < BuffNo; i++) {
        int child = pidsP[i];
        kill(child, SIGKILL);
        child = pidsC[i];
        kill(child, SIGKILL);
    }
}

char produceItem (int id) {
  char item;
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
  return item;
}

class Buffer:Monitor {
private:
  char buffer[N];
  Condition full;
  Condition empty;
  int bufferSize;
  
public:
  Buffer();
  void put(int id, char item);
  char remove(int id);
  void printBuffer();
};

Buffer::Buffer(){
  bufferSize = 0;
}

void Buffer::printBuffer() {
    printf("[ ");
    for(int i=0; i<bufferSize; i++) {
       printf("(%c) ",buffer[i]);
    }
    printf(" ]\n");
}

void Buffer::put(int id, char item){
  enter();
  int synch = BufferSynch[id];
  if (bufferSize >= BufferSize[id]-synch+1){
    std::cout << "Producer " << id << " waits for empty slots in Buffer "<< id << "\n";
    wait(full);
  }
  for(int i=0; i<BufferSynch[id]; ++i) {
    buffer[bufferSize] = item;
    std::cout << "Producer " << id << " inserted " << item <<" to Buffer " << id << "\n";
    ++bufferSize;
    std::cout << "Buffer " << id << " size: " << bufferSize <<"\n";
    std::cout << "Buffer " << id <<": ";
    printBuffer();
  }
  if (bufferSize == synch) { std::cout << "Producer " << id << " sent signal empty\n"; signal(empty); }
  leave();
}

char Buffer::remove(int id){
  enter();
  char ret;
  int synch = BufferSynch[id];
  if (bufferSize < synch){
    std::cout << "Consumer " << id << " waits for filled slots in Buffer " << id <<"\n";
    wait(empty);
  }
  for(int i=0; i<BufferSynch[id]; ++i) {
	switch(id) {
	    case 0:
	    case 2:
	        ret = buffer[0];
			for(int j=0; j<bufferSize; j++){
			  buffer[j] = buffer[j+1];
			}
		break;
		case 1:
			ret = buffer[bufferSize-1];
		break;
	}
	std::cout << "Consumer " << id << " removed  " << ret << " from Buffer " << id << "\n";
	--bufferSize;
	std::cout << "Buffer " << id << " size: " << bufferSize <<"\n";
	std::cout << "Buffer " << id << ": ";
	printBuffer();
  }
  if (bufferSize == (BufferSize[id] - synch)) { std::cout << "Consumer " << id << " sent signal full\n"; signal(full);}
  leave();
  return ret;
}

void* createSharedMemory(size_t size) {
    int protection = PROT_READ | PROT_WRITE;

    int visibility = MAP_ANONYMOUS | MAP_SHARED;

    void* toReturn = mmap(NULL, size, protection, visibility, 0, 0);
    if(toReturn == MAP_FAILED)
    {
        printf("Failed to allocate shared memory.\n");
        exit(EXIT_FAILURE);
    }
    return toReturn;
}



void produce(int id, Buffer* buffer){
  while(i--) {
    char item = produceItem(id);
    buffer->put(id,item);
  }
  printf("Producer %d finished\n",id);
  signal(SIGALRM, killAllChildren);
}


void consume(int id, Buffer * buffer){
  while(i--) {
    buffer->remove(id);
  }
  printf("Consumer %d finished\n",id);
  signal(SIGALRM, killAllChildren);
}




void createProducent(int id, Buffer * buffer){
  int created = fork();
  if(created == 0){
    produce(id,buffer);
    exit(0);
  }else {
    pidsP[id] = created;
  }
}

void createConsumer(int id, Buffer * buffer){
  int created = fork();
  if(created == 0){
    consume(id,buffer);
    exit(0);
  }else {
    pidsC[id] = created;
  }
}

int main(int argc, char const *argv[]) {
  srand (time(NULL));

  
  Buffer buffer[BuffNo];
  Buffer* buff[BuffNo];

  for(int i=0; i<BuffNo; i++) {
    buff[i] =(Buffer*) createSharedMemory(sizeof(Buffer));
    memcpy(buff[i], &buffer[i], sizeof(Buffer));
  }

  for(int i=0; i<BuffNo; i++) {
    createConsumer(i,buff[i]);
    createProducent(i,buff[i]);
    
  }
  alarm(7);
  while(wait(NULL) > 0) {}
  return 0;
}

