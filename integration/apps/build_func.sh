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


# Clean out old versions of application source and warn user
clean_source() {
    local DIRNAME=${1}
    # Clear out old versions:
    if [ -d "${DIRNAME}" ]; then
        echo "WARNING: Previous source directory detected at ./${DIRNAME}"
        read -p "OK to delete and rebuild? (y/n) " -n 1 -r
        echo
        if [[ ${REPLY} =~ ^[Yy]$ ]]; then
            rm -rf ${DIRNAME}
        else
            echo "Not OK.  Stopping."
            exit 1
        fi
    fi
}

# Setup a git repository and apply patches
setup_source_git() {
    local BASEDIR=${PWD}
    local DIRNAME=${1}
    if [ $# == 2 ]; then
        local PATCH_LIST=${2}
    else
        local PATCH_LIST="$(ls ${BASEDIR}/*.patch 2> /dev/null || true)"
    fi
    cd ${DIRNAME}
    # Create a git repo for the app source
    git init
    git checkout -b main
    git add -A
    git commit --no-edit -sm "Initial commit"
    if [ ! -z  "${PATCH_LIST}" ]; then
        git am ${PATCH_LIST}
    fi
    cd -
}

# Get the source archive from local cache or web page
get_archive() {
    local ARCHIVE=$1
    if [ ! -f ${ARCHIVE} ]; then
        if [ -f "${GEOPM_APPS_SRCDIR}/${ARCHIVE}" ]; then
            cp "${GEOPM_APPS_SRCDIR}/${ARCHIVE}" .
        elif [ $# -eq 2 ]; then
            local URL=$2
            wget ${URL}/${ARCHIVE}
        fi
    fi
}

# Unpack an archive with tar or unzip
unpack_archive() {
    local ARCHIVE=$1
    if [ "${ARCHIVE##*.}" == zip ]; then
        unzip ${ARCHIVE}
    else
        tar xvf ${ARCHIVE}
    fi
}
