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

AMG_DIR=AMG-master
# Clear out old versions:
if [ -d "$AMG_DIR" ]; then
    echo "WARNING: Previous AMG checkout detected at ./$AMG_DIR"
    read -p "OK to delete and rebuild? (y/n) " -n 1 -r
    echo
    if [[ ${REPLY} =~ ^[Yy]$ ]]; then
        rm -rf $AMG_DIR
    else
        echo "Not OK.  Stopping."
        exit 1
    fi
fi

# Acquire the source:
wget https://asc.llnl.gov/coral-2-benchmarks/downloads/AMG-master-5.zip

# Unpack the source:
unzip AMG-master-5.zip

# Change directories to the unpacked files:
cd $AMG_DIR

# Create a git repo for the app source
git init
git add -A
git commit -sm "Initial commit"

# Patch AMG with the patch utility:
git am ../0001-Adding-geopm-markup-to-CORAL-2-AMG.patch

# Build
make
