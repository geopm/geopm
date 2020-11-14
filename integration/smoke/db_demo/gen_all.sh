#!/bin/bash

#ALL_APPS="dgemm_tiny amg dgemm minife nekbone hpcg nasft"
#ALL_EXP="monitor power_sweep frequency_sweep power_balancer_energy barrier_frequency_sweep"

APPS_2NODE="dgemm_tiny nasft nekbone"
# apps with no 2-node config
APPS_1NODE="minife amg"
ALL_EXP="monitor power_sweep"
for exp in $ALL_EXP; do
    for app in $APPS_2NODE; do
        ./gen_sbatch.py --app=${app} --exp-type=${exp} --node-count=2
        sbatch ${app}_${exp}.sbatch
    done

    for app in $APPS_1NODE; do
        ./gen_sbatch.py --app=${app} --exp-type=${exp} --node-count=1
        sbatch ${app}_${exp}.sbatch
    done

done
