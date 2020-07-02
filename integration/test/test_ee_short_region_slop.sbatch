#!/bin/bash
#SBATCH -N 1
#SBATCH -J test_ee_short_region_slop
#SBATCH -t 36:00:00
#SBATCH --reservation ee_slop
#SBATCH -o %j.out
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
GEOPM_KEEP_FILES=true \
python ./test_ee_short_region_slop.py
mv *.png $out_dir_0
mv *.report* *.log *.json *trace* $out_dir_0/data

# Fix uncore at max
max_uncore_freq=$(geopmread MSR::UNCORE_RATIO_LIMIT:MAX_RATIO board 0)
geopmwrite MSR::UNCORE_RATIO_LIMIT:MIN_RATIO board 0 $max_uncore_freq

# Run full range for CPU and fixed uncore
GEOPM_KEEP_FILES=true \
python ./test_ee_short_region_slop.py
mv *.png $out_dir_1
mv *.report* *.log *.json *trace* $out_dir_1/data

# Run limited range for CPU and fixed uncore
GEOPM_KEEP_FILES=true \
GEOPM_SLOP_FREQ_MIN=1.6e9 \
GEOPM_SLOP_FREQ_MAX=2.0e9 \
python ./test_ee_short_region_slop.py
mv *.png $out_dir_2
mv *.report* *.log *.json *trace* $out_dir_2/data

