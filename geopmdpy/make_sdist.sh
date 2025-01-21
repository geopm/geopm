#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if which protoc >& /dev/null; then
    if [ ! -e geopmdpy/geopm_service_pb2.py ] || [ ! -e geopmdpy/geopm_service_pb2_grpc.py ]; then
        cd ../libgeopmd && ./protoc-gen.sh && cd -
    fi
fi
python3 make_sdist.py | tee make_sdist.log
