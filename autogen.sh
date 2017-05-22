#!/bin/bash
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

# If the VERSION file does not exist, then create it based on git
# describe or if not in a git repo just set VERSION to 0.0.0.
if [ ! -f VERSION ]; then
    if git describe --long > /dev/null; then
        sha=$(git describe --long | awk -F- '{print $(NF)}')
        release=$(git describe --long | awk -F- '{print $(NF-1)}')
        version=$(git describe --long | sed -e "s|\(.*\)-$release-$sha|\1|" -e "s|-|+|g" -e "s|^v||")
        if [ "${release}" != "0" ]; then
            version=${version}+dev${release}${sha}
        fi
    else
        echo "WARNING:  VERSION file does not exist and git describe failed, setting verison to 0.0.0" 2>&1
        version=0.0.0
    fi
    echo $version > VERSION
    echo "__version__ = '$version'" > scripts/geopmpy/version.py
fi

if [ ! -f MANIFEST ]; then
    if [ -f .git/config ]; then
        git ls-tree --full-tree -r HEAD | awk '{print $4}' | sort > MANIFEST
    else
        echo "WARNING: MANIFEST file does not exist and working directory is not a git repository, creating with find" 2>&1
        find . -type f | sed 's|^\./||' | sort > MANIFEST
    fi
fi

mkdir -p m4
autoreconf -i -f
