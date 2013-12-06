#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[]) {
   int numProcs, myProcID;
   MPI_Status status; 

   //
   // Boilerplate MPI startup -- query # processes/images and my unique ID
   //
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myProcID);

   printf("Process %d of %d checking in\n", myProcID, numProcs);

   /* TODO: Implement a reduction to compute the sum of all the proc
      IDs using MPI send/recv routines rather than the built-in MPI
      reduction.  This should essentially be the distributed memory
      rewrite of your previous homework (HW4, Q5c), using messages to
      synchronize between the tasks (processes) rather than
      synchronization variables. */

   /*
    Here's my code from HW5 Q5c
   var mystate: int = myval;
 
   //writeln(mytid, " Assigning State");
   //state$[mytid] = myval;
 
   var stride: int = 1;
   while (stride < numTasks && (mytid + stride < numTasks))
   {
      if (mytid % (2 * stride) == 0)
      {
         mystate += state$[mytid + stride];
      }
 
      stride *= 2;
   }
 
   state$[mytid] = mystate; 
   */

   long long state = myProcID;
   int stride = 1;
   while (stride < numProcs) {
      // If we've got a 1 at stride, then we send and we're done
      // If we've got a 0 at stride, then we recieve and we loop
      if (myProcID & stride) {
         // send and we're done
         MPI_Send(&state, 1, MPI_LONG_LONG, myProcID ^ stride, 0, MPI_COMM_WORLD);
         //printf("Proc %d sent value %d to %d\n", myProcID, state, myProcID ^ stride);
         break;
      } else {
         // we've got a 0 at stride, recieve from the cell with a 1 in this position
         if ((myProcID | stride) < numProcs) {
            long long other;
            MPI_Recv(&other, 1, MPI_LONG_LONG, myProcID | stride, 0, MPI_COMM_WORLD, &status);
            state += other;
            //printf("Proc %d recv value %d from %d and now has total %d\n", myProcID, other, myProcID | stride, state);
         }
      }

      stride *= 2;
   }

   if (myProcID == 0) {
      printf("Process %d has total %d\n", myProcID, state);
   }

   MPI_Finalize();
   return 0;
}
