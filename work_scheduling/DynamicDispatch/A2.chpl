module A2 {
   use Time;

   config const Tasks :int = 4;
   config const Items :int = 100000;
   config const MaxRamp :int = 1000;

   var Indx$: sync int;

   proc computeDynamicDispatch(myTask: int):range
   {
      //every time I'm asked, pass out half the remaining items

      var indx: int = Indx$;

      var len: int = Items - indx;
      len  /= 2; // rember, this is integer division
      if (len < 1) {
         len = 1;
      }

      // Never give more than 1/2 of this tasks "fair share".  We want each share to be very likely
      // to come back and get another one. Then, by the time we get to the end, everyone takes smaller
      // tasks
      if (len > Items / Tasks / 2){
         len = Items / Tasks / 2;
      }

      var upper:int = indx + len;

      var ret: range = indx..upper;
      ret = ret [0..#Items];

      Indx$ = upper + 1;
      //writeln("output: ", myTask, " ", indx, " ", upper);
      return ret;
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


   proc main() {
    var items = 0..#Items;
    var A:[items] int;
    var T:[0..#Tasks] int;
    
    Indx$ = 0;   

    for i in items
    {
       A[i] = rampValue(Items,i);
    }
    //writeln(A);

    var t: Timer;
    t.start();
    coforall i in 0..#Tasks
    {
       var myRange : range(stridable=true);

       myRange = computeDynamicDispatch(i);
       
       while (myRange.length > 0)
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
          myRange = computeDynamicDispatch(i);
       }

    }
    t.stop();

    write(Tasks, ", ");
    write("Dynamic, Factorial, Ramp, ");

    writeln(t.elapsed());

    var x:int = 0;
    for i in items
    {
       x += A[i];
    }
    writeln(T);
    //clswriteln("CRC: ", x);

   }
}
