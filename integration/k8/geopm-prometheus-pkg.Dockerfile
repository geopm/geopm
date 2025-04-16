FROM ubuntu:24.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -yq --no-install-recommends \
    software-properties-common python3-pip git build-essential libcap-dev \
    libnvidia-ml-dev libgrpc-dev libgrpc++-dev libprotobuf-dev libprotoc-dev \
    libsystemd-dev liburing-dev libtool pkgconf protobuf-compiler \
    protobuf-compiler-grpc unzip zlib1g-dev python3-all python3-setuptools \
    python3-setuptools-scm wget debhelper-compat dh-python curl zstd \
    python3-cffi gpg libze1 libze-dev python3-dev python3-defusedxml \
    python3-build fakeroot
RUN useradd -ms /bin/bash build
USER build
WORKDIR /home/build
RUN git clone https://github.com/geopm/geopm.git
WORKDIR /home/build/geopm/libgeopmd
RUN ./autogen.sh && ./configure && ENABLE_LEVELZERO=TRUE make deb
USER root
RUN apt-get install -yq --no-install-recommends /home/build/geopm/libgeopmd/libgeopmd*.deb
USER build
WORKDIR /home/build/geopm/geopmdpy
RUN ./make_deb.sh
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/home/build/.cargo/bin:${PATH}"
RUN rustup update stable
RUN cargo install cargo-deb
WORKDIR /home/build/geopm/geopmdrs
RUN ./build.sh
USER root
RUN mkdir -p /mnt/geopm-prometheus && cp -p $(find /home/build/geopm -name \*.deb) /mnt/geopm-prometheus
