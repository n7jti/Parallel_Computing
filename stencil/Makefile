CC=gcc
CFLAGS+=-std=gnu99 -fopenmp -lm -g -lrt

CHPL=chpl

mandelbrot_chapel: mandelbrot.chpl MPlot.chpl
	$(CHPL) $(CHPL_FLAGS) $^ -o $@ 

stencil9: stencil9.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ./*.o mandelbrot_chapel mandelbrot_c stencil9
