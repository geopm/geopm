#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

if [[ $# -eq 0 ]]; then
    echo 'Building from 40c5c1a'
    TOPHASH=40c5c1a
elif [[ $# -eq 1 ]]; then
    echo 'Building from head of main'
    TOPHASH=$1
else
    1>&2 echo "usage: $0 [HASH_FOR_HEAD_OF_MAIN]"
    exit 1
fi

DIRNAME=CosmicTagger
GITREPO=https://github.com/coreyjadams/CosmicTagger
clean_source ${DIRNAME}

ARCHIVE=${DIRNAME}_${TOPHASH}.tgz
get_archive ${ARCHIVE}
if [ -f ${ARCHIVE} ]; then
        unpack_archive ${ARCHIVE}
else
        clone_repo_git ${GITREPO} ${DIRNAME} ${TOPHASH}
fi

setup_source_git ${DIRNAME}

# install dependencies
cd ${DIRNAME}
python3 -m pip install scikit-build numpy --user --force-reinstall
python3 -m pip install -r requirements.txt --ignore-installed
python3 -m pip install mpi4py --user --force-reinstall
python3 -m pip install decorator --user --force-reinstall
python3 -m pip install tensorflow --user --force-reinstall
python3 -m pip install cloudpickle --user --force-reinstall
python3 -m pip install horovod --user --force-reinstall

# setup small dataset
cp example_data/cosmic_tagging_light.h5 example_data/cosmic_tagging_val.h5
cd ../
