#!/bin/bash



# run a non-MPI job
# this just reads the PERF_STATUS register, indicating the processor frequency
set -x
srun --reservation=diana geopmread FREQUENCY board 0


# run an MPI job with GEOPM
BENCH_CONFIG=dgemm_config.json
echo "{\"loop-count\": 10, \"region\": [\"dgemm\", \"stream\"], \"big-o\": [8.0, 1.5]}" > $BENCH_CONFIG
geopmlaunch srun -N1 -n1 --reservation=diana \
            --geopm-report=test.report -- geopmbench $BENCH_CONFIG

# check the frequency in the report
grep Policy test.report
grep -A11 Region test.report | grep 'Region\|frequency (Hz)'


# example of user trying to override the agent
geopmlaunch srun -N1 -n1 --reservation=diana \
            --geopm-agent=power_balancer \
            --geopm-report=test2.report -- geopmbench $BENCH_CONFIG
