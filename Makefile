# Makefile
# GNU makefile for Ni-base superalloy decomposition
# Questions/comments to trevor.keller@nist.gov (Trevor Keller)
# Compiler optimizations after http://www.nersc.gov/users/computational-systems/retired-systems/hopper/performance-and-optimization/compiler-comparisons/

# compilers: Intel, GCC, MPI
icompiler = icc
gcompiler = g++
pcompiler = mpicxx

# libraries: z
stdlinks = -lz

# precompiler directives
# Options: -DCALPHAD	-DPARABOLA	-DNDEBUG
stddirect = -DPARABOLA -DNDEBUG
dbgdirect = -DPARABOLA

# flags: common, debug, Intel, GNU, and MPI
stdflags  = -Wall -std=c++11 -I $(MMSP_PATH)/include -I ..
dbgflags  = -pedantic $(stdflags) $(dbgdirect) -O0 -pg
idbgflags = $(stdflags) $(dbgdirect) -O0 -profile-functions -profile-loops=all -profile-loops-report=2

#iccflags = $(stdflags) $(stddirect) -gcc-name=/usr/bin/g++-4.9 -w3 -diag-disable:remark -O3 -funroll-loops -opt-prefetch
iccflags = $(stdflags) $(stddirect) -w3 -diag-disable:remark -O3 -funroll-loops -qopt-prefetch
gccflags = $(stdflags) $(stddirect) -pedantic -O3 -funroll-loops -ffast-math
pgiflags = -I $(MMSP_PATH)/include $(stddirect) -fast -Mipa=fast,inline,safe -Mfprelaxed -std=c++11
mpiflags = $(gccflags) -include mpi.h

# WORKSTATION EXECUTABLES

# default program (shared memory, OpenMP)
alloy625: alloy625.cpp
	$(icompiler) $< -o $@ $(iccflags) $(stdlinks) -qopenmp

# profiling program (no parallelism or optimization)
serial: alloy625.cpp
	$(gcompiler) $< -o $@ $(dbgflags) $(stdlinks)

iserial: alloy625.cpp
	$(icompiler) $< -o $@ $(idbgflags) $(stdlinks)

# CLUSTER EXECUTABLES

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

# PGI compiler
pgparallel: alloy625.cpp
	$(pcompiler) $(pgiflags) -include mpi.h $< -o $@ $(stdlinks) -mp


# DOCUMENTATION
.PHONY: docs
docs:
	$(MAKE) -C docs

# THERMODYNAMICS EXECUTABLES
.PHONY: thermo
thermo:
	$(MAKE) -C thermo

# ANALYSIS EXECUTABLES
.PHONY: analysis
analysis:
	$(MAKE) -C analysis

.PHONY: clean
clean:
	rm -f alloy625 ibtest iserial ismpi parallel pgparallel serial smp smpi
