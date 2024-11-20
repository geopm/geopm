#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -lt 3 ]]; then
    echo "Usage: $0 JOBID PROMETHEUS_DIR GRAFANA_DIR"
    exit -1
fi
PROMETHEUS_DIR=$2
GRAFANA_DIR=$3

JOBID=$1
while [[ "$(qstat ${JOBID} | tail -n 1 | awk '{print $5}')" != "R" ]]; do
    sleep 1
done

VAR=$(qstat -x -f ${JOBID} | grep exec_host | awk '{print $3}')
NODES=$(python3 -c "ss=\"$VAR\";print(','.join([xx[0:xx.find('/')] + ':8080' for xx in ss.split('+')]))")

echo 'global:
  scrape_interval: 15s
  evaluation_interval: 15s
scrape_configs:
  - job_name: "prometheus"
    static_configs:
      - targets: '[${NODES}] > ${PROMETHEUS_DIR}/prometheus.yml
mkdir -p ${PROMETHEUS_DIR}/logs
mkdir -p ${PROMETHEUS_DIR}/data
nohup ${PROMETHEUS_DIR}/prometheus \
    --config.file ${PROMETHEUS_DIR}/prometheus.yml \
    --storage.tsdb.path ${PROMETHEUS_DIR}/data \
    --web.console.templates=${PROMETHEUS_DIR}/consoles \
    --web.console.libraries=${PROMETHEUS_DIR}/console_libraries \
    --web.listen-address=:8001 < /dev/null >& ${PROMETHEUS_DIR}/logs/prometheus-server-$(date +%F-%T-%Z).log &
PROM_PID=$!

mkdir -p ${GRAFANA_DIR}/logs
mkdir -p ${GRAFANA_DIR}/data
nohup ${GRAFANA_DIR}/bin/grafana server \
    --config=${GRAFANA_DIR}/conf/defaults.ini \
    --homepath=${GRAFANA_DIR} \
    cfg:default.paths.logs=${GRAFANA_DIR}/logs \
    cfg:default.paths.data=${GRAFANA_DIR}/data \
    cfg:default.paths.plugins=${GRAFANA_DIR}/plugins-bundled \
    cfg:default.paths.provisioning=${GRAFANA_DIR}/conf/provisioning < /dev/null >& ${GRAFANA_DIR}/logs/grafana-server-$(date +%F-%T-%Z).log &
GRAFANA_PID=$!

echo "Kill these PIDs before rerunning:"
echo
echo "    Prometheus PID: ${PROM_PID}"
echo "    Grafana PID: ${GRAFANA_PID}"
