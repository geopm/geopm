#!/bin/bash
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# This script sets up GEOPM configuration files to enforce a maximum
# CPU frequency of 1.5 GHz for jobs that do not use GEOPM, and force jobs
# that use GEOPM to use the energy efficient agent with a policy ranging
# between 1.2 GHz and 1.7 GHz.  This script should be run on every compute
# node for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

set -x

# Range of frequencies for agent to use
FREQ_MIN=1200000000
FREQ_MAX=1700000000
# Maximum frequency for non-GEOPM jobs
FREQ_FIXED=1500000000

# Remove any existing configuration
rm -rf /etc/geopm

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/energy_efficient_policy.json"
geopmagent -a energy_efficient -p $FREQ_MIN,$FREQ_MAX,NAN,$FREQ_FIXED > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "FREQ_MIN": 1200000000,
#     "FREQ_MAX": 1700000000,
#     "PERF_MARGIN": "NAN",
#     "FREQ_FIXED": 1500000000
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
#   1500000000
#
# GEOPM jobs use energy efficient agent with above policy (up to 1.7 GHz)
#   > geopmlaunch srun -N1 -n1 --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_MIN": 1200000000, "FREQ_MAX": 1700000000, "PERF_MARGIN": "NAN", "FREQ_FIXED": 1500000000}
#
# GEOPM jobs cannot use a different agent
#   > geopmlaunch srun -N1 -n1 --geopm-agent=monitor --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep Policy plugin_test.report
#   Policy: {"FREQ_MIN": 1200000000, "FREQ_MAX": 1700000000, "PERF_MARGIN": "NAN", "FREQ_FIXED": 1500000000}
#   > grep Warning geopm_stdout
#   Warning: <geopm> User provided environment variable "GEOPM_AGENT" with value <monitor> has been overriden with value <energy_efficient>