#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


if ! which protoc >& /dev/null || \
       ! which grpc_cpp_plugin >& /dev/null || \
       ! which grpc_python_plugin >& /dev/null; then
    echo "Error: Install the grpc and grpc development packages" 1>&2
    exit -1
fi

protoc --grpc_out src \
       --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
       --cpp_out src \
       --python_out ../geopmdpy/geopmdpy \
       geopm_service.proto

protoc --grpc_out ../geopmdpy/geopmdpy \
       --plugin=protoc-gen-grpc=$(which grpc_python_plugin) \
       geopm_service.proto

sed 's|import geopm_service_pb2|from . import geopm_service_pb2|' \
    -i ../geopmdpy/geopmdpy/geopm_service_pb2_grpc.py
