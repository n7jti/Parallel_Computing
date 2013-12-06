config const numTasks = 8;

//
// Each of these reductions will be called collectively by 'numTasks'
// tasks, each of which will pass in its taskID (mytid) from 0..numTasks-1.
//
// The routine should compute the sum reduction of the 'myval'
// arguments sent in by the tasks, returning the result on task 0
// (return values from other tasks will be ignored).  Though it's
// arguably poor form, it's expected that you'll use global variables
// to coordinate between the tasks calling into the routines.
//
// Note that, depending on your approach, one of the challenges in
// this problem will be to "reset" your global state so that
// subsequent calls to the routine will be correct.  If this bogs
// you down too much, separate your reductions into distinct files
// and test separately assuming they can only be called once (are
// not re-usable).
//

var state$: [0..#numTasks] sync int;
var Go$: sync int; // use to reset the global, start empty!

// This proc is used to ensure that all the global state is ready for a run.
// It works because $Go is empty to start with. All threads wait until Go$ 
// is full, except for task 0.  The 0 task resets all the global variables to 
// ensure we are ready to run. Once the initialization is complete, then 
// all the threads are released.
proc start(mytid:int)
{
   //writeln(mytid, " Start Called ");
   if (mytid == 0){

      // This code assumes that Go$ is empty.  
      assert(!Go$.isFull);

      // Clear the globals, and get ready for the run!
      for i in 0..#numTasks
      {
         state$[i].reset();
      }

      
      // Release the Hounds!
      //writeln(mytid, " Releasing Other tasks");
      Go$ = 1;
   }
   else {
      // Wait to start

      /*
      if(!Go$.isFull){
         writeln(mytid, " Spining");
      } 
      */ 

      while(!Go$.isFull)
      {
         //SPIN! Busy Wait
      }
      //writeln(mytid, " Resuming");
   }
   //writeln(mytid, " Started");
}


// The end proc allows task 0 to resets the Go$ variable to get ready for the next run.
proc end(mytid:int)
{
   if (mytid == 0)
   {
      // Empty out the sync variable that controls the clearing of the global
      //writeln("resetting Go$");
      Go$.reset();
   }
}

proc reduction_a(mytid:int, myval:int): int {
  /* Implement part a, reduction using a single variable. */
  start(mytid);

  var mystate: int = myval;

  //writeln(mytid, " Assigning State");
  state$[mytid] = myval;
  
  if (mytid == 0)
  {
     forall i in {1..(numTasks-1)}
     {
        //writeln(i);
        state$[0] += state$[i];
     }

     mystate = state$[0];
  }


  end(mytid);
  return mystate;
}

proc reduction_b(mytid:int, myval:int): int {
  /* Implement part b, reduction with binary tree. Only needs to work
     for numTasks that are power of two */
  start(mytid);

  var mystate: int = myval;

  //writeln(mytid, " Assigning State");
  //state$[mytid] = myval;

  var stride: int = 1;
  while (stride < numTasks)
  {
     if (mytid % (2 * stride) == 0)
     {
        mystate += state$[mytid + stride];
     }

     stride *= 2;
  }

  state$[mytid] = mystate;

  end(mytid);
  //writeln(mytid," Complete");
  return mystate;
}

proc reduction_c(mytid:int, myval:int): int {
  /* Implement part c, reduction with binary tree. Should work for any
     numTasks.  Paste solution to reduction_b here and modify */
    start(mytid);

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

  end(mytid);
  //writeln(mytid," Complete");
  return mystate;
}

proc reduction_d(mytid:int, myval:int, degree:int): int {
  /* NOTE: This part of the homework has been made completely optional
     and will not be graded.  Consider it a fun advanced step if you're
     enjoying the assignment; skip it if not. */

  /* Implement part d, reduction with degree-ary tree. Should work for any
     numTasks.  Paste solution to reduction_d here and modify */
  
  return 0;
}

proc isPowerOf2( x: int ) {
   return (x & (x-1)) == 0;
}

proc test() {
  //
  // This is what you should expect to get as the output:
  //
  writeln("expected result: ", + reduce [i in 1..numTasks] i);

  //
  // Test reduction a
  //
  coforall tid in 0..#numTasks {
    //
    // for simplicity, reduce our task IDs: 1 + 2 + 3 + ... + numTasks
    //
    // the result only needs to be valid for task 0
    //
    const myResult = reduction_a(tid, tid+1);

    // copy result into shared variable
    if (tid == 0) then
      writeln("a: ", myResult);
  }

  //
  // Test reduction b
  //
  if (isPowerOf2(numTasks)) {
    coforall tid in 0..#numTasks {
      const myResult = reduction_b(tid, tid+1);

      // copy result into shared variable
      if (tid == 0) then
	writeln("b: ", myResult);
    }
  } else {
    writeln("b: ", "does not accept non-power-of-2");
  }

  //
  // Test reduction c
  //
  coforall tid in 0..#numTasks {
    const myResult = reduction_c(tid, tid+1);

    // copy result into shared variable
    if (tid == 0) then
      writeln("c: ", myResult);
  }

  //
  // Test reduction d
  //
  for degree in 2..5 {
    coforall tid in 0..#numTasks {
      const myResult = reduction_d(tid, tid+1, degree);

      // copy result into shared variable
      if (tid == 0) then
	writeln("d (",degree,"-ary): ", myResult);    
    }
  }
}

test();
