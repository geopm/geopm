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

# This script sets up GEOPM configuration files to enforce an average power cap
# for all jobs of 50 watts less than TDP.  Jobs that opt into using GEOPM
# will be required to use the power_balancer agent, which will allow them
# to achieve better performance if their apps have been annotated with
# GEOPM epoch markup.
# This script should be run on every compute node for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

set -x

# Calculate the desired fixed power cap
POWER_CAP=$(($(geopmread POWER_PACKAGE_TDP board 0)-50))

# Remove any existing configuration
rm -rf /etc/geopm

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/power_balancer_policy.json"
geopmagent -a power_balancer -p $POWER_CAP > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "POWER_PACKAGE_LIMIT_TOTAL": 230
#   }
# for a system where the TDP is 140 W per package or 280 W total.

# Set the GEOPM configuration to use this policy file and the
# energy efficient agent.
echo "{\"GEOPM_AGENT\": \"power_balancer\", \"GEOPM_POLICY\": \"$POLICY_FILE_PATH\"}" > $(geopmadmin --config-override)

# This file should look similar to the following:
# {
#    "GEOPM_AGENT": "power_balancer",
#    "GEOPM_POLICY": "/etc/geopm/power_balancer_policy.json"
# }

# Non-GEOPM jobs are power limited
#   > srun --reservation=plugin_power_cap geopmread MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT package 0
#   115
# GEOPM jobs use the balancer and 230 W limit policy
#   > geopmlaunch srun -N1 -n1 --reservation=plugin_power_cap --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep 'Policy\|Agent' plugin_test.report
#   Agent: power_balancer
#   Policy: {"POWER_PACKAGE_LIMIT_TOTAL": 230}

# GEOPM jobs cannot use a different agent
#   > geopmlaunch srun -N1 -n1 --reservation=plugin_power_cap --geopm-agent=monitor --geopm-report=plugin_test.report -- geopmbench ~/short.conf > geopm_stdout 2>&1 && grep 'Policy\|Agent' plugin_test.report
#   Agent: power_balancer
#   Policy: {"POWER_PACKAGE_LIMIT_TOTAL": 230}
#   > grep Warning geopm_stdout
#   Warning: <geopm> User provided environment variable "GEOPM_AGENT" with value <monitor> has been overriden with value <power_balancer>