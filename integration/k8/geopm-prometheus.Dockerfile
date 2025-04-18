# Stage 1: Build container
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -yq --no-install-recommends \
    software-properties-common python3-pip git build-essential libcap-dev \
    libnvidia-ml-dev libgrpc-dev libgrpc++-dev libprotobuf-dev libprotoc-dev \
    libsystemd-dev liburing-dev libtool pkgconf protobuf-compiler \
    protobuf-compiler-grpc unzip zlib1g-dev python3-all python3-setuptools \
    python3-setuptools-scm wget debhelper-compat dh-python curl zstd \
    python3-cffi libze1 libze-dev python3-dev python3-defusedxml \
    python3-build fakeroot && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy the current repository context into the container
WORKDIR /src
COPY . /src

# Build GEOPM packages
WORKDIR /src/geopm/libgeopmd
RUN ./autogen.sh && ./configure && ENABLE_LEVELZERO=TRUE make deb

WORKDIR /src/geopm/geopmdpy
RUN ./make_deb.sh

WORKDIR /src/geopm/geopmdrs
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && \
    export PATH="/root/.cargo/bin:${PATH}" && \
    rustup update stable && \
    cargo install cargo-deb && \
    ./build.sh

# Collect all .deb packages
RUN mkdir -p /mnt/geopm-prometheus && \
    cp -p $(find /src/geopm -name \*.deb) /mnt/geopm-prometheus

# Stage 2: Runtime container
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -yq --no-install-recommends gpg wget software-properties-common python3-pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy .deb packages from the build stage and install them
COPY --from=build /mnt/geopm-prometheus /mnt/geopm-prometheus
RUN apt-get install -yq --no-install-recommends /mnt/geopm-prometheus/*.deb python3-grpcio && \
    rm -rf /mnt/geopm-prometheus

# Configure GEOPM
RUN printf \
"CPU_CORE_TEMPERATURE\nCPU_ENERGY\nCPU_FREQUENCY_STATUS\n"\
"CPU_PACKAGE_TEMPERATURE\nCPU_POWER\nCPU_UNCORE_FREQUENCY_STATUS\n"\
"DRAM_ENERGY\nDRAM_POWER\nGPU_CORE_FREQUENCY_STATUS\nGPU_ENERGY\n"\
"GPU_POWER\nGPU_TEMPERATURE\n" | \
    geopmaccess --direct --force --write --default && \
    printf "" | geopmaccess --direct --force --write --default --controls
