#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define MAX_RAMP_VALUE 15

void *ThreadProc(void* ptr);

void computeMyBlockPart(
   int64_t numItems, // # items to distribute
   int64_t numTasks, // # of tasks
   int64_t myTaskId, // my ID: 0..numTasks-1
   int64_t *myLo,    // my low bound
   int64_t *myHi     // my high bound
   );   

void computeMyCyclicPart(
   int64_t numItems, // # items to distribute
   int64_t numTasks, // # of tasks
   int64_t myTaskId, // my ID: 0..numTasks-1
   int64_t *myLo,    // my low bound
   int64_t *myHi     // my high bound
   );
   

typedef struct
{
   int64_t numItems;
   int64_t numTasks;
   int64_t myTaskId;
   int64_t *aData;
   bool fBlock;
   bool fSimple;
} THREAD_PARAMS;

int64_t parseCommandLine(int argc, char* argv[], int64_t *cThreads, int64_t *cTasks, bool *pfBlock, bool* pfSimple, bool* pfRandom);
void usage();

int64_t createThreads(int64_t cTasks, int64_t cItems, bool fBlock, bool fSimple,  int64_t * aData, pthread_t **aThreads, THREAD_PARAMS** aThreadParams);
int64_t joinThreads(int64_t cTasks, int64_t cItems,  pthread_t* aThreads, THREAD_PARAMS* aThreadParams);


// timing code copied from 
// http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/

struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

// end copy

int64_t rampValue(int64_t cItems, int64_t index);
int64_t allocateArray(bool fRandom, int64_t cItems, int64_t ** aArray);
int64_t test(bool fSimple, int64_t cItems, int64_t * aArray);

int main (int argc, char *argv[])
{
   struct timespec tStart;
   struct timespec tEnd;
   pthread_t *aThreads;
   THREAD_PARAMS *aParams;
   int64_t * aArray = NULL;

   int64_t iret = 0;
   int64_t cThreads = 0;
   int64_t cTasks = 0;
   bool fBlock = false;
   bool fSimple = true; 
   bool fRandom = false;

   iret = parseCommandLine(argc, argv, &cThreads, &cTasks, &fBlock, &fSimple, &fRandom);

   if (0 == iret) {
      iret = allocateArray(fRandom, cTasks, &aArray);
   }

   if (0 == iret) {
      iret = clock_gettime(CLOCK_REALTIME, &tStart);
   }
   
   if (0 == iret) {
      iret = createThreads(cThreads, cTasks, fBlock, fSimple, aArray, &aThreads, &aParams);
   }

   if (0 == iret) {
      iret = joinThreads(cThreads, cTasks, aThreads, aParams);
      aThreads = NULL;
      aParams = NULL;
   }

   clock_gettime(CLOCK_REALTIME, &tEnd);

   if (0 == iret) {
      //test(fSimple, cTasks, aArray);
   }

   free(aArray);

   struct timespec tsDiff = diff(tStart, tEnd);
   //printf("Elapsed Time: %d.%09d Sec.\n", tsDiff.tv_sec, tsDiff.tv_nsec);
   printf("%d, %d, %s, %s, %s, %d.%09d\n", cThreads, cTasks, fBlock ? "block":"cyclic", fSimple ? "negation":"factorial", fRandom ? "random":"ramp",tsDiff.tv_sec, tsDiff.tv_nsec);
   return(iret);
}

int64_t parseCommandLine(int argc, char* argv[], int64_t *cThreads, int64_t *cTasks, bool *pfBlock, bool* pfSimple, bool *pfRandom){ 
   int64_t iret = 0;
   if (argc != 6) {
      usage();
      iret = 1;
   }
   *pfBlock = false;
   *pfRandom = false;

   if (0 == iret) {
      *cThreads = atol(argv[1]);
      if (*cThreads <= 0 || *cThreads > 1000) {
         usage();
         printf("Must be between 1 and 1000 threads.\n");
         iret = 1;
      }
   }

   if (0 == iret) {
      *cTasks = atol(argv[2]);
      if (*cTasks <= 0 || *cTasks > 10E9) {
         usage();
         printf("Must be between 1 and 10E9 tasks.\n");
         iret = 1;
      }
   }

   if (0 == iret) {
      *cTasks = atol(argv[2]);
      if (*cTasks <= 0 || *cTasks > 10E9) {
         usage();
         printf("Must choose block or cyclic");
         iret = 1;
      }
   }

   if (0 == iret) {
      if (argv[3][0] == 'b') {
         *pfBlock = true;
         //printf("Block\n");
      }
      else
      {
         //rintf("Cyclic\n");
      }
   }

   if (0 == iret) {
      if (argv[4][0] == 'c') {
         *pfSimple = false;
         //rintf("Complex\n");
      }
      else
      {
         //printf("Simple\n");
      }
   }

   if (0 == iret) {
      if (argv[5][0] == 'r') {
         *pfRandom = true;
         //rintf("Complex\n");
      }
      else
      {
         //printf("Simple\n");
      }
   }

   if (iret == 0) {
      // automaticall clamp the number of threads to the number of tasks
      if (*cThreads > *cTasks) {
         *cThreads = *cTasks;
      }
   }

   return iret;
}

void usage(){
   printf("Assignment1 Usage:\n");
   printf("\t Assignment1 coutOfThreads coutOfTasks b[lock]|c[yclic] s[imple]|c[omplex] m[onotonic]|r[andom]\n");
   printf("\n");
}

void computeMyBlockPart(
   int64_t numItems, // # items to distribute
   int64_t numTasks, // # of tasks
   int64_t myTaskId, // my ID: 0..numTasks-1
   int64_t *myLo,    // my low bound
   int64_t *myHi)    // my high bound
{    
   // compute the block
   int64_t size = numItems / numTasks;  // note: integer division
   int64_t remainder = numItems % numTasks;

   // now divide the remaining tasks up
   if (myTaskId < remainder) {
      size += 1;
   }

   int64_t lo = size * myTaskId;
   if (myTaskId >= remainder) {
      lo += remainder;
   }

   *myLo = lo;
   *myHi = lo + size;
}

void computeMyCyclicPart(
   int64_t numItems, // # items to distribute
   int64_t numTasks, // # of tasks
   int64_t myTaskId, // my ID: 0..numTasks-1
   int64_t *myLo,    // my low bound
   int64_t *myHi     // my high bound
   ) 
{    
   //int64_t Hi = numItems - numItems % numTasks + myTaskId;
   //if (myTaskId > numItems % numTasks) {
   //   Hi -= numTasks;   
   //}

   *myLo = myTaskId;
   *myHi = numItems;

   //printf("Lo: %d Hi:%d Stride:%d\n", myTaskId, numItems, numTasks);

}

int64_t rampValue(int64_t cItems, int64_t index){
   int64_t out;
   double numItems = (double)cItems;
   double value = (index / cItems) * MAX_RAMP_VALUE;
   value = ceil(value);
   out = (int64_t)value;
   return out;
}

int64_t randValue(int64_t cItems, int64_t index){
   int64_t out;
   out = rand() % MAX_RAMP_VALUE;
   return out;
}

int64_t allocateArray(bool fRandom, int64_t cItems, int64_t ** aArray){
   int64_t iret = 0;
   int64_t *ary = NULL;
   ary = (int64_t *) malloc(cItems * sizeof(int64_t));
   if (NULL == ary) {
      printf("Out Of Memory in allocateArray\n");
      iret = 1; // OUT OF MEMORY!
   }

   if (0 == iret) {
      //Initialize
      if (fRandom) {
         for (int64_t i = 0; i < cItems; i++) {
            ary[i] = randValue(cItems,i);
         }
      }
      else{
         for (int64_t i = 0; i < cItems; i++) {
            ary[i] = rampValue(cItems,i);
         }
      }
   }

   if (0 == iret) {
      *aArray = ary;
   }
   else
   {
      free(ary);
   }

   return iret;
}

int64_t createThreads(int64_t cTasks, int64_t cItems, bool fBlock, bool fSimple, int64_t *aData, pthread_t **aThreads, THREAD_PARAMS **aParams){
   int64_t iret = 0; 
   pthread_t *threads = NULL;
   THREAD_PARAMS *params = NULL;
   
   // initialize output parameters
   *aThreads = NULL;
   *aParams = NULL;

   // allocate the thread contexts
   threads = (pthread_t*) calloc(cTasks, sizeof(pthread_t));
   if (NULL == threads) {
      printf("Out of memory allocating pthreat_t*!\n");
      iret = 1; //out of memory
   }

   // allocate the thead params
   if (0 == iret) {
      params = (THREAD_PARAMS*) calloc(cTasks, sizeof(THREAD_PARAMS));
      if (NULL == params) {
         printf("Out of memroy allocating params!\n");
         iret = 1; // out of memory
      }
   }

   if (0 == iret) {

      
      for (int64_t i = 0; i < cTasks; ++i) {
         // Initialize the thread params
         params[i].myTaskId = i;
         params[i].numTasks = cTasks;
         params[i].numItems = cItems; 
         params[i].aData = aData;
         params[i].fBlock = fBlock;
         params[i].fSimple = fSimple;

         iret = pthread_create(&threads[i], NULL, ThreadProc, &params[i]);
         if (0 != iret) {
            printf("Failed to create threads!");
            break;
            // I'm not going to worry about cleaning this mess up if only some threads are created.
         }
      }
   }

   if (0 == iret) {
      *aThreads = threads;
      *aParams = params;
   }
   else {
      // Cleanup
      if (NULL != threads) {
         free(threads);
         threads = NULL;
      }

      if (NULL != params) {
         free(params);
         params = NULL;
      }
   }

   return iret;
}


int64_t joinThreads(int64_t cTasks, int64_t cItems, pthread_t* aThreads, THREAD_PARAMS* aParams){
   int64_t iret = 0;
   // Join 'dem threads
   for (int64_t i = 0; i < cTasks; ++i) {
      iret = pthread_join(aThreads[i], NULL);
      if (0 != iret) {
         printf("Failed to join threads!");
         break;
      }
   }

   //free(aParams[0].aData);
   free(aThreads);
   free(aParams);

   return iret;
}

int64_t fact(int64_t x)
{
   int64_t out = 1;
   for (int64_t i = x; i > 1; --i) {
      out *= i;
   }

   return out;
}

void *ThreadProc(void *ptr){
   THREAD_PARAMS *pParams = (THREAD_PARAMS*)ptr;
   int64_t myLo = 0;
   int64_t myHi = 0;
   int64_t final = 0;
   int64_t stride = 1;

   if (pParams->fBlock) {
      computeMyBlockPart(pParams->numItems, pParams->numTasks, pParams->myTaskId, &myLo, &myHi);
   }
   else{
      computeMyCyclicPart(pParams->numItems, pParams->numTasks, pParams->myTaskId, &myLo, &myHi);
      stride = pParams->numTasks;
   }

   int64_t i;
   if (pParams->fSimple) {
      // Simple Calculation
      for (i = myLo; i < myHi; i += stride) {
         pParams->aData[i] = -pParams->aData[i];
      }
   }
   else{
      // Complex Calculation
      for (i = myLo; i < myHi; i += stride) {
         pParams->aData[i] = fact(pParams->aData[i]);
      }
   }


   //printf("Task: %d \t i: %d \n",pParams->myTaskId, i );
   return NULL;
}

int64_t test(bool fSimple, int64_t cItems, int64_t * aArray)
{
   int64_t minramp = rampValue(cItems,0);
   int64_t maxramp = minramp;
   int64_t last = minramp;


   for (int64_t i = 0; i < cItems; ++i) {
      int64_t initialValue = rampValue(cItems, i);

      if (initialValue < minramp) {
         minramp = initialValue;
      }

      if (initialValue > maxramp) {
         maxramp = initialValue;
      }

      if (initialValue < last) {
         printf("Bad Ramp! index:%d  last:%d cur:%d", i, last, initialValue);
      }

      int64_t expectedValue;
      if (fSimple) {
         expectedValue = -initialValue;
      }
      else {
         expectedValue = fact(initialValue);
      }

      if (aArray[i] != expectedValue) {
         printf("Error! index:%d value:%d expected:%lld\n", i, aArray[i], expectedValue);
      }
   }

   printf("min: %d, max: %d\n", minramp, maxramp);
}