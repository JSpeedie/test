# gcc flags for includes
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lpthread
# Flags
CFLAGS = -Wall
# Compiler and linker
CC = gcc -ggdb
# CC = gcc

# `compile` first because we want `make` to just compile the program, and the
# default target is always the the first one that doesn't begin with "."
.PHONY: compile
compile: cmp-tree

# Create the cmp-tree object file
cmp-tree.o: cmp-tree.c cmp-tree.h
	$(CC) $(CFLAGS) $(DEBUG) $< -c -o $@

# Create the dynamic array object file
dynamic-array.o: dynamic-array.c dynamic-array.h
	$(CC) $(CFLAGS) $(DEBUG) $< -c -o $@

# Create the better strings object file
better-strings.o: better-strings.c better-strings.h
	$(CC) $(CFLAGS) $(DEBUG) $< -c -o $@

cmp-tree: cmp-tree.o dynamic-array.o better-strings.o
	$(CC) $(CFLAGS) cmp-tree.o dynamic-array.o better-strings.o $(INCS) $(LIBS) -o cmp-tree
