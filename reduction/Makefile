# You may add targets to this Makefile,
# but do not change the orignal ones.
# Your submission must buildable with the
# original targets.

# // Example:
# make reduction
#
# // Example with --fast:
# make CHPL_FLAGS=--fast reduction

CHPL=chpl


# do not modify this target
reduction: reduction.chpl
	$(CHPL) $(CHPL_FLAGS) $< -o $@ 

# do not modify this target
userDefReduce: userDefReduce.chpl
	$(CHPL) $(CHPL_FLAGS) $< -o $@ 

clean:
	rm -f ./reduction ./userDefReduce
