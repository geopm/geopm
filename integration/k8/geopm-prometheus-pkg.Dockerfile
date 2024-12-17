FROM ubuntu:22.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -yq software-properties-common python3-pip git build-essential libcap-dev \
                        libnvidia-ml-dev libgrpc-dev libgrpc++-dev libprotobuf-dev libprotoc-dev \
			libsystemd-dev liburing-dev libtool pkgconf protobuf-compiler \
			protobuf-compiler-grpc unzip zlib1g-dev python3-all python3-setuptools \
			python3-setuptools-scm wget debhelper-compat dh-python curl zstd python3-cffi
RUN python3 -m pip install build
RUN apt-get install -yq gpg
RUN wget -qO - https://repositories.intel.com/gpu/intel-graphics.key | gpg --yes --dearmor --output /usr/share/keyrings/intel-graphics.gpg
RUN echo "deb [arch=amd64,i386 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/gpu/ubuntu jammy client" | \
  tee /etc/apt/sources.list.d/intel-gpu-jammy.list
RUN apt-get update
RUN apt-get install -yq libze1 libze-dev
RUN git clone https://github.com/cmcantalupo/geopm.git
RUN cd geopm && git checkout prometheus
RUN cd geopm/libgeopmd && ./autogen.sh && ./configure && ENABLE_LEVELZERO=TRUE make deb
RUN cd geopm/libgeopmd && apt-get install -yq ./libgeopmd*.deb
RUN cd geopm/geopmdpy && ./make_deb.sh
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustup update stable
RUN cargo install cargo-deb
RUN cd geopm/geopmdrs && ./build.sh
RUN mkdir -p /mnt/geopm-prometheus && cp -p $(find -name \*.deb) /mnt/geopm-prometheus
