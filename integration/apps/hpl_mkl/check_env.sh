#!/bin/bash
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

echo "This script will check if:"
echo "   1. MKL is installed in your system and environment is set (check \$MKLROOT environment variable)."
echo "   2. Intel HPL binary (xhpl_intel64_dynamic) is installed as a part of the MKL installation in the"
echo "      \$MKLROOT/benchmarks/mp_linpack directory."
echo "   3. MPI version is IMPI."
echo "It will not perform any download/build. User is expected to install MKL manually."

exit_val=0

echo "Check 1..."
if [ -z "${MKLROOT}" ]; then
    echo "ERROR: MKLROOT env variable is not found. Make sure that the MKL is installed."
    exit_val=1
else
    echo "Passed."
fi

echo "Check 2..."
xhpl_executable=${MKLROOT}/benchmarks/mp_linpack/xhpl_intel64_dynamic
if [ ! -f "${xhpl_executable}" ]; then
    echo "ERROR: HPL executable is not found in the MKL installation at ${xhpl_executable}."
    exit_val=1
else
    echo "Passed."
fi

echo "Check 3..."
mpilib=$(ldd ${xhpl_executable} |grep libmpi.so)
echo "    MPI library: ${mpilib}"
if echo ${mpilib} |grep mvapich > /dev/null; then
    echo "WARNING: Detected MVAPICH MPI. Not recommended for MKL HPL runs."
else
    echo "Passed."
fi

exit ${exit_val}
