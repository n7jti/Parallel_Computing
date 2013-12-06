//
// This is a helper module for creating P*M files (a very simple image
// file format.
//
use MPlot;
use Time;
use AdvancedIters;

//
// Dimensions of the image file in pixels
//
config const rows = 1001,
             cols = rows;
config const dynamicDist : bool = false;


//
// Maximum number of steps to iterate before giving up
//
config const maxSteps = 20;


proc main() {
  //
  // TODO: Declare NumSteps to be an array of rows x cols ints to
  // serve as your virtual image map.  You may wish to declare a named
  // domain for it as well.
  //
  var D:domain(2) = {0..#rows,0..#cols};
  var NumSteps: [D] int;
  var countSteps: int;
  var t: Timer;
  t.start();

  //
  // TODO: Call coordToNumSteps() for each index within the NumSteps
  // array to calculate the number of steps required to converge from
  // 0 to maxSteps for that pixel.

  if (dynamicDist == false)
  {
     [(i,j) in D] NumSteps[i,j] = coordToNumSteps((i,j));
  }
  else
  {
     forall i in dynamic(0..#rows, chunkSize=4, numTasks=0){
        for j in {0..#cols}
        {
           NumSteps[i,j] = coordToNumSteps((i,j));
        }

     } 
  }
   

  countSteps = max reduce NumSteps[D];

  t.stop();
  writeln("Elapsed Time: ", t.elapsed());


  //
  // Plot the image
  //
  plot(NumSteps, countSteps);
       /* TODO: add a second argument indicating the largest value in NumSteps,
          in the event that no computations took maxSteps */

}


// =========================================================================
// HELPER FUNCTIONS (should not require modification unless you want to zoom
// in on the Mandelbrot image, change the computation, etc.)
//

//
// Given a coordinate in the space (0..#rows, 0..#cols), compute the
// number of steps required to converge
//
proc coordToNumSteps((x, y)) {
  const c = mapImg2CPlane((x, y));
  
  var z: complex;
  for i in 1..maxSteps {
    z = z*z + c;
    if (abs(z) > 2.0) then
      return i;
  }
  return 0;			
}


//
// Map an image coordinate to a point in the complex plane.
// Image coordinates are (row, col), with row 0 at the top.
//
proc mapImg2CPlane((row, col)) {
  const (rmin, rmax) = (-1.5, .5);
  const (imin, imax) = (-1i, 1i);

  return ((rmax - rmin) * col / cols + rmin) +
         ((imin - imax) * row / rows + imax);
}

