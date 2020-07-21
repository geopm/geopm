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

#TUTORIAL_BASE
#
#  Setup script that installs geopm into a temporary location, and
#  then builds the tutorials from that install based on the
#  geopm-tutorial.tar.gz file generated by make dist.
#

set -e
export SRC_BASE_DIR=$(dirname $(dirname $(dirname $(realpath $0))))
export LINK_DIR=$SRC_BASE_DIR/integration/test/test_tutorial_base

function get_config
{
    # Extract variable value out of the config.log
    grep "^$1=" $SRC_BASE_DIR/config.log | sed "s|$1='\(.*\)'|\1|"
}

# Determine the compiler
if [[ $(get_config CC) == 'icc' ]]; then
    compiler=intel
elif [[ $(get_config CC) == 'gcc' ]]; then
    compiler=gnu
else
    echo "Error: $0 unable to determine compiler" 1>&2 && false
fi

# Clean out last install if it exists
if [[ -L $LINK_DIR ]]; then
    rm -rf $(readlink $LINK_DIR) $LINK_DIR
fi

# Make directory for temporary install and tutorial
mkdir -p $HOME/.local/tmp/
temp_dir=$(mktemp -d $HOME/.local/tmp/geopm-tutorial-test.XXXXXXXX)

# Create a symbolic link to the temporary install directory
ln -s $temp_dir $LINK_DIR

# Install geopm into the temporary location
destdir=$temp_dir/build
prefix=$(get_config prefix)
cd $SRC_BASE_DIR
DESTDIR=$destdir make install
make dist

# Build the tutorials
cd -P $LINK_DIR
tar xvf $SRC_BASE_DIR/geopm-tutorial.tar.gz
cd geopm-tutorial
export GEOPM_PREFIX=$destdir/$prefix
./tutorial_build_$compiler.sh

# Print success message
echo SUCCESS: $0
