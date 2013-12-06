
CHPL=chpl
PARALLEL?=true
LOCALES?=3

CHPL_FLAGS+=-scomputeInParallel=$(PARALLEL)

ARGS+=-nl $(LOCALES)

test: test-2a test-2c

sw: sw-framework.chpl BlockHelp.chpl
	$(CHPL) $(CHPL_FLAGS) $^ -o $@ 

sw-2c: sw-part2c.chpl BlockHelp.chpl
	$(CHPL) $(CHPL_FLAGS) $^ -o $@ 


test-2a: sw
	./sw $(ARGS)

test-2c: sw-2c
	./sw-2c $(ARGS)



clean:
	rm -f ./*.o ./sw ./sw-2c
