#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
# Create VERSION file
if [ ! -e VERSION ]; then
    python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > VERSION
    if [ $? -ne 0 ]; then
        echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
    fi
fi
autoreconf -i -f


if ! which protoc >& /dev/null || \
   ! which grpc_cpp_plugin >& /dev/null; then
    echo "Error: Install the grpc and grpc development packages" 1>&2
    exit -1
fi

protoc --grpc_out src \
       --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
       --cpp_out src \
       --python_out ../geopmpy/geopmpy \
       geopm_runtime.proto

protoc --grpc_out ../geopmpy/geopmpy \
       --plugin=protoc-gen-grpc=$(which grpc_python_plugin) \
       geopm_runtime.proto

sed 's|import geopm_runtime_pb2|from . import geopm_runtime_pb2|' \
    -i ../geopmpy/geopmpy/geopm_runtime_pb2_grpc.py
