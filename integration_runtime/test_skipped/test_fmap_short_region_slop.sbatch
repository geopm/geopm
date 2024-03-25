#!/bin/bash
#SBATCH -N 1
#SBATCH -J test_fmap_short_region_slop
#SBATCH -t 36:00:00
#SBATCH --reservation fmap_slop
#SBATCH -o %j.out
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

export OMP_NUM_THREADS=21
set -x
hostname
source $HOME/build/geopm-env.sh
out_dir_0=$HOME/public_html/$SLURM_JOB_ID/ucore-1.0-2.4
out_dir_1=$HOME/public_html/$SLURM_JOB_ID/ucore-2.4-2.4
out_dir_2=$HOME/public_html/$SLURM_JOB_ID/freq-1.6-2.0
mkdir -p $out_dir_0/data
mkdir -p $out_dir_1/data
mkdir -p $out_dir_2/data

# Run full range for CPU and uncore
python3 ./test_fmap_short_region_slop.py
mv *.png $out_dir_0
mv *.report* *.log *.json *trace* $out_dir_0/data

# Fix uncore at max
max_uncore_freq=$(geopmread MSR::UNCORE_RATIO_LIMIT:MAX_RATIO board 0)
geopmwrite MSR::UNCORE_RATIO_LIMIT:MIN_RATIO board 0 $max_uncore_freq

# Run full range for CPU and fixed uncore
python3 ./test_fmap_short_region_slop.py
mv *.png $out_dir_1
mv *.report* *.log *.json *trace* $out_dir_1/data

# Run limited range for CPU and fixed uncore
GEOPM_SLOP_FREQ_MIN=1.6e9 \
GEOPM_SLOP_FREQ_MAX=2.0e9 \
python3 ./test_fmap_short_region_slop.py
mv *.png $out_dir_2
mv *.report* *.log *.json *trace* $out_dir_2/data

