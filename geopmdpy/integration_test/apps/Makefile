#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Build the local SYCL/oneAPI GPU activity benchmark used by the integration
# tests.  No network access or container image is required.
#
# Honored variables (also settable in the environment):
#   GEOPM_GPU_BENCH_CXX        SYCL compiler (default: icpx, else dpcpp on PATH)
#   GEOPM_GPU_BENCH_BUILD_DIR  output directory (default: ./build)

BUILD_DIR := $(if $(GEOPM_GPU_BENCH_BUILD_DIR),$(GEOPM_GPU_BENCH_BUILD_DIR),build)

# Prefer GEOPM_GPU_BENCH_CXX; otherwise pick the first SYCL compiler on PATH.
# A direct assignment is used so the built-in default CXX (g++) is overridden.
ifeq ($(strip $(GEOPM_GPU_BENCH_CXX)),)
CXX := $(shell command -v icpx 2>/dev/null || command -v dpcpp 2>/dev/null)
else
CXX := $(GEOPM_GPU_BENCH_CXX)
endif

CXXFLAGS ?= -std=c++17 -O2 -fsycl

SRC := gpu_activity_benchmark.cpp
BIN := $(BUILD_DIR)/gpu_activity_benchmark

.PHONY: all clean
all: $(BIN)

$(BIN): $(SRC) | $(BUILD_DIR)
	@if [ -z "$(CXX)" ]; then \
	    echo "No SYCL compiler found. Set GEOPM_GPU_BENCH_CXX or load the Intel oneAPI compiler environment so icpx/dpcpp is on PATH." >&2; \
	    exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
