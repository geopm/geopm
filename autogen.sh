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
    version=$(git describe --long | sed -e "s|\(.*\)-$release-$sha|\1|" -e "s|-|+|g" -e "s|^v||")
    if [ "${release}" != "0" ]; then
        version=${version}+dev${release}${sha}
    fi
    echo $version > VERSION
elif [ ! -f VERSION ]; then
    echo "WARNING:  VERSION file does not exist and git describe failed, setting verison to 0.0.0" 1>&2
    echo "0.0.0" > VERSION
fi

# Grab the first paragraph from the Summary and Description sections
# of the README to use in other files.
grep -A4096 '^SUMMARY$' README | tail -n+3 | grep -m1 '^$' -B4096 | head -n-1 | head -c-1 | sed 's/$/  /' > BLURB
grep -A4096 '^DESCRIPTION$' README | tail -n+3 | grep -m1 '^$' -B4096 | head -n-1 >> BLURB

if [ -f .git/config ] || git rev-parse --git-dir > /dev/null 2>&1; then
    git ls-tree --full-tree -r HEAD | awk '{print $4}' | sort > MANIFEST
fi
if [ ! -f MANIFEST ]; then
    echo "WARNING: MANIFEST file does not exist and working directory is not a git repository, creating with find" 1>&2
    find . -type f | sed 's|^\./||' | sort > MANIFEST
fi

mkdir -p m4
autoreconf -i -f
