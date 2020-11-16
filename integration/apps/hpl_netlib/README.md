# Introduction

From https://www.netlib.org/benchmark/hpl/:

	HPL is a software package that solves a (random) dense linear
	system in double precision (64 bits) arithmetic on distributed-memory
	computers. It can thus be regarded as a portable as well as freely
	available implementation of the High Performance Computing Linpack Benchmark.

Testing has been done with mvapich2 2.3.1 (dynamically linked) and Intel (R)
compiler/library package version 19.0.4.243.

The application is configured to solve a double-precision FP matrix in this infrastructure.
Matrix size can be controlled via the --frac-dram option, which can be passed to the run
scripts that call this banchmark. The double-precision FP matrix will be sized such that 
it fits in the given fraction of the total DRAM of the nodes (value between 0 and 1).
Values above 0.75 are recommended for best performance. Lower values will not have the best
best ALU utilization. Higher percentages may compete with OS resources or in extreme
cases (typically >0.9) may cause the application to quit with a "not enough memory" error.
Lower percentages can be used to run fast tests.

The benchmark implementation here allows node sizes of 1, 2, 4, 8 and 16.

# HPL Threading

HPL uses MPI/OpenMP parallelism where each MPI rank creates a certain number
of OpenMP threads, which is defined by the benchmark user. In this case,
we choose to use all available physical cores in the node. Also, HPL does not
benefit from hyper-threads, which should be turned off.

This would normally be done by setting the following environment variables
(assuming there are 44 cores per node in this example):

    $ export KMP_HW_SUBSET=1t # forces 1 thread per core to be used
    $ export OMP_NUM_THREADS=44 # number of cores per node

In the GEOPM integration infrastructure, OMP_NUM_THREADS is inferred from
the get_cpu_per_rank function of AppConf base class. This is set to the number of
cores per node inside the HplNetlibAppConf class in hpl_netlib.py.

Similarly hyper-threading is turned off via --geopm-hyperthreads-disable in the 
HplNetlibAppConf.get_custom_geopm_args function.

Finally, the specific way that HPL is written does not allow GEOPM to start in
process control mode (i.e. GEOPM cannot start a separate MPI process to
run its control loop). Therefore GEOPM is started in application-mode where it
shares a core with the app. This is done via --geopm-ctl=application in the
HplNetlibAppConf.get_custom_geopm_args function.