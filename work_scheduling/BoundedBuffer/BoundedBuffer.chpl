use Time;

config const capacity = 4;
config const P = 2;
config const C = 2;
config const N = 1000;

const TERM = -1.0;

class BoundedBuffer {
    /* FILL IN FIELDS */
    var buffer$: [0..#capacity] sync real;
    var producerIndex$: sync int;
    var consumerIndex$: sync int;
    var cap: int;

    proc BoundedBuffer() {
       producerIndex$ = 0;
       consumerIndex$ = 0;
    }

    proc produce( item : real ) : void {
       var indx: int = producerIndex$;
       producerIndex$ = (indx + 1) % capacity;
       buffer$[indx] = item;
    }

    proc consume( ) : real {
       var indx: int = consumerIndex$;
       consumerIndex$ = (indx + 1) % capacity;
       return buffer$[indx];
    }
}


/* Test the buffer */

// consumer task procedure
proc consumer(b: BoundedBuffer, id: int) {
    // keep consuming until it gets a TERM element
    var count = 0;
    do {
        //writeln(id, " consuming ", count);
        const data = b.consume();
        count += 1;
    } while (data!=TERM);

    return count-1;
}

// producer task procedure
proc producer(b: BoundedBuffer, id: int) {
    // produce my strided share of N elements
    var count = 0;
    for i in 0..#N by P align id {
        //writeln(id, " producing ", i);
        b.produce(i);
        count += 1;
    }

    return count;
}

// produce an element that indicates that the consumer should exit
proc signalExit(b: BoundedBuffer) {
    b.produce(TERM);
}

var P_count: [0..#P] int;
var C_count: [0..#C] int;
var theBuffer = new BoundedBuffer();
var t: Timer;
t.start();
cobegin {
    sync {
        begin {
            // spawn P producers
            coforall prodID in 0..#P do
                P_count[prodID] = producer(theBuffer, prodID);

            // when producers are all done, produce C exit signals
            for consID in 0..#C do
                signalExit(theBuffer);
        }
    }
   
    // spawn C consumers 
    coforall consID in 0..#C do
        C_count[consID] = consumer(theBuffer, consID);
}
t.stop();
writeln(t.elapsed(), " Seconds ", " P_count= ", P_count, " C_count= ", C_count);    

