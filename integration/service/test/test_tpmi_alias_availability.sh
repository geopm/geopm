#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    High level alias availability across the MSR / TPMI interface change:
    -------------------------------------------------------------------

    On recent Intel Xeon parts (Granite Rapids, Sierra Forest, Clearwater
    Forest) the RAPL power/energy and uncore frequency knobs moved off the
    Model Specific Register (MSR) interface and onto TPMI.  GEOPM drops the
    now-unsupported MSRs from its per-platform definitions and instead serves
    the high level aliases (CPU_POWER_LIMIT_CONTROL, CPU_ENERGY,
    CPU_UNCORE_FREQUENCY_MAX_CONTROL, ...) from the powercap and uncore sysfs
    IOGroups, which the kernel backs with TPMI.

    This read-only test asserts three properties on the system under test:

      1. Every high level alias that is present reads a finite value, i.e. it
         maps to a functioning implementation rather than a stub.
      2. GEOPM rejects unknown register names instead of silently accepting
         them, so a write to an unsupported register cannot silently succeed.
      3. When a RAPL or uncore MSR has been replaced by TPMI (absent from the
         control list) the corresponding sysfs provider still supplies the high
         level alias, so the alias never loses its provider.

    The test only reads platform state and probes rejected writes that never
    take effect; it does not modify any control.
"
    exit 0
fi

set -u

err() {
    echo "Error: $1" 1>&2
    exit 1
}

# Succeeds (exit 0) when the argument parses to a finite number.  The string is
# screened first so that "nan", "inf" and "" are rejected on every awk variant
# (mawk parses those to 0, gawk does not).
is_finite() {
    case "$1" in
        ''|*[!0-9eE.+-]*) return 1 ;;
    esac
    awk -v x="$1" 'BEGIN { v = x + 0.0; if (v < 1e308 && v > -1e308) exit 0; exit 1 }'
}

# Succeeds when NAME appears on its own line in the newline separated LIST.
in_list() {
    printf '%s\n' "$2" | grep -qx -- "$1"
}

SIGNALS=$(geopmread) || err "Failed to list signals with geopmread"
CONTROLS=$(geopmwrite) || err "Failed to list controls with geopmwrite"

# 1. High level aliases affected by the MSR -> TPMI change must, when present,
#    map to a functioning implementation that reads a finite value.
ALIASES="
CPU_ENERGY
DRAM_ENERGY
CPU_POWER_LIMIT_CONTROL
CPU_POWER_TIME_WINDOW_CONTROL
DRAM_POWER_LIMIT_CONTROL
CPU_UNCORE_FREQUENCY_STATUS
CPU_UNCORE_FREQUENCY_MIN_CONTROL
CPU_UNCORE_FREQUENCY_MAX_CONTROL
"
for ALIAS in ${ALIASES}; do
    if in_list "${ALIAS}" "${SIGNALS}"; then
        DOMAIN=$(geopmread --signal-domain "${ALIAS}") ||
            err "Failed to query domain for ${ALIAS}"
        VALUE=$(geopmread "${ALIAS}" "${DOMAIN}" 0) ||
            err "${ALIAS} is listed but geopmread failed (dead alias)"
        test -n "${VALUE}" ||
            err "${ALIAS} read returned an empty value"
        is_finite "${VALUE}" ||
            err "${ALIAS} read a non-finite value '${VALUE}' (no functioning provider)"
        echo "OK: ${ALIAS} = ${VALUE} (${DOMAIN})"
    else
        echo "SKIP: ${ALIAS} is not available on this platform"
    fi
done

# 2. An unknown register must be rejected by both read and write, otherwise a
#    write to an unsupported register could silently appear to succeed.
BOGUS=MSR::GEOPM_TEST_BOGUS_REGISTER:FIELD
if geopmread "${BOGUS}" board 0 >/dev/null 2>&1; then
    err "geopmread silently accepted the unknown register ${BOGUS}"
fi
if geopmwrite "${BOGUS}" board 0 0 >/dev/null 2>&1; then
    err "geopmwrite silently accepted the unknown register ${BOGUS}"
fi
echo "OK: unknown register ${BOGUS} rejected by geopmread and geopmwrite"

# 3. For each RAPL / uncore MSR that TPMI may replace, if it is absent from the
#    control list then a direct write to it must fail (no silent success), and
#    the high level alias must instead be served by the matching sysfs provider.
check_replaced() {
    # $1 raw MSR control, $2 alias, $3 sysfs evidence glob
    local RAW_MSR="$1" ALIAS="$2" SYSFS_GLOB="$3"
    if in_list "${RAW_MSR}" "${CONTROLS}"; then
        echo "INFO: ${RAW_MSR} is MSR-backed on this platform"
        return 0
    fi
    if geopmwrite "${RAW_MSR}" package 0 1 >/dev/null 2>&1; then
        err "${RAW_MSR} is not a listed control yet geopmwrite silently succeeded"
    fi
    echo "OK: absent MSR ${RAW_MSR} write correctly rejected"
    if ls ${SYSFS_GLOB} >/dev/null 2>&1; then
        in_list "${ALIAS}" "${CONTROLS}" ||
            err "${RAW_MSR} is TPMI-replaced and ${SYSFS_GLOB} exists, but ${ALIAS} has no provider"
        echo "OK: ${ALIAS} served via sysfs while ${RAW_MSR} is TPMI-replaced"
    fi
}

check_replaced "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT" \
               "CPU_POWER_LIMIT_CONTROL" \
               "/sys/class/powercap/intel-rapl*"
check_replaced "MSR::DRAM_POWER_LIMIT:PL1_POWER_LIMIT" \
               "DRAM_POWER_LIMIT_CONTROL" \
               "/sys/class/powercap/intel-rapl*"
check_replaced "MSR::UNCORE_RATIO_LIMIT:MAX_RATIO" \
               "CPU_UNCORE_FREQUENCY_MAX_CONTROL" \
               "/sys/devices/system/cpu/intel_uncore_frequency"
check_replaced "MSR::UNCORE_RATIO_LIMIT:MIN_RATIO" \
               "CPU_UNCORE_FREQUENCY_MIN_CONTROL" \
               "/sys/devices/system/cpu/intel_uncore_frequency"

echo "SUCCESS"
exit 0
