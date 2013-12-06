module A2 {
    use Random;
    use Time;

    config const Tasks :int = 4;
    config const Items :int = 100000;
    config const MaxRamp :int = 1000;
    config const fRamp : bool = true; 
    config const fBlock : bool = true;
    config const fInverse : bool = false;


    var rnd : RandomStream;

    proc computeMyBlockPart(items: range,
                         numTasks: int,
                         myTask: int
                        ):range
    {
       assert(myTask < numTasks);
        // compute the block
        var size = items.length / numTasks;
        var remainder = items.length % numTasks;

        if (myTask < remainder)
        {
            size += 1;
        }

        var lo = size * myTask;
        if (myTask >= remainder) {
          lo += remainder;
        }

        return lo..(lo+size-1);
  }

  proc computeMyCyclePart(items: range,
                         numTasks: int,
                         myTask: int
                        ):range(stridable=true)
  {
      return items by numTasks align myTask;
  }

  proc rampValue(cItems: int, indx: int): int
  {
     var ret: int;
     var numItems: real = cItems;
     var value = (indx / numItems) * MaxRamp;
     //writeln(value);
     ret = floor(value) : int;
     return ret + 1;
  }

  proc randValue(cItems :int, indx :int) : int
  {
     var ret: int;
     var rand: real;
     rand = rnd.getNext();
     //writeln(rand);
     ret = (rand * MaxRamp) :int;
     ret = ret % MaxRamp;
     return ret;
  }


  proc main() {
    var items = 0..#Items;
    var A:[items] int;
    var T:[0..#Tasks] int;   

    rnd = new RandomStream(SeedGenerator.currentTime); 

    if(fRamp == true)
    {
       for i in items
       {
          A[i] = rampValue(Items,i);
       }
    }
    else
    {
       for i in items
       {
          A[i] = randValue(Items,i);
       }
    }
    //writeln(A);
    
    var t: Timer;
    t.start();
    coforall i in 0..#Tasks
    {
       var myRange : range(stridable=true);
       if(fBlock)
       {
          myRange = computeMyBlockPart(items, Tasks, i);
       }
       else
       {
          myRange = computeMyCyclePart(items, Tasks, i);
       }

       if(fInverse)
       {
          // negate the value
          for j in myRange
          {
             A[j] = -A[j];
             T[i] += 1;
          }
       }
       else
       {
          // compute the factorial
          for j in myRange
          {
             var fact :int = 1;
             for k in 2..A[j]
             {
                fact *= k;
             }
             A[j] = fact;
             T[i] += 1;
          }
       }


    }
    t.stop();

    //1	100000000	 block	 negation	 random	0.380497914


    write(Tasks, ", ");

    if(fBlock)
    {
       write("Block, ");
    }
    else
    {
       write("Cyclic, ");
    }

    if(fInverse)
    {
       write("Inverse, ");
    }
    else
    {
       write("Factorial, ");
    }

    if(fRamp)
    {
       write("Ramp, ");
    }
    else
    {
       write("Random, ");
    }

    writeln(t.elapsed());

    var x:int = 0;
    for i in items
    {
       x += A[i];
    }

    writeln(T);
    //writeln("CRC: ", x);

  }
}
