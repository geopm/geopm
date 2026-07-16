#!/bin/bash
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Build the local SYCL/oneAPI GPU activity benchmark used by the integration
# tests.  No network access or container image is required.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="${SCRIPT_DIR}/gpu_activity_benchmark.cpp"
BUILD_DIR="${GEOPM_GPU_BENCH_BUILD_DIR:-${SCRIPT_DIR}/build}"
BIN="${BUILD_DIR}/gpu_activity_benchmark"
CXX="${GEOPM_GPU_BENCH_CXX:-}"

if [[ -z "${CXX}" ]]; then
    for candidate in icpx dpcpp; do
        if command -v "${candidate}" >/dev/null 2>&1; then
            CXX="${candidate}"
            break
        fi
    done
fi

if [[ -z "${CXX}" ]]; then
    echo "No SYCL compiler found. Set GEOPM_GPU_BENCH_CXX or load the Intel oneAPI compiler environment so icpx/dpcpp is on PATH." >&2
    exit 1
fi

mkdir -p "${BUILD_DIR}"

if [[ ! -x "${BIN}" || "${SRC}" -nt "${BIN}" || "$0" -nt "${BIN}" ]]; then
    echo "Building ${BIN} with ${CXX}" >&2
    "${CXX}" -std=c++17 -O2 -fsycl "${SRC}" -o "${BIN}"
fi

printf '%s\n' "${BIN}"
