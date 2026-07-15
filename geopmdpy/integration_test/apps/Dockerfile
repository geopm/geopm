#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Optional "bake-in" image: copies the benchmark driver into Intel's
# prebuilt Extension-for-PyTorch (xpu) image so the workload can be run with
# a single `docker run <image>` and no volume mount.  Build with:
#
#   docker build -t geopm-resnet50-infer \
#       geopm/geopmdpy/integration_test/apps
#
# then set GEOPM_GPU_WORKLOAD_IMAGE=geopm-resnet50-infer (the wrapper still
# supplies the --device /dev/dri and --group-add render flags).
FROM intel/intel-extension-for-pytorch:2.8.10-xpu
COPY ipex_resnet50_infer.py /apps/ipex_resnet50_infer.py
ENTRYPOINT ["python", "/apps/ipex_resnet50_infer.py"]
