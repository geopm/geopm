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

# This script sets up GEOPM configuration files to cause all jobs
# to be started with a fixed power cap at 50 watts below TDP.
# It should be run on every compute node for which this configuration
# is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#
## TODO: not tested

# Calculate the desired fixed power cap
POWER_CAP=$($(srun geopmread POWER_PACKAGE_TDP board 0) - 50)

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/fixed_power_policy.json"
geopmagent -a power_balancer -p $POWER_CAP > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "POWER_PACKAGE_LIMIT_TOTAL": 230
#   }
# for a system where the TDP is 140 W per package or 280 W total.

# Set the GEOPM configuration to use this policy file and the
# energy efficient agent.
geopmadmin --magic power_balancer > $(geopmagent --config-default)

# This file should look similar to the following:
# {
#    "GEOPM_AGENT": "power_balancer",
#    "GEOPM_POLICY": "/etc/geopm/fixed_power_policy.json"
# }