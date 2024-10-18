FROM ubuntu:22.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -yq software-properties-common python3-pip
RUN python3 -m pip install --ignore-installed grpcio==1.47.5
COPY geopm-prometheus /mnt/geopm-prometheus
RUN apt-get install -yq /mnt/geopm-prometheus/*.deb
RUN printf \
"CPU_CORE_TEMPERATURE\nCPU_ENERGY\nCPU_FREQUENCY_STATUS\n"\
"CPU_PACKAGE_TEMPERATURE\nCPU_POWER\nCPU_UNCORE_FREQUENCY_STATUS\n"\
"DRAM_ENERGY\nDRAM_POWER\nGPU_CORE_FREQUENCY_STATUS\nGPU_ENERGY\n"\
"GPU_POWER\nGPU_TEMPERATURE\n" | \
    geopmaccess --direct --force --write --default
