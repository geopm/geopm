#!/bin/bash

cd ..
VERSION=$(python3 -c "from setuptools_scm import get_version; print(get_version('.'))")
tar --transform="s|docs|geopm-docs-$VERSION|" -zcvf geopm-docs-$VERSION.tar.gz docs
mv geopm-docs-$VERSION.tar.gz docs
cd docs
tar xvf geopm-docs-$VERSION.tar.gz
cd geopm-docs-$VERSION
PYTHONPATH=$PWD/../../geopmdpy:$PWD/../../geopmpy:$PYTHONPATH dpkg-buildpackage -us -uc
