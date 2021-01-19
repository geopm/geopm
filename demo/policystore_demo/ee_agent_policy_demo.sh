#!/bin/bash

# put geopm install paths in environment
source ~/env.sh

BENCH_CONFIG=dgemm_config.json
echo "{\"loop-count\": 100, \"region\": [\"dgemm\", \"stream-unmarked\"], \"big-o\": [38.0, 2.0]}" > $BENCH_CONFIG
#echo "{\"loop-count\": 100, \"region\": [\"dgemm-unmarked\", \"stream-unmarked\"], \"big-o\": [38.0, 2.0]}" > $BENCH_CONFIG
#echo "{\"loop-count\": 100, \"region\": [\"spin\", \"stream\"], \"big-o\": [2.0, 2.0]}" > $BENCH_CONFIG

POLICY_SHMEM=/geopm_endpoint_test

NUM_NODES=$SLURM_NNODES
RANKS_PER_NODE=2
NUM_RANKS=$((${RANKS_PER_NODE} * ${NUM_NODES}))

for prof in scalability_hi; do
#for prof in scalability_hi scalability_med scalability_lo default; do

    # launch the process on the node
    # slurm plugin should do this
    srun -w ${SLURM_NODELIST} /home/drguttma/geopm/examples/endpoint/.libs/geopm_static_policy_demo -s $POLICY_SHMEM &
    pid=$!

    # launch a job with matching policy path
    # this should come from /etc/environment-override.json
    geopmlaunch srun -N${NUM_NODES} -n${NUM_RANKS} \
                --geopm-agent=energy_efficient \
                --geopm-policy=${POLICY_SHMEM} \
                --geopm-profile=${prof} \
                --geopm-report=${prof}.report \
                --geopm-trace-policy ${prof}_policy_trace \
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
