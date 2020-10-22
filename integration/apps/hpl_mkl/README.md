# Introduction

From https://www.netlib.org/benchmark/hpl/:

    HPL is a software package that solves a (random) dense linear
    system in double precision (64 bits) arithmetic on distributed-memory
    computers. It can thus be regarded as a portable as well as freely
    available implementation of the High Performance Computing Linpack Benchmark.

This AppConf runs the MKL version of HPL, which is optimized for Intel
processors.

Testing has been done with MKL 19.0.4.243 and impi 2019.4.243 (dynamically linked).
IMPI is required for the best performance.

The --perc-dram option passed to the run scripts that call this banchmark
create a matrix size that fits the given percentage of the total DRAM of the nodes.
80-90% is recommended for best performance. Lower values will not have the best
best ALU utilization, whereas higher percentages will compete with OS resources.
Lower percentages can be used to run fast tests.

The benchmark implementation here allows node sizes of 1, 2, 4, 8 and 16.

The MKL AppConf class (HplMklAppConf) is derived from HplNetlibAppConf since
the app configurations are mostly the same.

# HPL Threading

HPL uses MPI/OpenMP parallelism where each MPI rank creates a certain number
of OpenMP threads, which is defined by the benchmark user. In this case,
we choose to use all available physical cores in the node. Also, HPL does not
benefit from hyper-threads, which should be turned off.

This would normally be done by setting the following environment variables
(assuming there are 44 cores per node in this example):

    $ export KMP_HW_SUBSET=1t   # forces 1 thread per core to be used
    $ export OMP_NUM_THREADS=44 # number of cores per node
    $ export MKL_NUM_THREADS=44 # matches with above, effective if linked against MKL.

MKL_NUM_THREADS is used by the MKL libraries to determine how many threads
are used per rank.

In the GEOPM integration infrastructure, OMP_NUM_THREADS is inferred from
the get_cpu_per_rank function of AppConf base class. This is set to the number of
cores per node inside the HplMklAppConf class in hpl_mkl.py.

Similarly hyper-threading is turned off via --geopm-hyperthreads-disable in the 
HplMklAppConf.get_custom_geopm_args function.

Finally, the specific way that HPL is written does not allow GEOPM to start in
process control mode (i.e. GEOPM cannot start a separate MPI process to
run its control loop). Therefore GEOPM is started in application-mode where it
shares a core with the app. This is done via --geopm-ctl=application in the
HplMklAppConf.get_custom_geopm_args function.