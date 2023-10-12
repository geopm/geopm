FROM ubuntu:22.04
# Make sure the linker can find libcudnn.so
ENV LD_LIBRARY_PATH "$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu/:/usr/local/cuda/lib64/"
ADD https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb cuda-keyring_1.1-1_all.deb
# Specifically using cudnn-8.6.0 for compatibility with the older libtorch we need (discussed below)
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates linux-headers-$(uname -r) \
    && dpkg -i cuda-keyring_1.1-1_all.deb && apt-get update \
    && apt-get install -y --no-install-recommends python3 python3-dev python3-pip \
       cuda-cudart-11-8 cuda-cudart-dev-11-8 libcudnn8=8.6.0.*-1+cuda11.8 libcudnn8-dev=8.6.0.*-1+cuda11.8 \
       libcublas-11-8 libcublas-dev-11-8 \
       libcufft-11-8 libcufft-dev-11-8 \
       libcurand-11-8 libcurand-dev-11-8 \
       libcusolver-11-8 libcusolver-dev-11-8 \
       libcusparse-11-8 libcusparse-dev-11-8 \
       cuda-nvcc-11-8 \
       libhdf5-dev gcc mpich libmpich-dev make cmake wget \
    && rm -rf /var/lib/apt/lists/*
RUN wget https://github.com/coreyjadams/CosmicTagger/archive/refs/tags/v1.1.0.tar.gz \
    && tar -xf v1.1.0.tar.gz && rm v1.1.0.tar.gz
# Tricky bits in the installation order:
# - Need to stick withh old tensorflow to work with CUDA11, or else we get
#   "already been registered" errors.
# - Install horovod *after* tensorflow so it sees the one we install.
# - torch 1.13.1: latest version that supports c++14, needed by horovod 0.28.1
RUN pip install --no-cache-dir scikit-build numpy mpi4py decorator tensorflow[and-cuda] cloudpickle torch==1.13.1 \
    && HOROVOD_WITH_PYTORCH=1 HOROVOD_WITH_TENSORFLOW=1 pip install --no-cache-dir horovod \
    && pip install --no-cache-dir -r CosmicTagger-1.1.0/requirements.txt
