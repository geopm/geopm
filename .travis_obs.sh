#!/bin/bash
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

do_obs=false

if [ -f VERSION ]; then
    version=$(cat VERSION)
fi

spec_file=geopm.spec
source_file=geopm-${version}.tar.gz
obs_url=https://api.opensuse.org

if [ "$TRAVIS_PULL_REQUEST" == "false" ] && \
   [ $OSC_CREDENTIALS ] && \
   [ $version ] && \
   [ $version != "0.0.0" ]; then

    if [ "$TRAVIS_REPO_SLUG" == "geopm/geopm" ] && \
       [ "$TRAVIS_BRANCH" == "master" ] && \
       git describe --exact-match >& /dev/null; then
        obs_proj=home:cmcantalupo:geopm
        obs_pkg=geopm
        do_obs=true
    elif [ "$TRAVIS_REPO_SLUG" == "geopm/geopm" ] && \
         [ "$TRAVIS_BRANCH" == "dev" ]; then
        obs_proj=home:cmcantalupo:geopm-dev
        obs_pkg=geopm-dev
        do_obs=true
    elif [ "$TRAVIS_REPO_SLUG" == "cmcantalupo/geopm" ]; then
        obs_proj=home:cmcantalupo
        obs_pkg=geopm
        do_obs=true
    elif [ "$TRAVIS_REPO_SLUG" == "bgeltz/geopm" ]; then
        obs_proj=home:bgeltz
        obs_pkg=geopm
        do_obs=true
    elif [ "$TRAVIS_REPO_SLUG" == "sssylvester/geopm" ]; then
        obs_proj=home:sssylvester
        obs_pkg=geopm
        do_obs=true
    fi

fi

if [ "$do_obs" == "true" ]; then
    echo "[general]" > $HOME/.oscrc
    echo "apiurl = $obs_url" >> $HOME/.oscrc
    echo "[$obs_url]" >> $HOME/.oscrc
    echo "user = $(echo $OSC_CREDENTIALS | awk -F: '{print $1}')" >> $HOME/.oscrc
    echo "pass = $(echo $OSC_CREDENTIALS | awk -F: '{print $2}')" >> $HOME/.oscrc
    osc co $obs_proj $obs_pkg && \
    cp $spec_file $obs_proj/$obs_pkg/geopm.spec && \
    cp $source_file $obs_proj/$obs_pkg/geopm.tar.gz && \
    cd $obs_proj/$obs_pkg && \
    osc add geopm.spec geopm.tar.gz &> /dev/null
    osc ci -m $version && \
    cd - && \
    echo "Pushed to OpenSUSE builds." ||
    echo "Failed to push to OpenSUSE builds."
else
    echo TRAVIS_PULL_REQUEST: $TRAVIS_PULL_REQUEST
    echo TRAVIS_REPO_SLUG: $TRAVIS_REPO_SLUG
    echo TRAVIS_BRANCH: $TRAVIS_BRANCH
    echo version: $version
    if [ ! $OSC_CREDENTIALS ]; then
        echo OSC_CREDENTIALS UNDEFINED
    fi
    echo "Did not push to OpenSUSE builds."
fi
