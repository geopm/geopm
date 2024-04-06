#!/bin/bash

VERSION=$(python3 -c "from setuptools_scm import get_version; print(get_version('..'))")
echo $VERSION > VERSION
DATE=$(date +'%a, %d %b %Y %H:%M:%S %z')
cat debian/changelog.in | sed -e "s|@VERSION@|$VERSION|g" -e "s|@DATE@|$DATE|g" > debian/changelog
cd ..
tar --transform="s|docs|geopm-docs-$VERSION|" -zcvf geopm-docs-$VERSION.tar.gz docs
mv geopm-docs-$VERSION.tar.gz docs
cd docs
tar xvf geopm-docs-$VERSION.tar.gz
cd geopm-docs-$VERSION
PYTHONPATH=$PWD/../../geopmdpy:$PWD/../../geopmpy:$PYTHONPATH dpkg-buildpackage -us -uc
