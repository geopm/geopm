#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

echo "This script will check if:"
echo "   1. Intel (R) MKL is installed in your system and environment is set (check \$MKLROOT environment variable)."
echo "   2. Intel (R) HPL binary (xhpl_intel64_dynamic) is installed as a part of the MKL installation in the"
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
mpilib=$(ldd ${xhpl_executable} | grep libmpi.so)
echo "    MPI library: ${mpilib}"
if echo ${mpilib} |grep mvapich > /dev/null; then
    echo "WARNING: Detected MVAPICH MPI. Not recommended for MKL HPL runs."
else
    echo "Passed."
fi

exit ${exit_val}
