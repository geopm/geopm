#!/bin/bash
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

# SMNG BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

# Default Intel Toolchain compiler overrides
# The MPI compiler wrappers are supported by Intel, but are not Intel specific.
module swap intel intel/19.1
module load python/3.6_intel
unset OMP_NUM_THREADS # Set to 1 by a default lmod module

# One time setup:
# python -m pip install --user -U pip
# python -m pip install --user --ignore-installed -r geopm/scripts/requirements.txt

export MPIFORT=mpiifort
export MPIFC=mpiifort
export MPIF77=mpiifort
export MPIF90=mpiifort

export GEOPM_USER_ACCOUNT=h1906
export GEOPM_SYSTEM_DEFAULT_QUEUE=test
export GEOPM_SBATCH_EXTRA_LINES="
#SBATCH --constraint=\"noscratch\"
#SBATCH --ear off
${GEOPM_SBATCH_EXTRA_LINES}
"

# Notes
##SBATCH --export=NONE # This breaks the env of srun calls within SBATCH scripts

