#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

num_trial=20
limit_list=($(seq 90 7 230))
if which msrsave >& /dev/null; then
    msrsave test_sweep_msrsave.out
fi

count=0
total_runs=$((${#limit_list[@]} * ${num_trial}))

for limit in ${limit_list[@]}; do
    for trial in $(seq $num_trial); do
        count=$((count+1))
        echo -ne "Starting run ${count} / ${total_runs} at ${limit} W... "
        ./rapl_pkg_limit_test $limit
        echo -ne "Done.\n"
    done
done
