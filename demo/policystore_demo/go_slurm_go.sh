#!/bin/bash

# get geopm in PATH
source ~/env.sh

set -x

POLICY_SHMEM=/geopm_demo

# launch the process on the node
# slurm plugin should do this
./examples/endpoint/geopm_static_policy_demo -s $POLICY_SHMEM &
pid=$!


echo "{\"loop-count\": 10, \"region\": [\"spin\"], \"big-o\": [1.0]}" > short.conf
# launch a job with matching policy path
# this should come from /etc/environment-override.json
geopmlaunch srun  -N 1 -n 1 --geopm-policy=$POLICY_SHMEM \
            --geopm-agent=power_governor \
            --geopm-timeout=5 \
            -- geopmbench short.conf


# kill the backgrounded process if if didn't end
kill -9 $pid

# clean up shmem if it didn't clean up
rm /dev/shm/${POLICY_SHMEM}*
