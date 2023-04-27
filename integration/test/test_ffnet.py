#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This end-to-end integration test verifies that the ffnet agent functions
as expected, properly identifying region classes and setting frequencies
accordingly.
"""
@util.skip_unless_config_enable('beta')
@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")

#TODO: Add cpu frequency sweep of aib and geopmbench

#TODO: If on gpu-enabled system, add gpu frequency sweep of parres dgemm / stream

#TODO: Add h5 generation
#TODO:     and check for expected columns

#TODO: Add NN generation for CPU (and GPU if on gpu-enabled system)
#          and check format / output columns

#TODO: Add region frequency map generation for CPU (and GPU if on gpu-enabled system)
#TODO:     and check that frequency for stream at phi=1 is less than frequency for dgemm at phi=0 (in json)
#TODO:     and check that frequency for dgemm at phi=0 >= frequency for dgemm at phi=1 (in json)

#TODO: Run ffnet agent with dummy neural net and check for expected responses
#      Dummy neural net can use region hash to ID region class definitively.

#TODO: Run geopmbench on pkg0, ffnet phi=0 and phi=1
#TODO:     and check that perf is within 2% for phi=0
#TODO:     and check perf @phi0 > perf @phi1
#TODO:     and check total energy@phi0 > total energy@phi1

#TODO: If on system with gpu, run parres dgemm ffnet with phi=0 and phi=1
#TODO:     and check that perf is within 2% for phi=0
#TODO:     and check perf0 > perf1
#TODO:     and check gpu energy0 > gpu energy1
#TODO:     and check that other gpus are not steered

#TODO [stretch]: Try running different apps on different sockets/gpus and tests that they are steered independently
