#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

# This file is sourced by each tutorial script and determines the
# default environment for the tutorial.  Modify these variables to
# suit your environment or export them before running the tutorial.

# GEOPM_PREFIX: Where to find lib and include directories for geopm.
if [ ! "$GEOPM_PREFIX" ]; then
    GEOPM_PREFIX=$HOME/build/geopm
fi

# GEOPM_RM: The resource manager to use to launch jobs.
# Options are either 'ALPS' or 'SLURM'.
if [ ! "$GEOPM_RM" ]; then
    GEOPM_RM='SLURM'
fi

# GEOPM_BINDIR: Diretory containing libgeopm.so.
if [ ! "$GEOPM_BINDIR" ]; then
    GEOPM_BINDIR=$GEOPM_PREFIX/bin
fi

# GEOPM_LIBDIR: Diretory containing libgeopm.so.
if [ ! "$GEOPM_LIBDIR" ]; then
    GEOPM_LIBDIR=$GEOPM_PREFIX/lib
fi

# GEOPMPY_PKGDIR: Diretory containing .
if [ ! "$GEOPMPY_PKGDIR" ]; then
    GEOPMPY_PKGDIR=$GEOPM_PREFIX/lib/python2.7/site-packages
fi

# GEOPM_INCLUDEDIR: Directory containing geopm.h.
if [ ! "$GEOPM_INCLUDEDIR" ]; then
    GEOPM_INCLUDEDIR=$GEOPM_PREFIX/include
fi

# GEOPM_CFLAGS: Contains compile options for geopm.
if [ ! "$GEOPM_CFLAGS" ]; then
    GEOPM_CFLAGS="-I$GEOPM_INCLUDEDIR"
fi

# GEOPM_CFLAGS: Contains link options for geopm.
if [ ! "$GEOPM_LDFLAGS" ]; then
    GEOPM_LDFLAGS="-L$GEOPM_LIBDIR -lgeopm"
fi
