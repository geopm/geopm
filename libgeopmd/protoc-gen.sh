#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


if ! which protoc >& /dev/null || \
   ! which grpc_cpp_plugin >& /dev/null; then
    echo "Error: Install the grpc and grpc development packages" 1>&2
    exit -1
fi

protoc --grpc_out src \
       --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
       --cpp_out src \
       geopm_service.proto
