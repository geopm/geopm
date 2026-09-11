#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#  Report which geopmopt sweep dimensions are actually usable on this system
#  and recommend a starting set.  Read-only: queries the platform and writes
#  nothing.

set -uo pipefail

VENV=""
WORKLOAD_KIND="unknown"

print_usage() {
    cat <<'USAGE'
Usage: geopm-probe-controls.sh [OPTION]...

Print the geopmopt sweep dimensions this platform supports, explain why the
others are unavailable, and suggest a starting --sweep set.

Options:
  --venv DIR        Use the GEOPM tools from DIR/bin.  geopmopt ships only in
                    development snapshots, so it usually lives in a virtual
                    environment rather than on the default PATH.
  --workload KIND   Tailor the suggestion.  One of: cpu-bound, memory-bound,
                    power-limited, gpu-bound, unknown (default).
  -h, --help        Print this help message and exit.

Exit status:
  0  at least one dimension is usable
  1  nothing is sweepable on this platform
  2  usage error
USAGE
}

while (( $# )); do
    case "$1" in
        --venv) [[ $# -ge 2 ]] || { echo "--venv requires an argument" >&2; exit 2; }
                VENV="$2"; shift ;;
        --workload) [[ $# -ge 2 ]] || { echo "--workload requires an argument" >&2; exit 2; }
                    WORKLOAD_KIND="$2"; shift ;;
        -h|--help) print_usage; exit 0 ;;
        *) echo "geopm-probe-controls.sh: unknown option '$1'" >&2
           print_usage >&2; exit 2 ;;
    esac
    shift
done

if [[ -n $VENV ]]; then
    [[ -x "$VENV/bin/geopmopt" ]] || {
        echo "geopm-probe-controls.sh: no geopmopt in '$VENV/bin'" >&2
        exit 2
    }
    PATH="$VENV/bin:$PATH"
    export PATH
fi

if ! command -v geopmopt >/dev/null 2>&1; then
    cat >&2 <<'MSG'
geopm-probe-controls.sh: geopmopt not found on PATH.

geopmopt is not part of any tagged GEOPM release, so it normally lives in a
virtual environment built from the dev branch.  Pass --venv DIR, or see the
geopm-install skill (references/client-venv.md).
MSG
    exit 2
fi

controls_out=$(geopmopt --list-controls 2>&1)
controls_rc=$?
if (( controls_rc != 0 )); then
    echo "geopm-probe-controls.sh: geopmopt could not list controls." >&2
    printf '%s\n' "$controls_out" | tail -3 >&2
    exit 2
fi

echo "geopmopt sweep dimensions on $(hostname)"
echo "==============================================================="
printf '%s\n' "$controls_out"
echo

# A dimension is usable only when its native domain resolved.  Unavailable
# power dimensions still print hardcoded default bounds next to an n/a domain,
# so bounds alone would give a false positive.
usable=$(printf '%s\n' "$controls_out" \
    | awk 'NR>1 && NF>=6 && $2!="n/a" && $4!="n/a" && $5!="n/a" && $6!="n/a" {print $1}')
unusable=$(printf '%s\n' "$controls_out" \
    | awk 'NR>1 && NF>=6 && ($2=="n/a" || $4=="n/a" || $5=="n/a" || $6=="n/a") {print $1}')

usable_count=$(printf '%s' "$usable" | grep -c . || true)

echo "Usable dimensions: ${usable_count}"
while IFS= read -r dim; do
    [[ -n $dim ]] && echo "  + ${dim}"
done <<< "$usable"

if [[ -n $unusable ]]; then
    echo
    echo "Unavailable, with the likely reason:"
    while IFS= read -r dim; do
        [[ -z $dim ]] && continue
        case "$dim" in
            gpu-freq|gpu-power)
                reason="no GPU detected by GEOPM, or built without LevelZero/NVML" ;;
            board-power)
                reason="platform exposes no board-level power limit" ;;
            uncore-freq)
                reason="uncore control unavailable, or its bounds signals are not granted" ;;
            cpu-power)
                reason="RAPL package power limit unavailable, or bounds signals not granted" ;;
            cpu-freq)
                reason="frequency control unavailable, or CPU_FREQUENCY_*_AVAIL not granted" ;;
            prefetch)
                reason="prefetcher MSRs unavailable or not granted" ;;
            *)
                reason="control not implemented on this platform" ;;
        esac
        printf '  - %-14s %s\n' "$dim" "$reason"
    done <<< "$unusable"
fi

if (( usable_count == 0 )); then
    cat <<'MSG'

Nothing can be swept on this platform.  Every dimension reports an unresolved
domain, so geopmopt has no search space.  This is normal in a virtual machine,
a container without hardware access, or WSL, none of which expose RAPL or the
frequency controls.  Use a bare-metal host.
MSG
    exit 1
fi

# Distinguish an access-list problem from a hardware limitation, since the two
# look identical from --list-controls alone.
if [[ -n $unusable ]] && command -v geopmaccess >/dev/null 2>&1; then
    supported=$(geopmaccess --all --controls 2>/dev/null \
                || /usr/bin/geopmaccess --all --controls 2>/dev/null)
    granted=$(geopmaccess --controls 2>/dev/null \
              || /usr/bin/geopmaccess --controls 2>/dev/null)
    withheld=""
    for pair in "cpu-freq:CPU_FREQUENCY_MAX_CONTROL" \
                "uncore-freq:CPU_UNCORE_FREQUENCY_MAX_CONTROL" \
                "cpu-power:POWERCAP::CPU_POWER_LIMIT" \
                "gpu-freq:GPU_CORE_FREQUENCY_MAX_CONTROL" \
                "gpu-power:GPU_POWER_LIMIT_CONTROL" \
                "board-power:BOARD_POWER_LIMIT_CONTROL"; do
        dim="${pair%%:*}"; ctl="${pair#*:}"
        printf '%s\n' "$unusable" | grep -qx "$dim" || continue
        if printf '%s\n' "$supported" | grep -qx "$ctl" \
           && ! printf '%s\n' "$granted" | grep -qx "$ctl"; then
            withheld+="  - ${dim} (${ctl})"$'\n'
        fi
    done
    if [[ -n $withheld ]]; then
        echo
        echo "These are supported by the service but NOT granted to you."
        echo "This is an access-list problem, not a hardware limitation:"
        printf '%s' "$withheld"
        echo "  Ask an administrator; see the geopm-install skill."
    fi
fi

echo
echo "Suggested starting point:"

has() { printf '%s\n' "$usable" | grep -qx "$1"; }

suggest=""
case "$WORKLOAD_KIND" in
    cpu-bound)
        has cpu-freq    && suggest+=" --sweep cpu-freq@board"
        has uncore-freq && suggest+=" --sweep uncore-freq@board" ;;
    memory-bound)
        has uncore-freq && suggest+=" --sweep uncore-freq@board"
        has cpu-freq    && suggest+=" --sweep cpu-freq@board" ;;
    power-limited)
        has cpu-power   && suggest+=" --sweep cpu-power@board"
        has cpu-freq    && suggest+=" --sweep cpu-freq@board" ;;
    gpu-bound)
        has gpu-freq    && suggest+=" --sweep gpu-freq@board"
        has gpu-power   && suggest+=" --sweep gpu-power@board" ;;
    *)
        has cpu-freq    && suggest+=" --sweep cpu-freq@board"
        has uncore-freq && suggest+=" --sweep uncore-freq@board" ;;
esac

if [[ -z $suggest ]]; then
    # Fall back to whatever is usable when the workload kind matched nothing.
    while IFS= read -r dim; do
        [[ -n $dim ]] && suggest+=" --sweep ${dim}@board"
    done <<< "$usable"
fi

dims=$(printf '%s' "$suggest" | grep -o -- '--sweep' | grep -c . || true)
echo "  geopmopt${suggest} \\"
echo "           --trials $(( dims <= 1 ? 20 : dims == 2 ? 40 : 60 )) --verbosity 2 \\"
echo "           -- ./your-workload.sh"
echo
echo "  ${dims} dimension(s).  Budget roughly 20 trials for one dimension,"
echo "  40 for two, 60 or more for three.  Estimate total wall time as"
echo "  trials x single-run time BEFORE starting."

if printf '%s\n' "$usable" | grep -qx prefetch; then
    echo
    echo "  prefetch is available.  Worth adding for memory-bound workloads,"
    echo "  where disabling hardware prefetchers can free bandwidth.  It rarely"
    echo "  helps compute-bound codes and costs trials, so omit it otherwise."
fi

exit 0
