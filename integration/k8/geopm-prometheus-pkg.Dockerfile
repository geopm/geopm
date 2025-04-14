FROM ubuntu:22.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -yq software-properties-common python3-pip git build-essential libcap-dev \
                        				  libnvidia-ml-dev libgrpc-dev libgrpc++-dev libprotobuf-dev libprotoc-dev \
										  libsystemd-dev liburing-dev libtool pkgconf protobuf-compiler \
										  protobuf-compiler-grpc unzip zlib1g-dev python3-all python3-setuptools \
										  python3-setuptools-scm wget debhelper-compat dh-python curl zstd python3-cffi
RUN python3 -m pip install build
RUN apt-get install -yq gpg
RUN curl -fsSL https://repositories.intel.com/gpu/intel-graphics.key | gpg --yes --dearmor --output /usr/share/keyrings/intel-graphics.gpg
RUN echo "deb [arch=amd64,i386 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/gpu/ubuntu jammy client" | \
  tee /etc/apt/sources.list.d/intel-gpu-jammy.list
RUN apt-get update && apt-get install -yq libze1 libze-dev
RUN git clone https://github.com/geopm/geopm.git
WORKDIR /geopm/libgeopmd
RUN ./autogen.sh && ./configure && ENABLE_LEVELZERO=TRUE make deb
RUN apt-get install -yq ./libgeopmd*.deb
WORKDIR /geopm/geopmdpy
RUN ./make_deb.sh
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustup update stable
RUN cargo install cargo-deb
WORKDIR /geopm/geopmdrs
RUN ./build.sh
RUN mkdir -p /mnt/geopm-prometheus && cp -p $(find -name \*.deb) /mnt/geopm-prometheus

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 CMD test -d /mnt/geopm-prometheus || exit 1
