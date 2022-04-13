#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This script sets up GEOPM configuration files to enforce a maximum
# CPU frequency of 2.6 GHz for jobs that do not use GEOPM, and force jobs
# that use GEOPM to use the energy efficient agent with a policy ranging
# between 1.2 GHz and 3.7 GHz.  This script should be run on every compute
# node for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

set -x

# Range of frequencies for agent to use
FREQ_MIN=1200000000
FREQ_MAX=3700000000
# Maximum frequency for non-GEOPM jobs
FREQ_FIXED=2600000000

# Remove any existing configuration
rm -rf /etc/geopm

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/energy_efficient_policy.json"
geopmagent -a energy_efficient -p $FREQ_MIN,$FREQ_MAX,NAN,$FREQ_FIXED > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "FREQ_MIN": 1200000000,
#     "FREQ_MAX": 3700000000,
#     "PERF_MARGIN": "NAN",
#     "FREQ_FIXED": 2600000000
#   }


# Set the GEOPM configuration to use this policy file and the
# energy efficient agent.
echo "{\"GEOPM_AGENT\": \"energy_efficient\", \"GEOPM_POLICY\": \"$POLICY_FILE_PATH\"}" > $(geopmadmin --config-override)

# This file should look similar to the following:
# {
#    "GEOPM_AGENT": "energy_efficient",
#    "GEOPM_POLICY": "/etc/geopm/energy_efficient_policy.json"
# }

# Example sanity checks of the configuration
#
# Fixed frequency for non-GEOPM jobs enforced
#   > srun geopmread MSR::PERF_CTL:FREQ board 0
#   2600000000
#
# GEOPM jobs use energy efficient agent with above policy (up to 3.7 GHz)
#   > geopmlaunch srun -N1 -n1 --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_MIN": 1200000000, "FREQ_MAX": 3700000000, "PERF_MARGIN": "NAN", "FREQ_FIXED": 2600000000}
#
# GEOPM jobs cannot use a different agent
#   > geopmlaunch srun -N1 -n1 --geopm-agent=monitor --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_MIN": 1200000000, "FREQ_MAX": 3700000000, "PERF_MARGIN": "NAN", "FREQ_FIXED": 2600000000}
#   > grep Warning geopm_stdout
#   Warning: <geopm> User provided environment variable "GEOPM_AGENT" with value <monitor> has been overriden with value <energy_efficient>
