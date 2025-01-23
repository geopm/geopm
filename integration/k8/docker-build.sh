#!/bin/bash

set -e
set -x

docker build . -f geopm-prometheus-pkg.Dockerfile -t geopm-prometheus-pkg >& geopm-prometheus-pkg.log
id=$(docker create geopm-prometheus-pkg)
rm -rf geopm-prometheus
docker cp ${id}:/mnt/geopm-prometheus geopm-prometheus
docker rm -v ${id}
docker build . -f geopm-prometheus.Dockerfile -t geopm-prometheus  >& geopm-prometheus.log

