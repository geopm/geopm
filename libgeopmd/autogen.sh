#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
# Create VERSION file
if [ ! -e VERSION ]; then
    python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > VERSION
    if [ $? -ne 0 ]; then
	set +x
        echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
	set -x
    fi
fi
autoreconf -i -f
if which protoc >& /dev/null; then
    ./protoc-gen.sh
else
    touch src/geopm_service.grpc.pb.cc \
          src/geopm_service.grpc.pb.h \
          src/geopm_service.pb.cc \
          src/geopm_service.pb.h
    set +x
    echo "Warning: No grpc support, install grpc development package to enable. The configure --enable-grpc option will fail." 1>&2
fi
