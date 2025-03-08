#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


if [[ $# -eq 0 ]] || [[ $1 == '--help' ]]; then
    printf "Usage: $0 --prefix=INSTALL_PREFIX ...

Overview:

When using pip to install a development version of geopmdpy, the ABI of
libgeopmd.so used at runtime must provide all the interfaces required by
geompdpy to avoid an import error.  The easiest way to ensure this is the case
is to install a version of libgeopmd.so that matches the pip installed version.

This script enables this use by installing geopmdpy and libgeopmd.so based on a
checkout of the geopm Git repo.  All command line arguments provided to this
script are forwarded to the configure command for libgeopmd.  User is expected
to provide the --prefix option which will install libgeopmd in a user defined
directory.  The user may also provide other options as required by their
environment.  For more details on how to configure libgeopmd.so see:

    https://geopm.github.io/devel.html#configuring-the-build

The user must set the environment variable GEOPM_GIT_PATH to the path of a geopm
Git repository when the script is moved or downloaded in isolation from the rest
of the GEOPM Git repo.  In the case where this environment variable is set, but
does not point to an existing repository, the script will clone a new geopm Git
repo.  The interactions with Git are controlled by environment variables
documented below.

Environment Variables:

    GEOPM_GIT_PATH: Path to Git repository. Defaults to two directories above
                    the location of this script.

    GEOPM_GIT_URL: URL to clone if GEOPM_GIT_PATH is set, but does not evaluate
                   to an existing directory.  Default value is
                   \"https://github.com/geopm/geopm.git\". To disable Git clone,
                   set GEOPM_GIT_PATH to an existing clone of the geopm Git
                   repository.

    GEOPM_GIT_CHECKOUT: Branch, sha or other commit-ish to checkout from the
                        repo.  Both checkout and rebase are disabled If
                        GEOPM_GIT_CHECKOUT=\"\" or is unset.

    GEOPM_GIT_REMOTE: Git remote to use for rebase.  Defaults to \"origin\" when
                      environment variable is unset.  Set GEOPM_GIT_REMOTE=\"\"
                      to disable rebase.


Example:"'

    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
    chmod a+x install_user.sh
    export GEOPM_GIT_PATH=$(mktemp -d)/geopm
    ./install_user.sh --prefix=$HOME/geopm-build --enable-levelzero
    export LD_LIBRARY_PATH=$HOME/geopm-build/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
    rm -rf $(dirname $GEOPM_GIT_PATH)


'
    exit 0
fi
set -x
set -e
python3 -m pip install --upgrade pip
DEFAULT_GIT_PATH=$(dirname $(dirname $(realpath $0)))
GEOPM_GIT_PATH=${GEOPM_GIT_PATH:-${DEFAULT_GIT_PATH}}
GEOPM_GIT_URL=${GEOPM_GIT_URL:-"https://github.com/geopm/geopm.git"}
GEOPM_GIT_REMOTE=${GEOPM_GIT_REMOTE:-"origin"}

if [ ! -d ${GEOPM_GIT_PATH} ]; then
    git clone ${GEOPM_GIT_URL} ${GEOPM_GIT_PATH}
fi
cd ${GEOPM_GIT_PATH}
if [ -n  "${GEOPM_GIT_CHECKOUT}" ]; then
    git checkout ${GEOPM_GIT_CHECKOUT}
    if [ -n "${GEOPM_GIT_REMOTE}" ]; then
        git fetch ${GEOPM_GIT_REMOTE}
        git rebase ${GEOPM_GIT_REMOTE}/${GEOPM_GIT_CHECKOUT}
    fi
fi
python3 -m pip install -r requirements.txt
cd libgeopmd
./autogen.sh
./configure $@
make -j
make install
INSTALL_PREFIX=$(grep '^prefix=' config.log | awk -F\' '{print $2}')
export C_INCLUDE_PATH="$INSTALL_PREFIX/include"
export LIBRARY_PATH="$INSTALL_PREFIX/lib:$INSTALL_PREFIX/lib64"
cd ../geopmdpy
python3 -m pip install .
set +x
echo "--------------------------------------------------------------------------"
echo "SUCCESS:"
echo "   Be sure to add the installed libgeopmd.so to your load library path:"
echo "   export LD_LIBRARY_PATH=${INSTALL_PREFIX}/lib"'${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}'
echo "--------------------------------------------------------------------------"
