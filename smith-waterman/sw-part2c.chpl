//
// standard modules for IO and the Block Distribution
//
use IO, BlockDist;

//
// A helper module for this assignment that permits a locale to query
// its subset of a Block-distributed array using A.getMyChunk().
//
use BlockHelp;

//
// An enumerated type storing the possible base elements in our
// sequences
//
enum Base {A, C, G, T};

//
// says whether to compute the matrix serially or in parallel.
// Override on the compiler command line using:
//   chpl -scomputeInParallel=true ...
//
config param computeInParallel = false;

//
// the filenames storing the sequences.  Files should be an integer
// length followed by a sequence of A|C|G|T characters.
//
config const seq1file = "seq1.txt";
config const seq2file = "seq2.txt";

//
// configs indicating whether or not to print the matrix and path
// information after the computation has run
//
config const printMatrix = false,
             printPath = false;

//
// This can be useful for debugging distributed arrays
//
config const debugDistributions = false;

//
// the input files
//
const infile1 = open(seq1file, iomode.r).reader();
const infile2 = open(seq2file, iomode.r).reader();

//
// the sequence lengths
//
const seq1len = infile1.read(int),
      seq2len = infile2.read(int);

//
// ranges describing the sequence indices
//
const seq1inds = 1..seq1len,
      seq2inds = 1..seq2len;

//
// non-distributed arrays storing the base pairs
//
var seq1: [seq1inds] Base;
var seq2: [seq2inds] Base;


//
// read the sequences and then compute the matrix
//
proc main() {
  readSequences();

  if (!computeInParallel) then
    computeMatrixSerially();
  else
    computeMatrixInParallel();
}


//
// The completely serial implementation of the algorithm.  This
// is just for reference and used when computeInParallel is false;
// no changes are required here.
//
proc computeMatrixSerially() {
  //
  // Declare a domain for the Matrix, extended by a row/column to
  // store boundary conditions
  //
  var HSpace = {0..seq1len, 0..seq2len};

  //
  // And one for the sequence space which is what we'll compute over
  // and store the path matrix for
  //
  var SeqSpace = HSpace[1.., 1..];

  //
  // H is our value matrix; pathMatrix stores our path back
  //
  var H: [HSpace] int;
  var pathMatrix: [SeqSpace] 2*int;

  //
  // Initialize the boundaries of H (not actually necessary since
  // arrays of integers are initialized to zero by default)
  //
  H[0, ..] = 0;
  H[1.., 0] = 0;

  //
  // Use our helper routine that can compute a chunk of the matrix
  // serially over the entire sequence space.
  //
  computeChunkSerially(H, pathMatrix, SeqSpace);

  //
  // Print the results
  //
  printResults(H, pathMatrix);
}


//
// For the parallel implementation, this tells how many rows we should
// compute in each chunk before signalling the next chunk to start.
// This effectively determines the depth of the pipeline.
//
config const rowsPerChunk = 3;

//
// Here's the parallel implementation.  Parallel in this case means
// "a multi-locale implementation with one task per locale"
//
proc computeMatrixInParallel() {
  //
  // TODO #1: Create a target array of locales to pass into your Block
  // distribution.  Note that in Chapel's standard Block distribution,
  // if no targetLocales argument is provided, it will default to a
  // square-ish reshaping of the Locales array, similar to your
  // 9-point stencil framework in MPI.
  //

  var targetLoc = reshape(Locales,{0..0,0..#numLocales});

  //
  // Declare a local and distributed HSpace domain
  //
  // TODO #2 : uncomment and fill in the dmapped clause to cause
  // HSpace to be a distributed domain such that each locale gets a
  // vertical panel of the matrix.
  //
  var LocHSpace = {0..seq1len, 0..seq2len};
  var HSpace = LocHSpace dmapped Block(boundingBox={0..seq1len, 0..seq2len},targetLoc);

  //
  // Create SeqSpace by slicing HSpace, as before.  If HSpace is
  // distributed, SeqSpace should be as well.
  //
  var SeqSpace = HSpace[1.., 1..];

  //
  // Declare our arrays; if our domains are distributed, they should
  // be too.
  //  
  var H: [HSpace] int;
  var pathMatrix: [SeqSpace] 2*int;

  //
  // This code is useful for making sure your distributed arrays got
  // set up correctly; once you're confident, it's no longer
  // necessary.
  //
  if debugDistributions {
    forall h in H do
      h = here.id;
    writeln("\nH is distributed as follows:\n", H);
    
    forall p in pathMatrix {
      p[1] = here.id;
      p[2] = here.id;
    }
    writeln("\npathMatrix is distributed as follows:\n", pathMatrix);
  }
  
  //
  // Initialize the boundaries of H (not actually necessary since
  // arrays of integers are initialized to zero by default)
  //
  H[0, ..] = 0;
  H[1.., 0] = 0;

  var ChunkSpaceBase = {0..seq1len by rowsPerChunk, 0..#numLocales};
  var ChunkSpace = ChunkSpaceBase dmapped Block(boundingBox = {0..seq1len, 0..#numLocales}, targetLoc);
  var NeighborsDone: [ChunkSpace] atomic int;

  // Populate neighbor count

  forall i in NeighborsDone.domain.dim(1)
  {
     // W Every row in 0 column
     NeighborsDone[i,0].add(1);

     // NW Every row in 0 column
     NeighborsDone[i,0].add(1);
  }

  forall i in NeighborsDone.domain.dim(2)
  {
     // NW Every column in row 0
     NeighborsDone[0,i].add(1);

     // N Every column in row 0
     NeighborsDone[0,i].add(1);
  }

  // and the corner is ready!
  NeighborsDone[0,0].add(1);

  // Start in the upper left corner, on Locale 0
  sync 
  {
     DoThatChunkThing(pathMatrix, 0, H, NeighborsDone);
  }


  //
  // Print the results
  //
  printResults(H, pathMatrix);
}

proc DoThatChunkThing(pathMatrix, startRow, H, NeighborsDone)
{
  const myChunk = pathMatrix.getMyChunk();
  const myCols = myChunk.dim(2);
  const myNextChunk = myChunk[startRow..#rowsPerChunk, myCols];
  computeChunkSerially(H, pathMatrix, myNextChunk);

  // East
  if(here.id < numLocales - 1)
  {
     const eReady = NeighborsDone[startRow, here.id + 1].fetchAdd(1);
     if (eReady == 2)
     {
        on Locales(here.id+1)
        {
           begin DoThatChunkThing(pathMatrix, startRow, H, NeighborsDone);
        }
     }
  }

  // South
  if(startRow < seq1len - rowsPerChunk)
  {
     const sReady = NeighborsDone[startRow + rowsPerChunk, here.id].fetchAdd(1);
     if (sReady == 2)
     {
        begin DoThatChunkThing(pathMatrix, startRow + rowsPerChunk, H, NeighborsDone);
     }
  }

  // SouthEast
  if((here.id < numLocales - 1) && (startRow < seq1len - rowsPerChunk))
  {
     const swReady = NeighborsDone[startRow + rowsPerChunk, here.id + 1].fetchAdd(1);
     if (swReady == 2)
     {
        on Locales(here.id+1)
        {
           begin DoThatChunkThing(pathMatrix, startRow + rowsPerChunk, H, NeighborsDone);
        }
     }
  }

}


//
// This is a helper routine that will compute a given chunk of the H
// and pathMatrix matrices serially.  It relies on serial iteration
// over a multidimensional array being in row-major order (column-
// major would also work).
//
proc computeChunkSerially(H, pathMatrix, chunk) {
  for coord in chunk do
    computePoint(H, pathMatrix, coord);
}



const north = (-1, 0),
      west  = (0, -1),
      nw    = (-1,-1);

//
// The heart of the Smith-Waterman algorithm -- this computes the
// value of 'point' in the H and pathMatrix matrices.
//
proc computePoint(H, pathMatrix, point) {
  const (row,col) = point;
  const w = if (seq1[row] == seq2[col]) then 2 else -1;
  const Hnw = H[point+nw] + w,
        Hnorth = H[point+north] + -1,
        Hwest = H[point+west] + -1;

  if (Hnw > Hnorth && Hnw > Hwest) {
    H[row,col] = Hnw;
    pathMatrix[row,col] = nw;
  } else if (Hnorth > Hwest) {
    H[row,col] = Hnorth;
    pathMatrix[row,col] = north;
  } else {
    H[row,col] = Hwest;
    pathMatrix[row,col] = west;
  }
}


//
// Helper routines to read and print the sequences from the input files
//
proc readSequences() {
  readSequence(seq1, infile1);
  readSequence(seq2, infile2);

  printSequences();
}

proc readSequence(seq, infile) {
  for s in seq do
    infile.read(s);
}

proc printSequences() {
  writeln("Computing the alignment of sequences:");
  for s in seq1 do
    write(s);
  writeln();
  for s in seq2 do
    write(s);
  writeln();
}


//
// Helper routine for printing the results, given the H and pathMatrix
// matrices.
//
proc printResults(H, pathMatrix) {
  if printMatrix then
    writeln("\nMatrix is:\n", H);

  if printPath then
    writeln("\nPath matrix is:\n", pathMatrix);

  const maxPathLen = seq1len+seq2len;
  var path: [1..maxPathLen] 2*int;
  const pathLen = computePath(path, pathMatrix);

  if printPath then
    writeln("\nPath back is:\n", path[1..pathLen]);

  writeln("\nResult is:");
  printSeqWithInsertions(path, pathLen, seq1, 1);
  printSeqWithInsertions(path, pathLen, seq2, 2);
}


//
// Compute the linear path leading back through the path matrix.  Note
// that I didn't handle the case when we run off the side of the
// matrix before reaching the starting vertex.  I'm not certain what
// should happen in that case.  Follow the edge back to the beginning?
//
proc computePath(path, pathMatrix) {
  const start = pathMatrix.domain.high;
  const stop = pathMatrix.domain.low;

  var loc = start;
  for step in (1..) {
    path[step] = loc;
    if (loc == stop) then
      return step;
    loc += pathMatrix[loc];
    if (loc[1] == 0 || loc[2] == 0) then
      halt("We ran off the side of the matrix--Brad didn't handle this case");
  }
  return 0;
}


//
// Given a path of length pathLen, print the sequence with insertions
// marked using '-'.
//
proc printSeqWithInsertions(path, pathLen, seq, seqnum) {
  var prevpos = 0;
  for step in 1..pathLen by -1 {
    const pos = path[step][seqnum];
    if (pos != prevpos) {
      write(seq[pos]);
    } else {
      write("-");
    }
    prevpos = pos;
  }
  writeln();
}



