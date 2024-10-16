# gcc flags for includes
INCS = -I. -I/usr/include
LIBS = -L/usr/lib
# Flags
CXXFLAGS = -Wall -ggdb3
# Compiler and linker
CXX = g++

# `compile` first because we want `make` to just compile the program, and the
# default target is always the the first one that doesn't begin with "."
compile:
	$(CXX) $(CXXFLAGS) cmp-tree.cpp $(INCS) $(LIBS) -o cmp-tree
