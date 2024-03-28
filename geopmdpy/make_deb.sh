#!/bin/bash

set -x -e
python3 -m build --sdist > make_deb-sdist.log
archive=$(cat make_deb-sdist.log | tail -n 1 | sed 's|^Successfully built ||')
tar -xvf dist/$archive
dir=$(echo $archive | sed 's|\.tar\.gz||')
cd $dir
dpkg-buildpackage -us -uc
