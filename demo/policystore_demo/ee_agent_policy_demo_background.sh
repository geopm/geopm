#!/bin/bash

# put geopm install paths in environment
source ~/env.sh

BENCH_CONFIG=dgemm_config.json
echo "{\"loop-count\": 100, \"region\": [\"dgemm\", \"spin\"], \"big-o\": [38.0, 1.0]}" > $BENCH_CONFIG

POLICY_SHMEM=/geopm_demo

for prof in default prio_lo prio_med prio_hi; do

    # launch the process on the node
    # slurm plugin should do this
    /home/drguttma/geopm/examples/endpoint/.libs/geopm_static_policy_demo -s $POLICY_SHMEM &
    pid=$!

    # launch a job with matching policy path
    # this should come from /etc/environment-override.json
    geopmlaunch srun -N1 -n2 \
                --geopm-agent=energy_efficient \
                --geopm-policy=${POLICY_SHMEM} \
                --geopm-profile=${prof} \
                --geopm-report=${prof}.report \
                --geopm-trace-policy ${prof}_policy_trace \
                --geopm-region-barrier \
                -- geopmbench $BENCH_CONFIG

    grep "Profile" ${prof}.report
    tail -n1 ${prof}_policy_trace
    grep -A 9 dgemm ${prof}.report | grep runtime
    grep -A 9 dgemm ${prof}.report | grep energy
    grep -A 9 dgemm ${prof}.report | grep requested

    # kill the backgrounded process if if didn't end
    kill -9 $pid

    # clean up shmem if it didn't clean up
    rm -f /dev/shm/${POLICY_SHMEM}*

done
