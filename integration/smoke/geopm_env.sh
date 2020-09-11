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

if [ ! "$GEOPM_PREFIX" ]; then
    export GEOPM_PREFIX=$HOME/build/geopm
fi
if [ ! "$GEOPM_SRC" ]; then
    export GEOPM_SRC=$HOME/Git/geopm
fi

PATH_EXT=$GEOPM_PREFIX/bin
LD_LIBRARY_PATH_EXT=$GEOPM_PREFIX/lib
PYTHONPATH_EXT=$GEOPM_PREFIX/lib/python3.6/site-packages:$GEOPM_SRC/integration

if [ ! "$PATH" ]; then
    echo "Warning: No PATH set?" 1>&2
    export PATH=$PATH_EXT
else
    export PATH=$PATH_EXT:$PATH
fi

if [ ! "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH_EXT
else
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH_EXT:$LD_LIBRARY_PATH
fi

if [ ! "$PYTHONPATH" ]; then
    export PYTHONPATH=$PYTHONPATH_EXT
else
    export PYTHONPATH=$PYTHONPATH_EXT:$PYTHONPATH
fi
