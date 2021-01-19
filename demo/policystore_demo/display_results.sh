#!/bin/bash

for region in epoch dgemm stream Application; do
    echo $region
    grep -A11 $region *.report | grep ' runtime\|requested\|package-energy\|requested\|frequency (Hz)'
done
tail -n 1 *policy_trace
