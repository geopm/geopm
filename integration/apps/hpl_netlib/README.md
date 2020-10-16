# Introduction

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
cores per node inside the HplCpuAppConf class in hpl_cpu.py.

Similarly hyper-threading is turned off via --geopm-hyperthreads-disable in the 
HplCpuAppConf.get_custom_geopm_args function.

Finally, the specific way that HPL is written does not allow GEOPM to start in
process control mode (i.e. GEOPM cannot start a separate MPI process to
run its control loop). Therefore GEOPM is started in application-mode where it
shares a core with the app. This is done via --geopm-ctl=application in the
HplCpuAppConf.get_custom_geopm_args function.