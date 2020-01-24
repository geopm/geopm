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
# to be started with a fixed processor frequency 300 MHz below the
# base (sticker) frequency.  It should be run on every compute node
# for which this configuration is desired.
#
# This script modifies system files and typically requires root
# because these files are not expected to be writeable by non-admin users.
#

## TODO: not tested

# Calculate the desired fixed frequency
FREQ_FIXED=$($(srun geopmread FREQ_STICKER board 0) - 300000000)

# Create the policy file in a location accessible to the compute node
mkdir -p /etc/geopm
POLICY_FILE_PATH="/etc/geopm/fixed_freq_policy.json"
geopmagent -a energy_efficient -p NAN,NAN,NAN,$FREQ_FIXED > $POLICY_FILE_PATH
# This file should look similar to:
#   {
#     "FREQ_MIN": "NAN",
#     "FREQ_MAX": "NAN",
#     "PERF_MARGIN": "NAN",
#     "FREQ_FIXED": 1800000000
#   }
# for a system where the sticker frequency is 2.1 GHz and the target
# frequency is 1.8 GHz.

# Set the GEOPM configuration to use this policy file and the
# energy efficient agent.
geopmadmin --magic > $(geopmagent --config-default)

# This file should look similar to the following:
# {
#    "GEOPM_AGENT": "energy_efficient",
#    "GEOPM_POLICY": "/etc/geopm/fixed_freq_policy.json"
# }
