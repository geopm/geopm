#!/bin/bash
#  Copyright (c) 2015 - 2021, Intel Corporation
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
if [ -f VERSION_OVERRIDE ]; then
    cp VERSION_OVERRIDE VERSION
elif git describe --long > /dev/null 2>&1; then
    sha=$(git describe --long | awk -F- '{print $(NF)}')
    release=$(git describe --long | awk -F- '{print $(NF-1)}')
    version=$(git describe --long | sed -e "s|\(.*\)-$release-$sha|\1|" -e "s|-|+|g" -e "s|^v||")
    if [ "${release}" != "0" ]; then
        version=${version}+dev${release}${sha}
    fi
    echo $version > VERSION
elif [ ! -f VERSION ]; then
    echo "WARNING:  VERSION file does not exist and git describe failed, setting verison to 0.0.0" 1>&2
    echo "0.0.0" > VERSION
fi

for ff in AUTHORS CODE_OF_CONDUCT.md CONTRIBUTING.rst COPYING; do
    if [ ! -f $ff ]; then
        cp ../$ff .
    fi
done

autoreconf -i -f
