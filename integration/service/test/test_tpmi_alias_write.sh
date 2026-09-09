#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    High level control aliases write through to the hardware:
    --------------------------------------------------------

    On recent Intel Xeon parts the RAPL power and uncore frequency controls
    moved from the MSR interface to TPMI, and GEOPM now serves the high level
    aliases (CPU_POWER_LIMIT_CONTROL, CPU_UNCORE_FREQUENCY_MAX_CONTROL, ...)
    from the powercap and uncore sysfs IOGroups.  A silent failure would be a
    write that returns success while the underlying value never changes.

    For each writeable alias affected by the change this test:

      1. reads the current value,
      2. opens a write session that sets a small, safe delta and holds it,
      3. reads the value back and asserts it actually changed (no silent
         failure), then
      4. lets the session end and asserts the value is restored.

    The write uses the plain alias name (no SERVICE:: prefix).  When the test
    is run by a non-root user the geopmwrite client connects to the GEOPM
    service, which records the previous value on the first write of a session
    and restores it when that session ends, so save/restore is automatic.
    Run this test as a non-root user for the restore assertion to hold.

    Aliases that are absent, or present but not writeable on this platform
    (for example a locked DRAM power limit), are skipped rather than failed.

    The test briefly perturbs the CPU/DRAM power limit and uncore frequency of
    the system under test; every change is reverted by save/restore.  It
    requires that control access has been granted:

        geopmaccess -a -c | sudo geopmaccess -w -c
"
    exit 0
fi

set -u

SCRIPT_DIR=$(dirname "$(realpath "$0")")

err() {
    echo "Error: $1" 1>&2
    exit 1
}

# Succeeds when the argument parses to a finite number on every awk variant.
is_finite() {
    case "$1" in
        ''|*[!0-9eE.+-]*) return 1 ;;
    esac
    awk -v x="$1" 'BEGIN { v = x + 0.0; if (v < 1e308 && v > -1e308) exit 0; exit 1 }'
}

# Succeeds when |$1 - $2| > $3.
differs_by() {
    awk -v a="$1" -v b="$2" -v t="$3" 'BEGIN { d = a - b; if (d < 0) d = -d; if (d > t) exit 0; exit 1 }'
}

# Succeeds when |$1 - $2| <= $3.
within() {
    awk -v a="$1" -v b="$2" -v t="$3" 'BEGIN { d = a - b; if (d < 0) d = -d; if (d <= t) exit 0; exit 1 }'
}

# Evaluate an awk expression of the start value 's'.
eval_expr() {
    awk -v s="$1" "BEGIN { print ($2) }"
}

CONTROLS=$(geopmwrite) || err "Failed to list controls with geopmwrite"

in_list() {
    printf '%s\n' "$2" | grep -qx -- "$1"
}

# round_trip ALIAS TARGET_EXPR CHANGED_MIN_EXPR RESTORE_TOL_EXPR
# The *_EXPR arguments are awk expressions in terms of the start value 's'.
round_trip() {
    local ALIAS="$1" TARGET_EXPR="$2" CHANGED_EXPR="$3" RESTORE_EXPR="$4"

    if ! in_list "${ALIAS}" "${CONTROLS}"; then
        echo "SKIP: ${ALIAS} is not available on this platform"
        return 0
    fi

    local DOMAIN
    DOMAIN=$(geopmwrite --control-domain "${ALIAS}") ||
        err "Failed to query control domain for ${ALIAS}"

    local START
    START=$(geopmread "${ALIAS}" "${DOMAIN}" 0) ||
        err "Failed to read start value of ${ALIAS}"

    if ! is_finite "${START}"; then
        echo "SKIP: ${ALIAS} start value '${START}' is not finite on this platform"
        return 0
    fi

    local TARGET CHANGED_MIN RESTORE_TOL
    TARGET=$(eval_expr "${START}" "${TARGET_EXPR}")
    CHANGED_MIN=$(eval_expr "${START}" "${CHANGED_EXPR}")
    RESTORE_TOL=$(eval_expr "${START}" "${RESTORE_EXPR}")

    # Hold the new value with a write session and read it back.  For a non-root
    # user this session goes through the service, which saves and restores the
    # value automatically when the session ends.  If the write is rejected (for
    # example a locked DRAM limit) do_write exits non-zero and the alias is
    # skipped rather than failed.
    setsid "${SCRIPT_DIR}/do_write.sh" "${ALIAS}" "${DOMAIN}" 0 "${TARGET}" &
    local SESSION_ID=$!
    sleep 4

    local DURING
    DURING=$(geopmread "${ALIAS}" "${DOMAIN}" 0)

    if ! wait "${SESSION_ID}"; then
        echo "SKIP: ${ALIAS} is present but the write was rejected on this platform"
        return 0
    fi
    sleep 1

    local END
    END=$(geopmread "${ALIAS}" "${DOMAIN}" 0)

    differs_by "${DURING}" "${START}" "${CHANGED_MIN}" ||
        err "${ALIAS} write had no effect (silent failure): start=${START} during=${DURING} target=${TARGET}"
    within "${END}" "${START}" "${RESTORE_TOL}" ||
        err "${ALIAS} was not restored after the session: start=${START} end=${END}"

    echo "OK: ${ALIAS} round-trip start=${START} during=${DURING} restored=${END} (${DOMAIN})"
}

# Power limits are in watts; back off by a few watts, well within range.
round_trip "CPU_POWER_LIMIT_CONTROL"  "s - 10" "1"       "1"
round_trip "DRAM_POWER_LIMIT_CONTROL" "s - 5"  "0.5"     "1"
# Time window is in seconds; halving it lands on a different representable value.
round_trip "CPU_POWER_TIME_WINDOW_CONTROL" "s * 0.5" "s * 0.1" "s * 0.05"
# Uncore frequency is in hertz on a 100 MHz grid; move one bin toward the middle.
round_trip "CPU_UNCORE_FREQUENCY_MAX_CONTROL" "s - 1e8" "5e7" "5e7"
round_trip "CPU_UNCORE_FREQUENCY_MIN_CONTROL" "s + 1e8" "5e7" "5e7"

echo "SUCCESS"
exit 0
