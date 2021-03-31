#!/bin/bash
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

#  This script will examine the currently installed allowlist for msr-safe
#  and verify that the allowlist has the minimum required permissions in
#  order for GEOPM to function properly.

set -eo pipefail

bitwise_and(){
    printf '0X%016X\n' "$(( ${1} & ${2} ))"
}

RC=0

FILE=/dev/cpu/msr_allowlist
if [ ! -c ${FILE} ]; then
    FILE=/dev/cpu/msr_whitelist
fi
if [ ! -c ${FILE} ]; then
    echo "ERROR: /dev/cpu/msr_allowlist is not available.  Note: msr_whitelist was also checked. Please install msr-safe."
    RC=1
else
    ALLOWLIST=$(cat ${FILE}) # This may require root.  If it does, re-run this script as root, or create a temp file
                             # with the contents of msr_allowlist and overwrite the value for ${FILE}.
    mapfile -t AL_MSRS < <(echo "${ALLOWLIST}" | tail -n +2 | cut -d' ' -f1)
    mapfile -t AL_WRITEMASK < <(echo "${ALLOWLIST}" | tail -n +2 | cut -d' ' -f2)

    GEOPM_AL=$(geopmadmin -a)
    mapfile -t GEOPM_MSRS < <(echo "${GEOPM_AL}" | tail -n +2 | cut -d' ' -f1)
    mapfile -t GEOPM_WRITEMASK < <(echo "${GEOPM_AL}" | tail -n +2 | cut -d' ' -f4)

    for idx in "${!GEOPM_MSRS[@]}"; do
        FOUND=0
        for jdx in "${!AL_MSRS[@]}"; do
            if [ "${GEOPM_MSRS[idx]^^}" = "${AL_MSRS[jdx]^^}" ]; then
                FOUND=1
                if [ "$(bitwise_and ${GEOPM_WRITEMASK[idx]^^} ${AL_WRITEMASK[jdx]^^})" != "${GEOPM_WRITEMASK[idx]^^}" ]; then
                    echo "ERROR: MSR ${GEOPM_MSRS[idx]} has an improper writemask."
                    echo "       GEOPM requires: ${GEOPM_WRITEMASK[idx]} | Current value: ${AL_WRITEMASK[jdx]}"
                    RC=1
                    break
                fi
            fi
        done
        if [ ${FOUND} -ne 1 ]; then
            echo "ERROR: Required MSR ${GEOPM_MSRS[idx]} was not found in the allowlist."
            RC=1
        fi
    done
fi

exit ${RC}
