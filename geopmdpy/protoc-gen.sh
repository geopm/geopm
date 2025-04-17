#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


if ! which protoc >& /dev/null || \
   ! which grpc_python_plugin >& /dev/null; then
    echo "Error: Install the grpc and grpc development packages" 1>&2
    exit -1
fi

protoc --plugin=protoc-gen-grpc=$(which grpc_python_plugin) \
       --grpc_out geopmdpy \
       --python_out geopmdpy \
       geopm_service.proto

sed 's|import geopm_service_pb2|from . import geopm_service_pb2|' \
    -i geopmdpy/geopm_service_pb2_grpc.py
