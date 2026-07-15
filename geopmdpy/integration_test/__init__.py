#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""GEOPM Python session-agent integration tests.

This package holds live-hardware effectiveness tests for the geopmdpy
session agents (e.g. the GPU activity agent).  Unlike the hermetic unit
tests under ``geopmdpy/test`` (which mock ``pio``/``topo`` and never touch
hardware), these tests require a running GEOPM service, real hardware
(e.g. a GPU), and a workload.  They are opt-in and are NOT collected by
``make check``; run them explicitly with the ``GEOPM_RUN_GPU_INTEGRATION``
environment variable set.  See ``README.md`` in this directory.
"""
