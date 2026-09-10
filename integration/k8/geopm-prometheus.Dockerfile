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
    python3-build fakeroot python3-sdnotify && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy the entire geopm repository into the container
WORKDIR /src
COPY ../../../geopm /src/geopm

# Build GEOPM packages
WORKDIR /src/geopm/libgeopmd
RUN ./autogen.sh && ./configure && make deb
RUN apt-get install -yq --no-install-recommends ./*.deb

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
# Copy .deb packages from the build stage and install them
COPY --from=build /mnt/geopm-prometheus /mnt/geopm-prometheus
RUN apt-get update && \
    apt-get install -yq --no-install-recommends python3-grpcio /mnt/geopm-prometheus/*.deb && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf /mnt/geopm-prometheus

# Configure GEOPM: seed a blanket access list from the shipped all_names.txt.
# geopmaccess cannot enumerate or validate names when geopmd is launched with
# --grpc, so instead of granting a curated list we bake the full signal/control
# name list (docs/all_names.txt) into the default access files. geopmd reads
# these directly at startup; names that are unavailable on a given node are
# harmless extras. The final rm keeps a build-host topo cache out of the image
# (see comment below).
COPY --from=build /src/geopm/docs/all_names.txt /etc/geopm/all_names.txt
RUN mkdir -p /etc/geopm/0.DEFAULT_ACCESS && \
    grep -v '^[[:space:]]*#' /etc/geopm/all_names.txt | grep -v '^[[:space:]]*$' \
        > /etc/geopm/0.DEFAULT_ACCESS/allowed_signals && \
    cp /etc/geopm/0.DEFAULT_ACCESS/allowed_signals \
        /etc/geopm/0.DEFAULT_ACCESS/allowed_controls && \
    chmod 700 /etc/geopm /etc/geopm/0.DEFAULT_ACCESS && \
    chmod 600 /etc/geopm/0.DEFAULT_ACCESS/allowed_signals \
        /etc/geopm/0.DEFAULT_ACCESS/allowed_controls && \
    rm -f /tmp/geopm-topo-cache-* /run/geopm/geopm-topo-cache

# Any geopm tool that resolves the platform topology writes a topo cache
# (/tmp/geopm-topo-cache-<uid>) reflecting the host that ran it. Shipping a
# build-host cache bakes the wrong topology into the image: geopm's check_file()
# reuses any 0600 cache newer than the node's last boot, so it is never
# regenerated and the runtime sees the build host's socket/NUMA counts. The rm
# above is defensive -- never keep a topo cache in the image; let each node
# regenerate it from live hardware at runtime.
