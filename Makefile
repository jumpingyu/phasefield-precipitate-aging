# Makefile
# GNU makefile for Ni-base superalloy decomposition
# Questions/comments to trevor.keller@nist.gov (Trevor Keller)

# includes
incdir = $(MMSP_PATH)/include

# compilers/flags
compiler = icc
pcompiler = /usr/bin/mpic++.openmpi

#flags = -O3 -Wall -std=c++11 -I $(incdir)
#flags = -O3 -Wall -std=c++11 -I $(incdir) -DJACOBIAN
flags = -O3 -Wall -std=c++11 -I $(incdir) -DJACOBIAN -DPARABOLIC

pflags = $(flags) -include mpi.h

# the program
alloy625: alloy625.cpp
	$(compiler) $(flags) $< -o $@ -lz -lgsl -lgslcblas -fopenmp

parallel: alloy625.cpp
	$(pcompiler) $(pflags) $< -o $@ -lz -lgsl -lgslcblas

minima: minima.c
	g++ -O3 -Wall $< -o $@ -lgsl -lgslcblas

clean:
	rm -f alloy625 parallel

#alloy718: alloy718.cpp
#	$(compiler) $(flags) $< -o $@ -lz
#
#parallel: alloy718.cpp
#	$(pcompiler) $(pflags) $< -o $@ -lz
#
#clean:
#	rm -f alloy718 parallel
