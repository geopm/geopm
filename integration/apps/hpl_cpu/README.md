From https://www.netlib.org/benchmark/hpl/:

	HPL is a software package that solves a (random) dense linear
	system in double precision (64 bits) arithmetic on distributed-memory
	computers. It can thus be regarded as a portable as well as freely
	available implementation of the High Performance Computing Linpack Benchmark.

Testing has been done with MKL 19.0.4.243 and mvapich2 2.3.1 (dynamically linked).
Requires that the intel/19.0.4.243 environment module is loaded.

The --perc-dram option passed to the run scripts that call this banchmark
create a matrix size that fits the given percentage of the total DRAM of the nodes.
80-90% is recommended for best performance. Lower values will not have the best
best ALU utilization, whereas higher percentages will compete with OS resources.
Lower percentages can be used to run fast tests.

The benchmark implementation here allows node sizes of 1, 2, 4, 8 and 16.