#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This file is sourced by each tutorial script and determines the
# default environment for the tutorial.  Modify these variables to
# suit your environment or export them before running the tutorial.

if [ -z ${GEOPM_INSTALL+x} ]; then
    if [ -f ${HOME}/.geopmrc ]; then
        source ${HOME}/.geopmrc
    else
        echo "Error: Please set GEOPM_INSTALL in your environment."
        exit 1
    fi
fi

# GEOPM_LAUNCHER: The resource manager exe used to launch jobs.
# See 'man geopmlaunch' for supported options.
if [ ! "$GEOPM_LAUNCHER" ]; then
    GEOPM_LAUNCHER='srun'
fi

# GEOPM_BIN: Directory containing geopm programs.
if [ ! "$GEOPM_BIN" ]; then
    GEOPM_BIN=$GEOPM_INSTALL/bin
fi

# GEOPM_LIB: Directory containing libgeopm.so, libgeopmd.so, etc.
if [ ! "$GEOPM_LIB" ]; then
    GEOPM_LIB=$GEOPM_INSTALL/lib
fi
LD_LIBRARY_PATH=${GEOPM_LIB}:${LD_LIBRARY_PATH}

# GEOPMPY_PKGDIR: Directory containing geopmpy packages.
if [ ! "$GEOPMPY_PKGDIR" ]; then
    # Use whichever python version was used to build geopmpy
    GEOPMPY_PKGDIR=$(ls -dv $GEOPM_LIB/python*/site-packages 2>/dev/null)
    if [ 0 -eq $? ]; then
        if [ 1 -ne $(echo "$GEOPMPY_PKGDIR" | wc -l) ]; then
            GEOPMPY_PKGDIR=$(echo "$GEOPMPY_PKGDIR" | tail -n1)
            echo 1>&2 "Warning: More than 1 python site-packages directory in $GEOPM_LIB"
            echo 1>&2 "         Remove all except one, or manually set GEOPMPY_PKGDIR."
            echo 1>&2 "         Assuming GEOPMPY_PKGDIR=${GEOPMPY_PKGDIR}."
        fi
    else
        if ! python -c 'import geopmdpy; import geopmpy' 2>/dev/null; then
            echo 1>&2 "Warning: Unable to find python site-packages in $GEOPM_LIB"
        fi
    fi
fi

# GEOPM_INC: Directory containing geopm_prof.h.
if [ ! "$GEOPM_INC" ]; then
    GEOPM_INC=$GEOPM_INSTALL/include
fi

# GEOPM_CFLAGS: Contains compile options for geopm.
if [ ! "$GEOPM_CFLAGS" ]; then
    GEOPM_CFLAGS="-I$GEOPM_INC"
fi

# GEOPM_LDFLAGS: Contains link options for geopm.
if [ ! "$GEOPM_LDFLAGS" ]; then
    GEOPM_LDFLAGS="-L$GEOPM_LIB"
fi
