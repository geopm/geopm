#!/bin/bash

if [[ -z "$GEOPM_DIR" ]]; then
    echo "GEOPM_DIR must be set in environment" 1>&2
    exit -1
fi
clush --hostfile=$PBS_NODEFILE -- \
    env LD_LIBRARY_PATH=$GEOPM_DIR/lib:$LD_LIBRARY_PATH \
    geopmexporter -p 8080  &

sleep 600
