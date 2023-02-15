#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This script sets up GEOPM configuration files to cause jobs that
# use GEOPM to use the frequency map agent by default.  The default
# policy will cause the agent to set a fixed frequency.  It should be run
# on every compute node for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

set -x

# Calculate the desired fixed frequency
FREQ_FIXED=1700000000

# Remove any existing configuration
rm -rf /etc/geopm

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/fixed_freq_policy.json"
geopmagent -a frequency_map -p $FREQ_FIXED > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "FREQ_DEFAULT": 1700000000
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
# Fixed frequency for non-GEOPM jobs stays at max
#   > srun geopmread MSR::PERF_CTL:FREQ board 0
#   3700000000
#
# GEOPM jobs use frequency map agent with above policy; check report
#   > geopmlaunch srun -N1 -n1 --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_DEFAULT": 1700000000}
