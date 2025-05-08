# Stage 1: Build container
FROM fedora:42 AS build

RUN dnf -y update && \
    dnf install -y autoconf automake gcc-c++ glibc-devel gmock-devel gtest-devel \
                   libcap-devel libtool liburing-devel systemd-devel zlib-ng-compat-devel \
                   grpc-devel protobuf-devel awk rpmbuild lscpu python3-setuptools_scm \
                   python3-build python3-cffi python3-dasbus python3-defusedxml \
                   python3-devel python3-jsonschema python3-psutil python3-setuptools \
                   which systemd-units elfutils-libelf-devel python3-pandas \
                   python3-natsort python3-pyyaml python3-tables python3-numpy && \
    mkdir -p /mnt/geopm-fedora && \
    chmod a+rwx /mnt/geopm-fedora && \
    useradd -m build

USER build
ENV PACKAGING_URL=https://raw.githubusercontent.com/geopm/geopm/refs/heads/release-v3.2-packaging/release/fedora
WORKDIR /home/build
RUN mkdir -p rpmbuild/SOURCES && \
    mkdir -p rpmbuild/SPECS && \
    curl -sL https://github.com/geopm/geopm/archive/v3.2.0/geopm-3.2.0.tar.gz > \
        rpmbuild/SOURCES/geopm-3.2.0.tar.gz && \
    curl -sL ${PACKAGING_URL}/libgeopmd.spec > \
        rpmbuild/SPECS/libgeopmd.spec && \
    curl -sL ${PACKAGING_URL}/geopmd.spec > \
        rpmbuild/SPECS/geopmd.spec && \
    curl -sL ${PACKAGING_URL}/libgeopm.spec > \
       rpmbuild/SPECS/libgeopm.spec && \
    curl -sL ${PACKAGING_URL}/python-geopmpy.spec > \
        rpmbuild/SPECS/python-geopmpy.spec && \
    curl -sL ${PACKAGING_URL}/0001-Avoid-Wnon-virtual-dtor-option-in-CFLAGS.patch > \
        rpmbuild/SOURCES/0001-Avoid-Wnon-virtual-dtor-option-in-CFLAGS.patch && \
    curl -sL ${PACKAGING_URL}/0002-Allow-numpy-2.0-and-higher.patch > \
        rpmbuild/SOURCES/0002-Allow-numpy-2.0-and-higher.patch && \
    curl -sL ${PACKAGING_URL}/0003-Define-macro-to-set-defaults-used-by-init-function.patch > \
        rpmbuild/SOURCES/0003-Define-macro-to-set-defaults-used-by-init-function.patch && \
    tar xf rpmbuild/SOURCES/geopm-3.2.0.tar.gz && \
    rpmbuild -ba rpmbuild/SPECS/libgeopmd.spec

USER root
RUN dnf install -y rpmbuild/RPMS/*/libgeopmd-3.2.0*.rpm rpmbuild/RPMS/*/libgeopmd-devel-3.2.0*.rpm

USER build
RUN rpmbuild -ba rpmbuild/SPECS/geopmd.spec && \
    pushd geopm-3.2.0/geopmdrs && \
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && \
    export PATH="$HOME/.cargo/bin:${PATH}" && \
    sed -e "s|@VERSION@|3.2.0|" Cargo.toml.in > Cargo.toml && \
    cargo vendor && \
    cargo build --release && \
    cp -p target/release/geopmd-proxy /mnt/geopm-fedora/geopmd-proxy && \
    popd && \
    rpmbuild -ba rpmbuild/SPECS/libgeopm.spec

USER root
RUN dnf install -y rpmbuild/RPMS/*/libgeopm-3.2.0*.rpm rpmbuild/RPMS/*/libgeopm-devel-3.2.0*.rpm \
                   rpmbuild/RPMS/*/python3-geopmdpy-3.2.0*.rpm rpmbuild/RPMS/*/geopmd-cli-3.2.0*.rpm

USER build
RUN rpmbuild -ba rpmbuild/SPECS/python-geopmpy.spec && \
    cp -p rpmbuild/RPMS/*/*.rpm /mnt/geopm-fedora

# Stage 2: Runtime container
FROM fedora:42

# Copy .rpm packages from the build stage and install them
COPY --from=build /mnt/geopm-fedora /mnt/geopm-fedora
RUN dnf update -y && \
    dnf install --setopt=install_weak_deps=False -y util-linux  \
                                                    /mnt/geopm-fedora/libgeopmd-3.2.0*.rpm \
                                                    /mnt/geopm-fedora/geopmd-cli-3.2.0*.rpm \
                                                    /mnt/geopm-fedora/python3-geopmdpy-3.2.0*.rpm && \
    dnf clean all && \
    install /mnt/geopm-fedora/geopmd-proxy /usr/bin/geopmd-proxy && \
    rm -rf /mnt/geopm-fedora

# Configure GEOPM
RUN printf \
"CPU_CORE_TEMPERATURE\nCPU_ENERGY\nCPU_FREQUENCY_STATUS\n"\
"CPU_PACKAGE_TEMPERATURE\nCPU_POWER\nCPU_UNCORE_FREQUENCY_STATUS\n"\
"DRAM_ENERGY\nDRAM_POWER\nGPU_CORE_FREQUENCY_STATUS\nGPU_ENERGY\n"\
"GPU_POWER\nGPU_TEMPERATURE\n" | \
    geopmaccess --direct --force --write --default && \
    printf "" | geopmaccess --direct --force --write --default --controls
