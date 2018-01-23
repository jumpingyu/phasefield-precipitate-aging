# Makefile
# GNU makefile for Ni-base superalloy decomposition
# Questions/comments to trevor.keller@nist.gov (Trevor Keller)
# Compiler optimizations after http://www.nersc.gov/users/computational-systems/retired-systems/hopper/performance-and-optimization/compiler-comparisons/

# compilers: Intel, GCC, MPI
icompiler = icc
gcompiler = /usr/bin/g++-4.9
pcompiler = mpicxx


# libraries: z, gsl, mpiP
stdlinks = -lz -lgsl -lgslcblas
mpilinks = -lmpiP -lbfd -liberty


# precompiler stddirect
# Options: -DCALPHAD	-DPARABOLA	-DADAPTIVE_TIMESTEPS	-DNDEBUG
stddirect = -DPARABOLA -DNDEBUG
dbgdirect = -DPARABOLA


# flags: common, debug, Intel, GNU, and MPI
stdflags  = -Wall -std=c++11 -I $(MMSP_PATH)/include
dbgflags  = $(stdflags) $(dbgdirect) -O0 -pg
idbgflags = $(stdflags) $(dbgdirect) -O0 -profile-functions -profile-loops=all -profile-loops-report=2

iccflags = $(stdflags) $(stddirect) -w3 -diag-disable:remark -O3 -funroll-loops -opt-prefetch
gccflags = $(stdflags) $(stddirect) -pedantic -O3 -funroll-loops -ffast-math
pgiflags = -I $(MMSP_PATH)/include $(stddirect) -fast -Mipa=fast,inline,safe -Mfprelaxed -std=c++11
mpiflags = $(gccflags) -include mpi.h


# WORKSTATION

# default program (shared memory, OpenMP)
alloy625: alloy625.cpp
	$(icompiler) $< -o $@ $(iccflags) $(stdlinks) -openmp

# profiling program (no parallelism or optimization)
serial: alloy625.cpp
	$(gcompiler) $< -o $@ $(dbgflags) $(stdlinks)

iserial: alloy625.cpp
	$(icompiler) $< -o $@ $(iccflags) $(stdlinks)


# CLUSTER

# threaded program (shared memory, OpenMP)
smp: alloy625.cpp
	$(gcompiler) $< -o $@ $(gccflags) $(stdlinks) -fopenmp

# parallel program (distributed memory, MPI)
parallel: alloy625.cpp $(core)
	$(pcompiler) $< -o $@ $(mpiflags) $(stdlinks)

smpi: alloy625.cpp
	$(pcompiler) $< -o $@ $(mpiflags) $(stdlinks) -fopenmp

ismpi: alloy625.cpp
	$(pcompiler) $< -o $@ $(iccflags) -include mpi.h -L/usr/lib64 $(stdlinks) -fopenmp

ibtest: alloy625.cpp
	/usr/local/bin/mpicxx $< -o $@ $(mpiflags) $(stdlinks) -fopenmp

# PGI compiler
pgparallel: alloy625.cpp
	$(pcompiler) $(pgiflags) -include mpi.h $< -o $@ $(stdlinks) -mp


# DESCRIPTION
description: phasefield-precipitate-aging_description.tex
	pdflatex -interaction=nonstopmode $<


# UTILITIES

# extract composition from line profile
mmsp2comp: mmsp2comp.cpp
	$(gcompiler) $(stdflags) $(stddirect) -O2 $< -o $@ -lz

# extract phase fractions
mmsp2frac: mmsp2frac.cpp
	$(gcompiler) $(stdflags) -O2 $< -o $@ -lz

# check interfacial adsorption (should be zero)
adsorption: adsorption.cpp
	$(gcompiler) $(stdflags) -O2 $< -o $@ -lz

# generate equilibrium phase diagram information
equilibrium: equilibrium.cpp
	$(gcompiler) $(gccflags) $< -o $@ -lgsl -lgslcblas

clean:
	rm -f adsorption alloy625 equilibrium ibtest iserial mmsp2comp parallel pgparallel serial smp smpi
