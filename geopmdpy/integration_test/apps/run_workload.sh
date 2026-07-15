#!/bin/bash
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Run one of the GPU inference workload drivers in this directory, either
# inside the prebuilt Intel Extension for PyTorch container (default) or
# natively on the host.  The first argument is the driver's file name (e.g.
# ipex_resnet50_infer.py); all remaining arguments are forwarded to it.  Each
# driver prints a single "FOM (<unit>): <n>" line to stdout, which the GEOPM
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

if [[ $# -lt 1 ]]; then
    echo "usage: $(basename "$0") <driver.py> [driver-args...]" >&2
    exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Reduce to a bare file name so neither host paths nor traversal leak into the
# container path.
DRIVER="$(basename "$1")"
shift

IMAGE="${GEOPM_GPU_WORKLOAD_IMAGE:-intel/intel-extension-for-pytorch:2.8.10-xpu}"
ENGINE="${GEOPM_GPU_CONTAINER_ENGINE:-docker}"

if [[ "${GEOPM_GPU_WORKLOAD_NATIVE:-0}" == "1" ]]; then
    exec python3 "${SCRIPT_DIR}/${DRIVER}" "$@"
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
    python "/apps/${DRIVER}" "$@"
