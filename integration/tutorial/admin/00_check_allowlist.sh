#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
