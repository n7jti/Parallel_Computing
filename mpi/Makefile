MPICC=mpicc
CFLAGS+=-std=gnu99 -lm

NPROC?=2
MPIRUN=mpirun
MPIFLAGS+=-np $(NPROC) -host localhost

manual-reduce-mpi: manual-reduce-mpi.c
	$(MPICC) $(CFLAGS) $< -o $@

stencil9-mpi: stencil9-mpi.c
	$(MPICC) $(CFLAGS) $< -o $@

run-reduce: manual-reduce-mpi
	$(MPIRUN) $(MPIFLAGS) ./manual-reduce-mpi $(ARGS)

run-stencil: stencil9-mpi
	$(MPIRUN) $(MPIFLAGS) ./stencil9-mpi $(ARGS)

clean:
	rm -f ./*.o ./stencil9-mpi ./manual-reduce-mpi
