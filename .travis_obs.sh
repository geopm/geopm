#!/bin/bash
#
#  Copyright (c) 2015, 2016, Intel Corporation
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

if [ -f VERSION ]; then
    version=$(cat VERSION)
fi

if [ "$TRAVIS_PULL_REQUEST" == "false" ] && \
   [ $OBS_BRANCH ] && \
   [ $OSC_CREDENTIALS ] && \
   [ $OBS_REPO ] && \
   [ "$TRAVIS_BRANCH" == "$OBS_BRANCH" ] && \
   [ $version ] && \
   [ $version != "0.0.0" ]; then
    spec_file=geopm-${version}.spec
    source_file=geopm-${version}.tar.gz
    curl -X DELETE -u ${OSC_CREDENTIALS} https://api.opensuse.org/source/${OBS_REPO}/geopm/geopm.spec
    curl -X DELETE -u ${OSC_CREDENTIALS} https://api.opensuse.org/source/${OBS_REPO}/geopm/geopm.tar.gz
    curl -X PUT -T $spec_file -u ${OSC_CREDENTIALS} https://api.opensuse.org/source/${OBS_REPO}/geopm/geopm.spec
    curl -X PUT -T $source_file -u ${OSC_CREDENTIALS} https://api.opensuse.org/source/${OBS_REPO}/geopm/geopm.tar.gz
else
    echo TRAVIS_PULL_REQUEST: $TRAVIS_PULL_REQUEST
    echo TRAVIS_BRANCH: $TRAVIS_BRANCH
    echo OBS_BRANCH: $OBS_BRANCH
    echo OBS_REPO: $OBS_REPO
    echo version: $version
    echo "Did not trigger open suse build"
fi

