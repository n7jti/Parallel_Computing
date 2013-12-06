#include <stdint.h>
#include <pthread.h>

typedef struct BoundedBuffer {

   // capacity of the buffer
   uint64_t capacity;

   // Condition variable indicating that the index is empty
   pthread_cond_t *aCondEmpty;

   // Condition variable indicating that the index is full
   pthread_cond_t *aCondFull;

   // Mutex protecting each index
   pthread_mutex_t *aMutex;

   // The buffer
   double *aItem;

   // Flag for each element in the buffer
   //  0 == Empty
   //  1 == Full
   uint64_t *aFlag;
   
   // A lock to protect the producer curser, and the cursor it protects 
   pthread_mutex_t producerIndexLock;
   uint64_t producerIndex; 

   // A lock to protect the consumer cursor, and the cursor it protects
   pthread_mutex_t consumerIndexLock;
   uint64_t consumerIndex;

} BoundedBuffer;


// Initialize buffer to be empty and
// have the given capacity
void initBoundedBuffer( BoundedBuffer * b, uint64_t capacity );

// Insert the item into the buffer.
// Blocks until there is room in buffer. 
void produce( BoundedBuffer * buffer, double item );

// Delete one item in the buffer and return its value.       
// Blocks until the buffer is not empty.
double consume( BoundedBuffer * buffer );
