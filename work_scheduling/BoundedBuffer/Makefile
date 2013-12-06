CC=gcc
CFLAGS+=-std=c99 -pthread

CHPL=chpl

BoundedBuffer_chapel: BoundedBuffer.chpl
	$(CHPL) $(CHPL_FLAGS) $< -o $@ 

BoundedBuffer_pthreads: BoundedBuffer.c BoundedBuffer.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ./*.o BoundedBuffer_pthreads BoundedBuffer_chapel
