#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"


//
// The logical *global* problem size -- N x N elements; each process
// will own a fraction of the whole.
//
#ifndef N
//#define N 10
#define N 1000
#endif

#define MESSAGE_BASE 10000
#define MESSAGE_END_OF_ROW (MESSAGE_BASE + 1)
#define MESSAGE_END_OF_GRID_ROW (MESSAGE_BASE + 2)
#define MESSAGE_SEND_TOP (MESSAGE_BASE + 3)
#define MESSAGE_SEND_BOTTOM (MESSAGE_BASE + 4)
#define MESSAGE_SEND_LEFT (MESSAGE_BASE + 5)
#define MESSAGE_SEND_RIGHT (MESSAGE_BASE + 6)
#define MESSAGE_SEND_DOWN_LEFT (MESSAGE_BASE + 7)
#define MESSAGE_SEND_DOWN_RIGHT (MESSAGE_BASE + 8)
#define MESSAGE_SEND_UP_LEFT (MESSAGE_BASE + 9)
#define MESSAGE_SEND_UP_RIGHT (MESSAGE_BASE + 10)

//
// We'll terminate when the difference between all elements in
// adjacent iterations is less than this value of epsilon.
//
//#define epsilon .01
#define epsilon .000001

// START OF PROVIDED ROUTINES (should not need to change)
// ------------------------------------------------------------------------------

//
// This routine is a really lame way of computing a square-ish grid of
// processes, favoring more columns than rows.  It returns the results
// via numRows and numCols.
//
void computeGridSize(int numProcs, int *numRows, int *numCols) {
   int guess = sqrt(numProcs);
   while (numProcs % guess != 0) {
      guess--;
   }
   *numRows = numProcs / guess;
   *numCols = guess;
}


//
// This routine calculates a given process's location within a virtual
// numRows x numCols grid, laying them out in row major order.
//
//
void computeGridPos(int me, int numRows, int numCols, int *myRow, int *myCol) {
   *myRow = me / numCols;
   *myCol = me % numCols;
}

// END OF PROVIDED ROUTINES (should not need to change)
// ------------------------------------------------------------------------------

void computeMyRange(
   int64_t numItems,
   int64_t numTasks,
   int64_t myTaskId,
   int64_t *pMyLoBound,
   int64_t *pMySize) {
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

   *pMyLoBound = lo;
   *pMySize = size;
}

int inMyGrid(int globalRow, int globalCol, int mySourceRow, int mySourceRowSize, int mySourceCol, int mySourceColSize) {
   if (globalRow >= mySourceRow && globalRow <= mySourceRow + mySourceRowSize) {
      if (globalCol >= mySourceCol && globalCol <= mySourceCol + mySourceColSize) {
         return 1;
      }
   }
   return 0;
}

inline int globalToLocal(int global, int source) {
   return global - source;
}

int isFirstProcInRow(int myCol, int cols) {
   int ret = 0;
   if (myCol % cols == 0) {
      ret = 1;
   }

   return ret;
}

int isLastProcInRow(int myCol, int cols) {
   int ret = 0;
   if (myCol % cols == cols - 1) {
      ret = 1;
   }
   return ret;
}

int getFirstProcInRow(int row,  int cols) {
   return row * cols;
}

int getLastProcInRow(int row, int cols) {
   return (row + 1) * cols - 1;
}

int getUpID(int myProcID, int numCols) {
   int retval = myProcID - numCols;
   return retval < 0 ? -1 : retval;
}

int getDownID(int myProcID, int numProcs,  int numCols) {
   int retval = myProcID + numCols;
   return retval >= numProcs ? -1 : retval;
}

int getLeftID(int myProcID, int numCols) {
   return myProcID % numCols == 0 ? -1 : myProcID - 1;
}

int getRightID(int myProcID, int numProcs, int numCols) {
   return myProcID % numCols == numCols - 1 ? -1 : myProcID + 1;
}

int getUpLeftID(int myProcID, int numCols) {
   int id = getUpID(myProcID, numCols);
   if (id >= 0) {
      id = getLeftID(id,numCols);
   }
   return id;
}

int getUpRightID(int myProcID, int numProcs, int numCols) {
   int id = getUpID(myProcID,numCols);
   if (id >= 0) {
      id = getRightID(id,numProcs,numCols);
   }
   return id;
}

int getDownLeftID(int myProcID, int numProcs,  int numCols) {
   int id = getDownID(myProcID,numProcs,numCols);
   if (id >= 0) {
      id = getLeftID(id,numCols);
   }
   return id;
}

int getDownRightID(int myProcID, int numProcs,  int numCols) {
   int id = getDownID(myProcID,numProcs,numCols);
   if (id >= 0) {
      id = getRightID(id,numProcs,numCols);
   }
   return id;
}


void outputArray(double **X,
                 int myProcID,
                 int myRow,
                 int myCol,
                 int mySourceRowSize,
                 int mySourceColSize,
                 int numRows,
                 int numCols,
                 int numProcs) {
   MPI_Status status;
   int buffer = 0;
   FILE *f = NULL;
   char fname[] = "stencil9-mpi-output";

   if (myProcID == 0) {
      //Delete the old output file
      remove(fname);
   }

   // Make sure nobody writes to the file until Proc0 has had a chance to delete it
   MPI_Barrier(MPI_COMM_WORLD);

   if (myRow > 0 && isFirstProcInRow(myCol, numCols)) {
      // Wait for previous row to complete, unless we are the first!
      MPI_Recv(&buffer, 1, MPI_LONG, myProcID - 1, MESSAGE_END_OF_GRID_ROW, MPI_COMM_WORLD, &status);
   }

   // Go row by row in each grid printing out a line.
   for (int i = 1; i <= mySourceRowSize; ++i) {

      if (!isFirstProcInRow(myCol, numCols)) {
         //printf("Process %d: Waiting for MESSAGE_END_OF_ROW from %d\n", myProcID, myProcID - 1);
         MPI_Recv(&buffer, 1, MPI_LONG, myProcID - 1, MESSAGE_END_OF_ROW, MPI_COMM_WORLD, &status);
      } else {
         // First Proc in the row
         if (!isLastProcInRow(myCol, numCols)) {
            // First proc in the row
            if (i != 1) {
               //printf("Process %d: Waiting for MESSAGE_END_OF_ROW from %d\n", myProcID, getLastProcInRow(myRow,numCols));
               MPI_Recv(&buffer, 1, MPI_LONG, getLastProcInRow(myRow, numCols), MESSAGE_END_OF_ROW, MPI_COMM_WORLD, &status);
            }
         }
      }

      f = fopen("stencil9-mpi-output", "a");

      for (int j = 1; j <= mySourceColSize; ++j) {
         //fprintf(f, "%d, %d, %d, %f ", myProcID, i, j, X[i][j]);
         fprintf(f,"%f ",X[i][j]);
      }

      if (isLastProcInRow(myCol, numCols)) {
         // End Of Block Row
         //printf(" END OF BLOCK ROW ");
         fprintf(f, "\n");
      }


      fflush(f);
      fclose(f);


      if (isLastProcInRow(myCol,numCols)) {
         if ((!isFirstProcInRow(myCol, numCols)) && (i <mySourceRowSize)) {
            // if we're the last in the row, then signal the front of the row for all rows but the last
            //printf("Process %d: %d notifying %d MESSAGE_END_OF_ROW\n", myProcID, i, getFirstProcInRow(myRow,numCols));
            MPI_Send(&buffer, 1, MPI_LONG, getFirstProcInRow(myRow, numCols), MESSAGE_END_OF_ROW, MPI_COMM_WORLD);
         }
      }
      else
      {
            // if we're not the lst in the row, then signal the next process
            //printf("Process %d: %d notifying %d MESSAGE_END_OF_ROW\n", myProcID, i, myProcID + 1);
            MPI_Send(&buffer, 1, MPI_LONG, myProcID + 1, MESSAGE_END_OF_ROW, MPI_COMM_WORLD);
      }

   }

   if (isLastProcInRow(myCol, numCols) && myProcID < numProcs - 1) {
      //Message the END_OF_GRID_ROW
      //printf("Process %d: notifying %d MESSAGE_END_OF_GRID_ROW\n", myProcID, myProcID + 1);
      MPI_Send(&buffer, 1, MPI_LONG, myProcID + 1, MESSAGE_END_OF_GRID_ROW, MPI_COMM_WORLD);
   }

   // gaurentee that we're all done!
   MPI_Barrier(MPI_COMM_WORLD);

   if (0 == myProcID) {
      f = fopen(fname, "r");
      char ch;
      ch = getc(f);
      while (ch != EOF) {
         putchar(ch);
         ch = getc(f);
      }
      fclose(f);
      printf("\n");
   }

}


void testArray(double **X,
               int mySourceRow,
               int mySourceRowSize,
               int mySourceCol,
               int mySourceColSize,
               int numRows,
               int numCols) {
   //printf("numRows: %d numCols: %d", numRows, numCols);
   for (int i = 1; i <= N; ++i) {
      for (int j = 1; j <= N; ++j) {
         //printf("%d , %d\n",i,j);
         if (inMyGrid(i, j, mySourceRow, mySourceRowSize, mySourceCol, mySourceColSize))  {
            //printf("InMyGrid %d , %d\n",i,j);
            X[globalToLocal(i, mySourceRow)][globalToLocal(j, mySourceCol)] = 100.0 * i + j;
         }
      }
   }
}

int main(int argc, char *argv[]) {
   int numProcs, myProcID;
   int numRows, numCols;
   int myRow, myCol;
   MPI_Status status;

   //
   // Boilerplate MPI startup -- query # processes/images and my unique ID
   //
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myProcID);

   //
   // Arrange the numProcs processes into a virtual 2D grid (numRows x
   // numCols) and compute my logical position within it (myRow,
   // myCol).
   //
   computeGridSize(numProcs, &numRows, &numCols);
   computeGridPos(myProcID, numRows, numCols, &myRow, &myCol);

   //
   // Sanity check that we're up and running correctly.  Feel free to
   // disable this once you get things running.
   //
   //printf("Process %d of %d checking in\n"
   //       "I am at (%d, %d) of %d x %d processes\n\n", myProcID, numProcs,
   //       myRow, myCol, numRows, numCols);


   /* TODO (step 1): Using your block distribution (or a
      corrected/improved/evolved version of it) from assignment #1,
      compute the portion of the global N x N array that this task
      owns, using a block x block distribution */

   // Compute Rows
   int64_t mySourceRow;
   int64_t mySourceRowSize;
   computeMyRange(N, numRows, myRow, &mySourceRow, &mySourceRowSize);


   // Compute Cols
   int64_t mySourceCol;
   int64_t mySourceColSize;
   computeMyRange(N, numCols, myCol, &mySourceCol, &mySourceColSize);

   //printf("Process %d I have rows %d-%d and cols %d-%d\n\n",
   //       myProcID,
   //       mySourceRow,
   //       mySourceRow + mySourceRowSize-1,
   //       mySourceCol,
   //       mySourceCol + mySourceColSize-1);

   /* TODO (step 2): Allocate arrays corresponding to the local portion
      of data owned by this process -- in particular, don't allocate an
      O(N**2) array on each process, only the portion it owns.
      Allocate an extra row/column of data as a halo around the array
      to store global boundary conditions and/or overlap regions/ghost
      cells for caching neighboring processors' values, similar to what
      was shown for the 1D 3-point stencil in class, simply in 2D. */

   //printf("Process %d: Allocating X\n",myProcID);
   double **X; // the X array
   X = (double **)malloc(sizeof(double *) * (mySourceRowSize + 2));
   for (int i = 0; i < mySourceRowSize + 2; ++i) {
      X[i] = (double *)malloc(sizeof(double) * (mySourceColSize + 2));
   }

   //printf("Process %d: Allocating Y\n",myProcID);
   double **Y; // the Y array
   Y = (double **)malloc(sizeof(double *) * (mySourceRowSize + 2));
   for (int i = 0; i < mySourceRowSize + 2; ++i) {
      Y[i] = (double *)malloc(sizeof(double) * (mySourceColSize + 2));
   }

   /* TODO (step 3): Initialize the arrays to zero. */
   // printf("Process %d: Initializing Arrays to ZERO\n",myProcID);
   for (int i = 0; i < mySourceRowSize + 2; ++i) {
      for (int j = 0; j < mySourceColSize + 2; ++j) {
         X[i][j] = 0.0;
         Y[i][j] = 0.0;
      }
   }

   /* TODO (step 4): Initialize the arrays to contain four +/-1.0
      values, as in assignment #5.  Note that you will need to do a
      global -> local index calculation to determine (a) which
      process(es) owns the points and (b) which array value the points
      correspond to. */

   //
   // Place a nonzero entry in the center of each quadrant.
   //

   if (inMyGrid(N / 4 + 1, N / 4 + 1, mySourceRow, mySourceRowSize, mySourceCol, mySourceColSize))  {
      //A[N/4+1][N/4+1] = 1.0;
      X[globalToLocal(N / 4 + 1, mySourceRow)][globalToLocal(N / 4 + 1, mySourceCol)] = 1.0;
   }

   if (inMyGrid(3 * N / 4 + 1, 3 * N / 4 + 1, mySourceRow, mySourceRowSize, mySourceCol, mySourceColSize)) {
      //A[3*N/4+1][3*N/4+1] = 1.0;
      X[globalToLocal(3 * N / 4 + 1, mySourceRow)][globalToLocal(3 * N / 4 + 1, mySourceCol)] = 1.0;
   }

   if (inMyGrid(N / 4 + 1, 3 * N / 4 + 1, mySourceRow, mySourceRowSize, mySourceCol, mySourceColSize)) {
      //A[N/4+1][3*N/4+1] = -1.0;
      X[globalToLocal(N / 4 + 1, mySourceRow)][globalToLocal(3 * N / 4 + 1, mySourceCol)] = -1.0;
   }

   if (inMyGrid(3 * N / 4 + 1, N / 4 + 1, mySourceRow, mySourceRowSize, mySourceCol, mySourceColSize)) {
      //[3*N/4+1][N/4+1] = -1.0;
      X[globalToLocal(3 * N / 4 + 1, mySourceRow)][globalToLocal(N / 4 + 1, mySourceCol)] = -1.0;
   }

   /* TODO (step 5): Implement a routine to sequentially print out the
      distributed array to the console in a coordinated manner such
      that it appears as a global whole, as we logically think of it.
      In other words, the output of this routine should be identical to
      that of printArr() in assignment #5, in spite of the fact that
      the array is decomposed across a number of processes. Use
      Send/Recv calls to coordinate between the processes.  Use this
      routine to verify that your initialization is correct.
   */

   int buffer = 0;
   fflush(stdout);

   // testArray(X, mySourceRow,mySourceRowSize,mySourceCol,mySourceColSize,numRows,numCols);

   //outputArray(X, myProcID, myRow, myCol, mySourceRowSize, mySourceColSize, numRows, numCols, numProcs);

   double globalEpsilon = 0.0;
   int iterations = 0;
   do {
      /* TODO (step 6): Implement the 9-point stencil using ISend/IRecv
         and Wait routines.  Use the non-blocking routines in order to get
         all the communication up and running in a safe manner.  While it
         is possible to compute on the innermost elements of the array
         before the communication completes, there is no reason to do so
         -- simply use the non-blocking calls as a means of getting a
         number of communications up and running without waiting for
         others to complete. */



      int procUpID = getUpID(myProcID, numCols);
      MPI_Request upSendRequest;
      MPI_Request upRecvRequest;
      int procDownID = getDownID(myProcID, numProcs, numCols);
      MPI_Request downSendRequest;
      MPI_Request downRecvRequest;


      // Up & Down
      //printf("Process %d: procUpID = %d procDownID = %d\n", myProcID, procUpID, procDownID);
      if (procUpID >= 0) {
         MPI_Isend(&X[1][1], mySourceColSize, MPI_DOUBLE, procUpID, MESSAGE_SEND_TOP, MPI_COMM_WORLD, &upSendRequest);
         MPI_Irecv(&X[0][1], mySourceColSize, MPI_DOUBLE, procUpID, MESSAGE_SEND_BOTTOM, MPI_COMM_WORLD, &upRecvRequest);
      }

      if (procDownID >= 0) {
         MPI_Isend(&X[mySourceRowSize][1], mySourceColSize, MPI_DOUBLE, procDownID, MESSAGE_SEND_BOTTOM, MPI_COMM_WORLD, &downSendRequest);
         MPI_Irecv(&X[mySourceRowSize + 1][1], mySourceColSize, MPI_DOUBLE, procDownID, MESSAGE_SEND_TOP, MPI_COMM_WORLD, &downRecvRequest);
      }

      if (procUpID >= 0) {
         MPI_Wait(&upSendRequest, &status);
         MPI_Wait(&upRecvRequest, &status);
      }

      if (procDownID >= 0) {
         MPI_Wait(&downSendRequest, &status);
         MPI_Wait(&downRecvRequest, &status);
      }

      // Left & Right

      int procLeftID = getLeftID(myProcID, numCols);
      int procRightID = getRightID(myProcID, numProcs, numCols);
      //printf("Process %d: LeftID = %d rightID = %d\n", myProcID, procLeftID, procRightID);
      MPI_Request leftSendRequest;
      MPI_Request leftRecvRequest;
      MPI_Request rightSendRequest;
      MPI_Request rightRecvRequest;

      for (int i = 1; i < mySourceRowSize + 2; ++i) {
         if (procLeftID >= 0) {
            MPI_Isend(&X[i][1], 1, MPI_DOUBLE, procLeftID, MESSAGE_SEND_LEFT, MPI_COMM_WORLD, &leftSendRequest);
            MPI_Irecv(&X[i][0], 1, MPI_DOUBLE, procLeftID, MESSAGE_SEND_RIGHT, MPI_COMM_WORLD, &leftRecvRequest);
         }

         if (procRightID >= 0) {
            MPI_Isend(&X[i][mySourceColSize], 1, MPI_DOUBLE, procRightID, MESSAGE_SEND_RIGHT, MPI_COMM_WORLD, &rightSendRequest);
            MPI_Irecv(&X[i][mySourceColSize + 1], 1, MPI_DOUBLE, procRightID, MESSAGE_SEND_LEFT, MPI_COMM_WORLD, &rightRecvRequest);
         }

         if (procLeftID >= 0) {
            MPI_Wait(&leftSendRequest, &status);
            MPI_Wait(&leftRecvRequest, &status);
         }

         if (procRightID >= 0) {
            MPI_Wait(&rightSendRequest, &status);
            MPI_Wait(&rightRecvRequest, &status);
         }
      }

      // Down Left & Up Right Down Right & Up Left
      int procDownLeftID = getDownLeftID(myProcID, numProcs, numCols);
      int procDownRightID = getDownRightID(myProcID, numProcs, numCols);
      int procUpLeftID   = getUpLeftID(myProcID, numCols);
      int procUpRightID  = getUpRightID(myProcID, numProcs, numCols);

      //printf("Process %d: Down Left: %d Down Right: %d up Left = %d Up Right = %d\n", myProcID, procDownLeftID, procDownRightID, procUpLeftID, procUpRightID);
      MPI_Request downLeftSendRequest;
      MPI_Request downLeftRecvRequest;
      MPI_Request upLeftSendRequest;
      MPI_Request upLeftRecvRequest;
      MPI_Request downRightSendRequest;
      MPI_Request downRightRecvRequest;
      MPI_Request upRightSendRequest;
      MPI_Request upRightRecvRequest;

      if (procDownLeftID >= 0) {
         //printf("Process: %d, send/recv down left\n", myProcID);
         MPI_Isend(&X[mySourceRowSize][1], 1, MPI_DOUBLE, procDownLeftID, MESSAGE_SEND_DOWN_LEFT, MPI_COMM_WORLD, &downLeftSendRequest);
         MPI_Irecv(&X[mySourceRowSize + 1][0], 1, MPI_DOUBLE, procDownLeftID, MESSAGE_SEND_UP_RIGHT, MPI_COMM_WORLD, &downLeftRecvRequest);
      }

      if (procDownRightID >= 0) {
         //printf("Process: %d, send/recv down right\n", myProcID);
         MPI_Isend(&X[mySourceRowSize][mySourceColSize], 1, MPI_DOUBLE, procDownRightID, MESSAGE_SEND_DOWN_RIGHT, MPI_COMM_WORLD, &downRightSendRequest);
         MPI_Irecv(&X[mySourceRowSize + 1][mySourceColSize + 1], 1, MPI_DOUBLE, procDownRightID, MESSAGE_SEND_UP_LEFT, MPI_COMM_WORLD, &downRightRecvRequest);
      }

      if (procUpLeftID >= 0) {
         //printf("Process: %d, send/recv up left\n", myProcID);
         MPI_Isend(&X[1][1], 1, MPI_DOUBLE, procUpLeftID, MESSAGE_SEND_UP_LEFT, MPI_COMM_WORLD, &upLeftSendRequest);
         MPI_Irecv(&X[0][0], 1, MPI_DOUBLE, procUpLeftID, MESSAGE_SEND_DOWN_RIGHT, MPI_COMM_WORLD, &upLeftRecvRequest);
      }

      if (procUpRightID >= 0) {
         //printf("Process: %d, send/recv up right\n", myProcID);
         MPI_Isend(&X[1][mySourceColSize], 1, MPI_DOUBLE, procUpRightID, MESSAGE_SEND_UP_RIGHT, MPI_COMM_WORLD, &upRightSendRequest);
         MPI_Irecv(&X[0][mySourceColSize+1], 1, MPI_DOUBLE, procUpRightID, MESSAGE_SEND_DOWN_LEFT, MPI_COMM_WORLD, &upRightRecvRequest);
      }

      if (procDownLeftID >= 0) {
         //printf("Process: %d, wait down left\n", myProcID);
         MPI_Wait(&downLeftSendRequest, &status);
         MPI_Wait(&downLeftRecvRequest, &status);
      }

      if (procDownRightID >= 0) {
         //printf("Process: %d, wait down right\n", myProcID);
         MPI_Wait(&downRightSendRequest, &status);
         MPI_Wait(&downRightRecvRequest, &status);
      }

      if (procUpLeftID >= 0) {
         //printf("Process: %d, wait up left\n", myProcID);
         MPI_Wait(&upLeftSendRequest, &status);
         MPI_Wait(&upLeftRecvRequest, &status);
      }

      if (procUpRightID >= 0) {
         //printf("Process: %d, wait up right\n", myProcID);
         MPI_Wait(&upRightSendRequest, &status);
         MPI_Wait(&upRightRecvRequest, &status);
      }

      // Now compute the stencil
      for (int i = 1; i < mySourceRowSize+1; ++i) {
         for (int j = 1; j < mySourceColSize+1; ++j) {
            double center = X[i][j] * 0.25;
            double adjacent = (X[i-1][j] + X[i+1][j] + X[i][j-1] + X[i][j+1]) * 0.125;
            double diagonals = (X[i-1][j-1] + X[i+1][j-1] + X[i-1][j+1] + X[i+1][j+1]) * 0.0625;

            Y[i][j] = 
               (center + adjacent + diagonals);
         }
      }

      //outputArray(Y,myProcID,myRow,myCol,mySourceRowSize,mySourceColSize,numRows,numCols,numProcs);


      /* TODO (step 7): Verify that the stencil seems to be progressing
         correctly, as in assignment #5. */

      //Done!


      /* TODO (step 8): Use an MPI reduction to compute the termination of
         the routine, as in assignment #5. */
      double localEpsilon = 0.0;
      for (int i = 1; i < mySourceRowSize+1; ++i) {
         for (int j = 1; j< mySourceColSize+1; ++j) {
            double le = fabs(Y[i][j] - X[i][j]);
            if (le > localEpsilon) {
               localEpsilon = le;
            }
         }
      }

      MPI_Allreduce(&localEpsilon, &globalEpsilon, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

      //Copy Y to X
      for (int i = 1; i < mySourceRowSize+1; ++i) {
         for (int j = 1; j< mySourceColSize+1; ++j) {
            X[i][j] = Y[i][j];
         }
      }

      ++iterations;
   }
   while (globalEpsilon > epsilon); 
   /* TODO (step 9): Verify that the results of the computation (output
      array, number of iterations) are the same as assignment #5 for a
      few different problem sizes and numbers of processors; be sure to
      test a case in which there are interior processes (e.g., 9, 12,
      16, ... processes...) */

   //printf("Process %d: Done! \n",myProcID);
   if (myProcID == 0) {
      printf("%d iterations\n", iterations);
   }

   MPI_Finalize();
return 0;
}


