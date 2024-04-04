#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
VERSION=$(python3 -c "from setuptools_scm import get_version; print(get_version())")
if [ $? -eq 0 ]; then
    echo ${VERSION} > VERSION
elif [ ! -e VERSION ]; then
    echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
    echo "0.0.0" > VERSION
fi
for SUBDIR in libgeopmd libgeopm geopmdpy/geopmdpy geopmpy/geopmpy; do
    cp VERSION ${SUBDIR}/VERSION
done
