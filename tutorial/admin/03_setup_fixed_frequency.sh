#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This script sets up GEOPM configuration files to cause all jobs
# to be started with a fixed processor frequency 300 MHz below the
# base (sticker) frequency.  It should be run on every compute node
# for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

set -x

# Calculate the desired fixed frequency
FREQ_FIXED=$(($(geopmread CPU_FREQUENCY_STICKER board 0)-300000000))

# Remove any existing configuration
rm -rf /etc/geopm

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/fixed_freq_policy.json"
geopmagent -a frequency_map -p $FREQ_FIXED > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "FREQ_CPU_DEFAULT": 1800000000
#   }
# for a system where the sticker frequency is 2.1 GHz and the target
# frequency is 1.8 GHz.

# Set the GEOPM configuration to use this policy file and the
# frequency map agent.
echo "{\"GEOPM_AGENT\": \"frequency_map\", \"GEOPM_POLICY\": \"$POLICY_FILE_PATH\"}" > $(geopmadmin --config-default)

# This file should look similar to the following:
# {
#    "GEOPM_AGENT": "frequency_map",
#    "GEOPM_POLICY": "/etc/geopm/fixed_freq_policy.json"
# }

# Example sanity checks of the configuration
#
# Fixed frequency for non-GEOPM jobs enforced
#   > srun geopmread MSR::PERF_CTL:FREQ board 0
#   1800000000
#
# GEOPM jobs use frequency map agent with above policy (no restrictions on algorithm)
#   > geopmlaunch srun -N1 -n1 --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_CPU_DEFAULT": 1800000000}
