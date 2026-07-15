#!/bin/bash
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Run the ResNet-50 inference benchmark, either inside the prebuilt Intel
# Extension for PyTorch container (default) or natively on the host.  All
# arguments are forwarded to ipex_resnet50_infer.py.  The benchmark prints
# a single "FOM (images/sec): <n>" line to stdout, which the GEOPM
# integration test captures.
#
# GEOPM controls GPU frequency on the host at the hardware level, so a
# containerized workload is transparently affected by the agent; no GEOPM
# components are needed inside the container.
#
# Environment variables:
#   GEOPM_GPU_WORKLOAD_NATIVE   1 => run natively instead of in a container.
#   GEOPM_GPU_CONTAINER_ENGINE  Container engine to use. Default: docker.
#   GEOPM_GPU_WORKLOAD_IMAGE    Container image. Default: pinned IPEX xpu tag.
#   GEOPM_GPU_SELINUX_DISABLE   Non-empty => add --security-opt label=disable
#                               (useful for rootless podman on SELinux hosts).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE="${GEOPM_GPU_WORKLOAD_IMAGE:-intel/intel-extension-for-pytorch:2.8.10-xpu}"
ENGINE="${GEOPM_GPU_CONTAINER_ENGINE:-docker}"

if [[ "${GEOPM_GPU_WORKLOAD_NATIVE:-0}" == "1" ]]; then
    exec python3 "${SCRIPT_DIR}/ipex_resnet50_infer.py" "$@"
fi

engine_args=(run --rm --device /dev/dri -v "${SCRIPT_DIR}:/apps:ro")

render_gid="$(getent group render | cut -d: -f3 || true)"
if [[ -n "${render_gid}" ]]; then
    engine_args+=(--group-add "${render_gid}")
fi

if [[ -n "${GEOPM_GPU_SELINUX_DISABLE:-}" ]]; then
    engine_args+=(--security-opt label=disable)
fi

exec "${ENGINE}" "${engine_args[@]}" "${IMAGE}" \
    python /apps/ipex_resnet50_infer.py "$@"
