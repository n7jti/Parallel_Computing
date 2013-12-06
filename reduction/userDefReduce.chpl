/* This program demonstrates the ability for a user to create their own
custom reduction function and use it with Chapel's global-view reduce
operator */

use Random, Sort;

//
// seed for random number generator
//
config const seed = SeedGenerator.currentTime;

//
// Problem size 
//
config const n = 10;

//
// Maximum value to store in array
//
config const maxval = 100;

//
// Create a random array of ints; it's a longstanding feature request
// to support this directly in the standard Random module, so here's
// a lame way to work around that lack...
//
var randstr = new RandomStream(seed=seed);
var randReals: [1..n] real;
fillRandom(randReals);
var randInts: [1..n] int = (randReals*maxval):int;

//
// Show the input
//
writeln("Input is: ", randInts);

//
// Invoke our user-defined min-3 reduction (defined below)
//
const smallest3 = min3 reduce randInts;

//
// Show the output
//
writeln("Smallest3 are: ", smallest3);


// Uncomment when you're ready to try your reduction

const mostCommon = mostFrequent reduce randInts;

writeln("Most common element is: ", mostCommon);

//


//
// To define a user-defined reduction, follow the template below.
// The class should be generic w.r.t. eltType (the input element
// type); it can store any state you wish and needs to support
// accumulate(), combine(), and generate() methods with the given
// signatures.
//
class min3: ReduceScanOp {
  //
  // Make this class generic w.r.t. element type
  //
  type eltType;

  //
  // The state type for the reduction -- a buffer of the three smallest
  // values so far; initialize to [max, max, max].  We'll keep these
  // in sorted order to make it easier to reason about.
  //
  var state: [1..3] int = [max(eltType), max(eltType), max(eltType)];

  //
  // accumulate an input value in with the state
  //
  proc accumulate(inputval: eltType) {
    //
    // This is a reasonably lame 3-element sorted 'insert'
    //
    if (inputval < state[3]) {
      if (inputval < state[2]) {
	state[3] = state[2];
	if (inputval < state[1]) {
	  state[2] = state[1];
	  state[1] = inputval;
	} else {
	  state[2] = inputval;
	}
      } else {
	state[3] = inputval;
      }
    } else {
      // inputval is bigger than the 3 smallest we've seen
    }
  }

  //
  // combine two states into one
  //
  proc combine(partner: min3) {
    // A lazy way to do this is just to accumulate each of the three
    // state values together; if we wanted to be more sophisticated,
    // we could do a merge step.
    accumulate(partner.state[1]);
    accumulate(partner.state[2]);
    accumulate(partner.state[3]);
  }

  //
  // convert the state into our output value; in this case, the state
  // is our output value, but in other cases, we may want/need to
  // generate something different.
  //
  proc generate() {
    return state;
  }
}


//
// TODO: Flesh out this class to find the most frequently occuring
// element in the input.
//
class mostFrequent: ReduceScanOp {
  type eltType;

  // TODO: Declare and initialize whatever state you want here
  var state: [0..maxval] eltType;

  proc accumulate(inputval: eltType) {
    // TODO: Accumulate an input element into your state
     state[inputval] += 1;
  }

  proc combine(partner: mostFrequent) {
    // TODO: combine the partner object's state into your state

    //vector Addition should work here
    state += partner.state;
  }

  proc generate() {
    // TODO: return your result based on the current state
     var count: int = -1;
     var indx: int = -1;

     // goes to find one of the elements that happens the least.  
     // the winner should be the lowest numbered one
     forall i in 0..maxval do {
        if(state[i] > count) {
           count = state[i];
           indx = i;
        }
     }
     return indx;
  }
}
