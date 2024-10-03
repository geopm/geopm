#!/bin/bash

set -e
set -x

docker build . -f geopm-prometheus-pkg.Dockerfile >& geopm-prometheus-pkg.log
id=$(grep "writing image sha256:" geopm-prometheus-pkg.log | tail -n 1 | awk -F: '{print $2}' | awk '{print $1}')
docker tag ${id} geopm-prometheus-pkg
id=$(docker create geopm-prometheus-pkg)
rm -rf geopm-prometheus
docker cp ${id}:/mnt/geopm-prometheus geopm-prometheus
docker rm -v ${id}
docker build . -f geopm-prometheus.Dockerfile >& geopm-prometheus.log
id=$(grep "writing image sha256:" geopm-prometheus.log | tail -n 1 | awk -F: '{print $2}' | awk '{print $1}')
docker tag ${id} geopm-prometheus
