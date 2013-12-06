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

  //
  // TODO #3: Declare a distributed array, ChunkReady$, of sync vars
  // that will be used to signal when locale x can compute its block
  // starting at row r.  For example, the convention could be that
  // when element ChunkReady$[r,x] is full, locale x can start
  // computing its chunk starting at row r.  ChunkReady$ should be
  // distributed such that elements [..,x] should be stored on locale
  // x.
  //
  // One important note: the boundingBox argument to the Block
  // distribution must currently be non-strided; for this reason you
  // may need to use a non-strided array for the bounding box and
  // a strided one for the domain itself.
  //

  var ChunkSpaceBase = {0..seq1len by rowsPerChunk, 0..#numLocales};
  var ChunkSpace = ChunkSpaceBase dmapped Block(boundingBox = {0..seq1len, 0..#numLocales}, targetLoc);
  var ChunkReady$: [ChunkSpace] sync int;

  //
  // Here's another debugging routine that you can enable once you've
  // declared ChunkReady$ to verify how it was distributed.  This code
  // is safe because the syncs start empty, it fills them all, then
  // reads them all, leaving them empty again.
  //

  
  if debugDistributions {
    writeln("\nChunkReady$.domain.dim(1) ", ChunkReady$.domain.dim(1));
    writeln("ChunkReady$.domain.dim(2) ", ChunkReady$.domain.dim(2));
    forall b in ChunkReady$ do
      b = here.id;
    writeln("\nChunkReady$ is distributed as follows:\n", ChunkReady$);
  }
  

  //
  // TODO #4:
  //
  // Initialize ChunkReady$ such that locale 0 is ready to start any
  // of its blocks whenever it wants; they depend only on the boundary
  // conditions which are ready to go.

  forall i in ChunkReady$.domain.dim(1)
  {
     ChunkReady$[i,0] = 1;
  }
  
  //
  // TODO #5: Create a task per locale running on that locale
  //

  coforall loc in Locales do
  {
    on loc do{
      //
      // Compute the chunk of the distributed pathMatrix that we own
      // and the columns of that chunk.  These use a helper routine,
      // getMyChunk(), provided in the BlockHelp.chpl module which
      // only works when invoked from one of the locales to which
      // pathMatrix is distributed.  It will return the 2D domain
      // describing that locale's subdomain of pathMatrix.
      //
      const myChunk = pathMatrix.getMyChunk();
      const myCols = myChunk.dim(2);

      // TODO #6: Set up the appropriate control flow to iterate over
      // our rows a chunk at a time when it is safe to do so as
      // indicated by ChunkReady$.

      /*
      if debugDistributions
      {
         writeln( "\n", here.id, " myChunk.dim(1) = ", myChunk.dim(1) );
         writeln( "\n", here.id, " myChunk.dim(2) = ", myChunk.dim(2) );
      }
      */
        
      for row in ChunkReady$.domain.dim(1)
      {
        // block until our chunk is ready!
        var ready = ChunkReady$[row,here.id];

        // TODO #7: 
        // Compute the indices of the next chunk that this particular
        // task can safely compute serially at this time.  (NOTE: The
        // assignment of 'myChunk' here is just a placeholder
        // assignment, for the sake of making the framework compile
        // without modifications.  Replace it with a correct
        // description of this task's next chunk).
        //
        //const myNextChunk = myChunk;
        const myNextChunk = myChunk[row..#rowsPerChunk, myCols];

        //
        // This is just a debug print -- remove it once you're up
        // and running
        //
        if (debugDistributions)
        {
           writeln("Locale ", here.id, " computing ", myNextChunk);
        }

        //
        // Compute the matrix corresponding to my next chunk serially
        //
        computeChunkSerially(H, pathMatrix, myNextChunk);

        //
        // TODO #8:
        // Signal that the next locale can now start at this same row
        // since its chunk was dependent on ours.
        // And that should be it!
        //

        // East
        if(here.id < numLocales - 1)
        {
           ChunkReady$[row,here.id +1 ] = 1;
        }
      }
    }
  }
  // ...join tasks...

  //
  // Print the results
  //
  printResults(H, pathMatrix);
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



