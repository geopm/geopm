#!/bin/bash
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Run one of the GPU inference workload drivers in this directory.  The default
# driver is a local SYCL/oneAPI benchmark that is built on demand and requires
# no container or network access.  Python drivers are also available for
# optional container/native experiments.  The first argument is the driver's
# file name or binary name; all remaining arguments are forwarded to it.  Each
# driver prints a single "FOM (<unit>): <n>" line to stdout, which the GEOPM
# integration test captures.
#
# Environment variables:
#   GEOPM_GPU_WORKLOAD_NATIVE   1 => run Python drivers natively instead of in a
#                               container.
#   GEOPM_GPU_BENCH_CXX         SYCL compiler for the local benchmark.
#   GEOPM_GPU_BENCH_BUILD_DIR   Local benchmark build directory.
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

if [[ "${DRIVER}" == "gpu_activity_benchmark" ]]; then
    if [[ -z "${ONEAPI_DEVICE_SELECTOR:-}" ]]; then
        export ONEAPI_DEVICE_SELECTOR="level_zero:gpu"
    fi
    if [[ -z "${SYCL_DEVICE_FILTER:-}" ]]; then
        export SYCL_DEVICE_FILTER="level_zero:gpu"
    fi
    # The benchmark is compiled by the test harness (apps/Makefile); this
    # script only launches the prebuilt binary.
    BUILD_DIR="${GEOPM_GPU_BENCH_BUILD_DIR:-${SCRIPT_DIR}/build}"
    BENCHMARK="${BUILD_DIR}/gpu_activity_benchmark"
    if [[ ! -x "${BENCHMARK}" ]]; then
        echo "benchmark not built: ${BENCHMARK} (run 'make -C ${SCRIPT_DIR}')" >&2
        exit 1
    fi
    exec "${BENCHMARK}" "$@"
fi

IMAGE="${GEOPM_GPU_WORKLOAD_IMAGE:-intel/intel-extension-for-pytorch:2.8.10-xpu}"
ENGINE="${GEOPM_GPU_CONTAINER_ENGINE:-docker}"

if [[ "${GEOPM_GPU_WORKLOAD_NATIVE:-0}" == "1" ]]; then
    exec python3 "${SCRIPT_DIR}/${DRIVER}" "$@"
fi

# Base container run arguments: expose the GPU render nodes (/dev/dri) to the
# container so the workload can reach the GPU, and bind-mount this apps
# directory read-only at /apps so the driver script is available inside.
engine_args=(run --rm --device /dev/dri -v "${SCRIPT_DIR}:/apps:ro")

render_gid="$(getent group render | cut -d: -f3 || true)"
if [[ -n "${render_gid}" ]]; then
    engine_args+=(--group-add "${render_gid}")
fi

# On SELinux hosts the read-only bind mount is relabeled by default, which can
# deny access under rootless podman; disabling the label opt-in avoids that.
if [[ -n "${GEOPM_GPU_SELINUX_DISABLE:-}" ]]; then
    engine_args+=(--security-opt label=disable)
fi

exec "${ENGINE}" "${engine_args[@]}" "${IMAGE}" \
    python3 "/apps/${DRIVER}" "$@"
