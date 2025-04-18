#!/bin/bash

set -e
set -x

docker build ../../.. -f geopm-prometheus.Dockerfile -t geopm-prometheus >& geopm-prometheus.log
