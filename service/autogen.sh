#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# If the VERSION file does not exist, then create it based on git
# describe or if not in a git repo just set VERSION to 0.0.0.
if [ -f VERSION_OVERRIDE ]; then
    cp VERSION_OVERRIDE VERSION
elif git describe --long > /dev/null 2>&1; then
    sha=$(git describe --long | awk -F- '{print $(NF)}')
    release=$(git describe --long | awk -F- '{print $(NF-1)}')
    version=$(git describe --long | sed -e "s|+rc|~rc|" -e "s|\(.*\)-$release-$sha|\1|" -e "s|-|+|g" -e "s|^v||")
    if [ "${release}" != "0" ]; then
        version=${version}+dev${release}${sha}
    fi
    echo $version > VERSION
elif [ ! -f VERSION ]; then
    echo "WARNING:  VERSION file does not exist and git describe failed, setting version to 0.0.0" 1>&2
    echo "0.0.0" > VERSION
fi

for ff in AUTHORS CODE_OF_CONDUCT.md CONTRIBUTING.rst COPYING COPYING-TPP; do
    if [ ! -f $ff ]; then
        cp ../$ff .
    fi
done

./protoc-gen.sh

autoreconf -i -f
