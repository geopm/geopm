#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


if [[ $# -eq 0 ]] || [[ $1 == '--help' ]]; then
    printf "Usage: $0 --prefix=INSTALL_PREFIX ...

Overview:

When using pip to install a development version of geopmdpy, the ABI of
libgeopmd.so used at runtime must provide all the interfaces required by
geompdpy to avoid an import error.  The easiest way to ensure this is the case
is to install a version of libgeopmd.so that matches the pip installed version.

This script enables this use by by installing geopmdpy and libgeopmd.so based on
latest upstream dev branch.  All command line arguments provided to this script are
forwarded to the configure command for libgeopmd.  User is expected to provide
the --prefix option which will install libgeopmd in a user defined directory.
The user may also provide other options as required by their environment.  For
more details on how to configure libgeopmd.so see:

    https://geopm.github.io/devel.html#configuring-the-build

The script will clone a new GEOPM git repo called \"install_user_geopm_git\" if
this directory does not exist in the user's current working directory.

Environment Variables:

    GEOPM_GIT_URL: URL to clone if install_user_geopm_git directory does not exist.
                   Defaults to \"https://github.com/geopm/geopm.git\".

    GEOPM_GIT_CHECKOUT: Branch, sha or other commit-ish to checkout from the
                        repo.  Defaults to \"dev\".  Rebase against origin is
                        disabled when this environment variable is set.

Example:"'

    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
    chmod a+x install_user.sh
    ./install_user.sh --prefix=$HOME/geopm-build
    export LD_LIBRARY_PATH=$HOME/geopm-build/lib:$LD_LIBRARY_PATH


'
    exit 0
fi
set -x
set -e
if [ -z "${GEOPM_GIT_URL}" ]; then
    GEOPM_GIT_URL="https://github.com/geopm/geopm.git"
fi
if [ -z "${GEOPM_GIT_CHECKOUT}" ]; then
    GEOPM_GIT_CHECKOUT="dev"
    DO_REBASE="true"
fi
if [ ! -d install_user_geopm_git ]; then
    git clone ${GEOPM_GIT_URL} install_user_geopm_git
fi
cd install_user_geopm_git
git checkout ${GEOPM_GIT_CHECKOUT}
if [ ${DO_REBASE} == "true" ]; then
    git fetch origin
    git rebase origin/${GEOPM_GIT_CHECKOUT}
fi
git clean -dfx
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
echo "   export LD_LIBRARY_PATH=${INSTALL_PREFIX}/lib:\$LD_LIBRARY_PATH"
echo "--------------------------------------------------------------------------"
