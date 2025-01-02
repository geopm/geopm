#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 PROMETHEUS_DIR PROMETHEUS_SERVER_PORT GRAFANA_DIR GRAFANA_SERVER_PORT [GEOPM_DIR JOBID PROMETHEUS_CLIENT_PORT]"
    exit -1
fi
PROMETHEUS_DIR=$1
PROMETHEUS_SERVER_PORT=$2
GRAFANA_DIR=$3
GRAFANA_SERVER_PORT=$4
JOBID=0
if [[ $# -gt 6 ]]; then
    GEOPM_DIR=$5
    JOBID=$6
    PROMETHEUS_CLIENT_PORT=$7
fi

NODES=""
if [[ ${JOBID} != 0 ]]; then
    while [[ "$(qstat ${JOBID} | tail -n 1 | awk '{print $5}')" != "R" ]]; do
        sleep 1
    done
    VAR=$(qstat -x -f -w ${JOBID} | grep exec_host | awk '{print $3}')
    NODES=$(python3 -c "ss=\"$VAR\";print(','.join([xx[0:xx.find('/')] + ':$PROMETHEUS_CLIENT_PORT' for xx in ss.split('+')]))")
    NODEFILE=$(mktemp)
    python3 -c "ss=\"$VAR\";print('\\n'.join([xx[0:xx.find('/')] for xx in ss.split('+')]))" > ${NODEFILE}
    clush --hostfile=${NODEFILE} -- \
        env LD_LIBRARY_PATH=${GEOPM_DIR}/lib:${GEOPM_DIR}/lib64:${LD_LIBRARY_PATH} \
        geopmexporter -p ${PROMETHEUS_CLIENT_PORT} &
    CLUSH_PID=$!
fi

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
    --web.listen-address=:${PROMETHEUS_SERVER_PORT} < /dev/null >& \
    ${PROMETHEUS_DIR}/logs/prometheus-server-$(date +%F-%T-%Z).log &
PROM_PID=$!
echo "Prometheus PID: ${PROM_PID}"

mkdir -p ${GRAFANA_DIR}/logs
mkdir -p ${GRAFANA_DIR}/data
GRAFANA_CONF_PATH="${GRAFANA_DIR}/conf/${USER}_conf.ini"
cp ${GRAFANA_DIR}/conf/defaults.ini ${GRAFANA_CONF_PATH}
sed "s|http_port = [0-9]*|http_port = ${GRAFANA_SERVER_PORT}|" -i ${GRAFANA_CONF_PATH}

nohup ${GRAFANA_DIR}/bin/grafana server \
    --config=${GRAFANA_CONF_PATH} \
    --homepath=${GRAFANA_DIR} \
    cfg:default.paths.logs=${GRAFANA_DIR}/logs \
    cfg:default.paths.data=${GRAFANA_DIR}/data \
    cfg:default.paths.plugins=${GRAFANA_DIR}/plugins-bundled \
    cfg:default.paths.provisioning=${GRAFANA_DIR}/conf/provisioning < /dev/null >& \
    ${GRAFANA_DIR}/logs/grafana-server-$(date +%F-%T-%Z).log &
GRAFANA_PID=$!
echo "Grafana PID: ${GRAFANA_PID}"

if [[ "${JOBID}" != "0" ]]; then
    # clush will be killed when job completes
    wait ${CLUSH_PID}
    rm -f ${NODEFILE}
else
    read -p "Press enter to kill prometheus and grafana servers: "
fi
kill ${PROM_PID} ${GRAFANA_PID}
rm -f ${GRAFANA_CONF_PATH}
