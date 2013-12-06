use Random, Time;  // use standard Random number generation and Timing modules


/* 
   configs: Override their default values on the executable's command
   line using: --<ident>=<val>
*/


//
// This lets you specify the number of random numbers to insert into
// the tree (approximately the number of tree nodes, though we drop
// duplicates).
//
config const numNodes = 100;


//
// This is a seed for the random number generator.  This default value
// will automatically pick a seed based on the microsecond field of
// the current time.  If you'd like to generate the same tree over and
// over again (for performance comparisons), set it to an odd integer
// between 0 and 2**46.
//
config const seed: int = SeedGenerator.currentTime;


//
// This should be used to specify an (approximate) upper bound on the
// number of tasks executing at any given time.  By default, we'll set
// it to the number of cores.
//
config const maxTasks = here.numCores;


//
// The program starts executing here.  Create the tree and search it.
//
proc main() {
  writeln("Creating tree...");
  const (root, findme) = SetupProblem();

  writeln("Starting search...");
  BlindTreeSearch(root, findme);
}


//
// TODO: Your job is to mprove the following routine which searches
// blindly from 'root' for the value 'findme' (currently
// sequentially).
// 
// NOTE: while the random tree created for this program happens to
// have the (sorted) binary search tree property, you should
// ignore this fact and search the tree blindly for the 'findme'
// value, as the serial solution does.  I.e., pretend it does not have
// the binary search tree property.
//
// 1) Modify the code to recursively search the children in parallel.
//
// 2) Avoid creating more tasks than necessary by switching to a
//    sequential search in the event that the number of tasks
//    running (query using 'here.runningTasks()') exceeds the 'maxTasks'
//    config.
//
// 2a) To see how parallelism is used, modify the path to the solution
//     that's built up to use 'L' and 'R' for a parallel search down a
//     left or right child (vs. the 'l' and 'r' used for serial cases).
//
// 3) Use a variable that is shared among the tasks in order to
//    have the tasks return from their searches early if another task 
//    has already found the value in question.
// 
proc BlindTreeSearch(root: TreeNode, findme) {
  //
  // This will store the path to the solution once we find it
  //
  var pathToSolution: string;
  var Done :bool = false;
  //
  // check the starting time
  //
  const startTime = getCurrentTime();

  //
  // start the search and don't proceed ('sync') until all tasks have
  // completed...
  //
  sync SearchNode(root);

  //
  // How long did that take?
  //
  const elapsedTime = getCurrentTime() - startTime;

  //
  // print results
  //
  writeln("Elapsed time was: ", elapsedTime);
  writeln("Path was: ", pathToSolution);

  //
  // This is a nested helper function which permits it to refer
  // directly to variables within BlindTreeSearch() like 'findme'
  // and 'pathToSolution'.
  //
  proc SearchNode(node: TreeNode, path = "") {
    //
    // base case: if we fall off the tree, return
    //
    if (node == nil || Done) then return;

    if (node.data == findme) {
      //
      // we found the data element!  Store the path
      //
      pathToSolution = path;
      Done = true;
    } else {
      //
      // search the left and right children
      //

      if (here.runningTasks() < maxTasks) {
         begin {
            //writeln("begin");
            SearchNode(node.left,  path+"L");
         }
      } else {
         SearchNode(node.left,  path+"l");
      }

      if(here.runningTasks() < maxTasks)
      {
         begin {
            //writeln("begin");
            SearchNode(node.right, path+"R");
         }
      } else {
         SearchNode(node.right, path+"r");
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////
// Everything below this is helper code and shouldn't need to be modified,
// though you may find it helpful/interesting to read (or you may not).
////////////////////////////////////////////////////////////////////////////


//
// A class implementing a binary tree node
//
class TreeNode {
  var id: int(64);             // a unique ID for the node
  var data: real(64);          // its data value
  var left, right: TreeNode;   // its children

  //
  // insert a new tree node ('newnode') as a descendent to 'this'
  // using normal sorted binary tree ordering
  //
  proc insert(newnode: TreeNode) {
    if (newnode.data < data) {    // insert smaller data values to the left
      if (left == nil) then
	left = newnode;
      else
	left.insert(newnode);
    } else if (newnode.data > data) {   // bigger ones to the right
      if (right == nil) then
	right = newnode;
      else
	right.insert(newnode);
    } else {                            // drop duplicates on the floor
      delete newnode;
    }
  }
}


//
// A helper routine that creates and returns the root of the random
// tree; it also returns one of the values in the tree (arbitrarily,
// the one halfway through the stream of random numbers that were
// inserted) to use as the target of the search.
//
proc SetupProblem(): (TreeNode, real(64)) {
  //
  // a stream of random numbers for creating the tree
  //
  const stream = new RandomStream(seed);
  //
  // For lack of imagination, we'll create the tree serially
  //
  const root = new TreeNode(data=stream.getNext(), id=1);
  for i in 2..numNodes {
    const newNode = new TreeNode(data=stream.getNext(), id=i);
    root.insert(newNode);
  }

  //
  // return the root of the tree and (arbitrarily) the random number
  // that was halfway through the values we inserted as the target
  // of our search.
  //
  return (root, stream.getNth(numNodes/2));
}


