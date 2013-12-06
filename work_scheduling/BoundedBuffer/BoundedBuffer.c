#include <stdio.h>
#include <stdlib.h>
#include "BoundedBuffer.h"

/* config */ uint64_t P = 2;
/* config */ uint64_t C = 2;
/* config */ uint64_t N = 1000;
/* config */ uint64_t capacity = 4;

BoundedBuffer the_buffer;

const double TERM = -1.0f;

void initBoundedBuffer( BoundedBuffer * b, uint64_t capacity ) {
  // Set the capacity ofthe buffer
  b->capacity = capacity;

  // Initialize the "is empty" condition variable for each element of the buffer
  b->aCondEmpty = malloc (capacity * sizeof(pthread_cond_t));
  for (int64_t i = 0; i < capacity; ++i) {
     pthread_cond_init(&b->aCondEmpty[i], NULL);
  }

  // Initialize the "is full" condition variable for each element of the buffer
  b->aCondFull = malloc (capacity * sizeof(pthread_cond_t));
  for (int64_t i = 0; i < capacity; ++i) {
     pthread_cond_init(&b->aCondFull[i], NULL);
  }

  // Initilize a mutex for each element of the buffer
  b->aMutex = malloc (capacity * sizeof(pthread_mutex_t));
  for (int64_t i = 0; i < capacity; ++i) {
     pthread_mutex_init(&b->aMutex[i],NULL);
  }

  // Initialize the buffer
  b->aItem = malloc(capacity * sizeof(double));
  for (int64_t i = 0; i < capacity; ++i) {
     b->aItem[i]= 0.0f;
  }

  // Initialize a flag for each element of the buffer
  // 0 == empty, 1 == full
  b->aFlag = malloc(capacity * sizeof(uint64_t));
  for (int64_t i = 0; i < capacity; ++i) {
     b->aFlag[i]= 0;
  }
 
  // Initialize the producerIndex mutex, and the Consumer Index Mutex, as well as both indexes
  pthread_mutex_init(&b->producerIndexLock, NULL);
  b->producerIndex = 0;
  pthread_mutex_init(&b->consumerIndexLock, NULL);
  b->consumerIndex = 0;

}

void produce( BoundedBuffer * b, double item ) {
  int index = 0;
  
  // Get Our index
  //printf("Getting Index\n");
  pthread_mutex_lock(&b->producerIndexLock);
  index = b->producerIndex;
  b->producerIndex += 1;
  b->producerIndex %= b->capacity;
  pthread_mutex_unlock(&b->producerIndexLock);

  // Acquire the lock
  //printf("Acquiring Lock\n");
  pthread_mutex_lock(&b->aMutex[index]);

  // wait till the index is empty
  while (b->aFlag[index] == 1) {
     //printf("Waiting till empty\n");
     pthread_cond_wait(&b->aCondEmpty[index], &b->aMutex[index]);
  }

  // Fill the index
  //printf("Filling the index \n");
  b->aItem[index] = item;
  b->aFlag[index] = 1;

  // Signal that it is full
  //printf("Signaling index is full \n");
  pthread_cond_signal(&b->aCondFull[index]);

  // Release the lock
  //printf("Releasing thelock\n");
  pthread_mutex_unlock(&b->aMutex[index]);
}

double consume( BoundedBuffer * b ) {
   double item = 0.0;

   // Get our index
   int index = 0;
   pthread_mutex_lock(&b->consumerIndexLock);
   index = b->consumerIndex;
   b->consumerIndex += 1;
   b->consumerIndex %= b->capacity;
   pthread_mutex_unlock(&b->consumerIndexLock);

   // Acquire the lock
   pthread_mutex_lock(&b->aMutex[index]);

   // wait till the index is full
   while (b->aFlag[index] == 0) {
      pthread_cond_wait(&b->aCondFull[index], &b->aMutex[index]);
   }

   // consume the index
   item = b->aItem[index];
   b->aFlag[index] = 0;

   // signal that it is empty
   pthread_cond_signal(&b->aCondEmpty[index]);

   // release the lock
   pthread_mutex_unlock(&b->aMutex[index]);

   return item;
}


// producer thread procedure
void * producer( void * arg ) {
  uint64_t id = (uint64_t) arg;

  // produces 0..#N by P align id
  uint64_t count = 0;
  for ( uint64_t j=id; j<N; j+=P ) {
    //printf("%lld producing %lld \n", id, j);
    produce( &the_buffer, j );
    count++;
  }

  return (void*)count;
}

// consumer thread procedure
void * consumer( void * ignore ) {
  // consumes items until it eats a TERM element
  uint64_t count = 0;
  double data;
  do {
    data = consume( &the_buffer );
    count++;
  } while (data != TERM);

  return (void*)(count-1);
}


int main(int argc, char ** argv) {
  if (argc != 5) {
    printf("usage: %s <capacity> <N> <P> <C>\n", argv[0]);
    exit(1);
  }

  // command line parse
  capacity = atoi(argv[1]);
  N = atoi(argv[2]);
  P = atoi(argv[3]);
  C = atoi(argv[4]);
   
  //printf("initializg buffer \n");
  initBoundedBuffer( &the_buffer, capacity );

  // create P producers
  //printf("Creating Producers \n");
  pthread_t P_thread_ids[P];
  for ( int64_t i=0; i<P; i++ ) {
    pthread_create( &P_thread_ids[i], NULL, producer, (void*) i);
  }
  
  // create C consumers
  //printf("Creating Consumers \n");
  pthread_t C_thread_ids[C];
  for ( int64_t i=0; i<C; i++ ) {
    pthread_create( &C_thread_ids[i], NULL, consumer, NULL);
  }

  // join on producers
  //printf("joining producers\n");
  uint64_t P_count[P];
  for ( int64_t i=0; i<P; i++ ) {
    void * status;
    pthread_join( P_thread_ids[i], &status );
    P_count[i] = (uint64_t) status;
  }


  // create C signal exits to kill consumers
  //printf("killing consumers");
  for ( int64_t i=0; i<C; i++ ) {
    produce(&the_buffer, TERM);
  }

  // join on consumers
  //printf("joining consumers \n");
  uint64_t C_count[C];
  for ( int64_t i=0; i<C; i++ ) {
    void * status;
    pthread_join( C_thread_ids[i], &status );
    C_count[i] = (uint64_t) status;
  }


  // print out p/c counts
  printf("== TOTALS ==\n");
  printf("P_Count=");
  for ( int64_t i=0; i<P; i++ ) {
    printf(" %ld", P_count[i]);
  }
  printf("\n");
  printf("C_Count=");
  for ( int64_t i=0; i<C; i++ ) {
    printf(" %ld", C_count[i]);
  }
  printf("\n");
}

