FROM ubuntu:24.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -yq --no-install-recommends gpg wget software-properties-common python3-pip
COPY geopm-prometheus /mnt/geopm-prometheus
RUN apt-get install -yq --no-install-recommends /mnt/geopm-prometheus/*.deb python3-grpcio
RUN printf \
"CPU_CORE_TEMPERATURE\nCPU_ENERGY\nCPU_FREQUENCY_STATUS\n"\
"CPU_PACKAGE_TEMPERATURE\nCPU_POWER\nCPU_UNCORE_FREQUENCY_STATUS\n"\
"DRAM_ENERGY\nDRAM_POWER\nGPU_CORE_FREQUENCY_STATUS\nGPU_ENERGY\n"\
"GPU_POWER\nGPU_TEMPERATURE\n" | \
    geopmaccess --direct --force --write --default
RUN printf "" | geopmaccess --direct --force --write --default --controls
