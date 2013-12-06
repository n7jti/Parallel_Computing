use BlockDist;

/* This module contains helper routines that can be used to determine
   the domain of values that a specific locale owns for a
   Block-distributed array. */

proc _array.getMyChunk() {
  return _value.getMyChunk();
}

proc DefaultRectangularArr.getMyChunk() {
  if (this.locale == here) then
    return {(...this.dom.ranges)};
  else
    return {0..-1, 0..-1};
}

proc BlockArr.getMyChunk() {
  return this.myLocArr.locDom.myBlock;
}

