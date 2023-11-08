FROM ubuntu:22.04 AS build

# TODO: Can we remove libsystemd-dev as a requirement when using a container+grpc?
# Note: python3-protobuf needs to be a version compatible with protobuf-compiler
RUN apt-get update && apt-get install -y --no-install-recommends \
      software-properties-common python3-pip wget automake autoconf \
      build-essential cargo gobject-introspection libgrpc++-dev libgrpc-dev \
      libnvidia-ml-dev libprotobuf-dev liburing-dev libtool pkgconf \
      protobuf-compiler protobuf-compiler-grpc python3 python3-all \
      python3-dev python3-setuptools unzip zstd libsystemd-dev \
      debhelper debhelper-compat dh-python \
      python3-protobuf \
    && rm -rf /var/lib/apt/lists/*

ARG GEOPM_COMMIT_HASH=a9f85959eb3bd65efc16402fc65429cac03840ba
RUN wget https://github.com/geopm/geopm/archive/${GEOPM_COMMIT_HASH}.tar.gz \
    && tar -xf ${GEOPM_COMMIT_HASH}.tar.gz && rm ${GEOPM_COMMIT_HASH}.tar.gz \
    && cd geopm-${GEOPM_COMMIT_HASH}/service \
    && pip install --no-cache-dir -r requirements.txt \
    # TODO: add grpcio to requirements.txt \
    && pip install --no-cache-dir grpcio==1.47.5 \
    && echo "3.0.0+geopm.${GEOPM_COMMIT_HASH}" > VERSION \
    && ./autogen.sh \
    && ./configure && sed -i 's/make -j/make/' debian/rules && make deb

FROM ubuntu:22.04 AS env
COPY --from=build geopm-*/service/*.deb /
RUN apt-get update && apt-get install -y --no-install-recommends /*.deb \
    python3-pip \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --no-cache-dir grpcio==1.47.5
RUN ldconfig && mkdir /etc/geopm -m 0700 \
    && groupadd -g 10001 client \
    && useradd -g 10001 -u 10001 -ms /bin/bash client
