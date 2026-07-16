#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""GEOPM Python session-agent integration tests.

This package holds live-hardware effectiveness tests for the geopmdpy
session agents (e.g. the GPU activity agent).  Unlike the hermetic unit
tests under ``geopmdpy/test`` (which mock ``pio``/``topo`` and never touch
hardware), these tests require a running GEOPM service, real hardware
(e.g. a GPU), and a workload.  They are NOT collected by ``make check``; run
them explicitly from this independent ``integration_test`` directory.  See
``README.md`` in this directory.
"""
