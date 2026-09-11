#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#  Decide whether this system is ready for a GEOPM tuning campaign.  Every
#  check is read-only unless --write-probe is given.  Exits 0 only when the
#  readiness gate is met, so a caller can branch on the exit status.

set -uo pipefail

VENV=""
WRITE_PROBE=0
QUIET=0

# Controls a tuning campaign may sweep, in the order the report lists them.
CANDIDATE_CONTROLS=(
    CPU_FREQUENCY_MAX_CONTROL
    CPU_UNCORE_FREQUENCY_MAX_CONTROL
    POWERCAP::CPU_POWER_LIMIT
    GPU_CORE_FREQUENCY_MAX_CONTROL
    GPU_POWER_LIMIT_CONTROL
    BOARD_POWER_LIMIT_CONTROL
)

# A frequency sweep pins rather than caps: geopmopt mirrors a *_MAX_* setting
# onto the matching *_MIN_* control.  cpu-freq also forces the performance
# governor so the requested cap sticks.  Granting only the sweep control lets
# the gate pass and then fails the campaign partway, so check the dependencies
# too.
declare -A REQUIRED_CONTROLS=(
    [CPU_FREQUENCY_MAX_CONTROL]=CPU_FREQUENCY_GOVERNOR_CONTROL
    [CPU_UNCORE_FREQUENCY_MAX_CONTROL]=CPU_UNCORE_FREQUENCY_MIN_CONTROL
    [GPU_CORE_FREQUENCY_MAX_CONTROL]=GPU_CORE_FREQUENCY_MIN_CONTROL
)

print_usage() {
    cat <<'USAGE'
Usage: geopm-verify-install.sh [OPTION]...

Verify that GEOPM is installed, that the Access Service is reachable, and that
this user may write at least one control that a tuning campaign would sweep.

Options:
  --venv DIR        Use the GEOPM tools from DIR/bin in preference to $PATH.
                    geopmopt ships only in development snapshots, so it usually
                    lives in a virtual environment rather than in /usr/bin.
  --write-probe     Additionally prove writability by writing one control back
                    to the value it already has.  The value is unchanged and
                    the GEOPM session restores it on exit, but this does open a
                    write session, so it is off by default.
  --quiet           Suppress the report and communicate only via exit status.
  -h, --help        Print this help message and exit.

Exit status:
  0  ready: the readiness gate is met
  1  not ready: at least one gate criterion failed
  2  usage error
USAGE
}

while (( $# )); do
    case "$1" in
        --venv) [[ $# -ge 2 ]] || { echo "--venv requires an argument" >&2; exit 2; }
                VENV="$2"; shift ;;
        --write-probe) WRITE_PROBE=1 ;;
        --quiet) QUIET=1 ;;
        -h|--help) print_usage; exit 0 ;;
        *) echo "geopm-verify-install.sh: unknown option '$1'" >&2
           print_usage >&2; exit 2 ;;
    esac
    shift
done

if [[ -n $VENV ]]; then
    if [[ ! -x "$VENV/bin/geopmread" ]]; then
        echo "geopm-verify-install.sh: no geopmread in '$VENV/bin'" >&2
        exit 2
    fi
    PATH="$VENV/bin:$PATH"
    export PATH
fi

say() { (( QUIET )) || printf '%s\n' "$*"; }

PASS_MARK="  [ok]  "
FAIL_MARK="  [FAIL]"
WARN_MARK="  [warn]"

FAILURES=()
fail() { FAILURES+=("$1"); }

say "GEOPM readiness check: $(hostname)"
say "==============================================================="

## 1. Client tools present

if ! command -v geopmread >/dev/null 2>&1; then
    say "${FAIL_MARK} geopmread not found on PATH"
    fail "GEOPM is not installed, or its virtual environment is not active.
       Install the Access Service, then create a client virtual environment.
       See references/distro-packages.md and references/client-venv.md."
    say
    say "Cannot continue without geopmread."
    exit 1
fi
say "${PASS_MARK} geopmread: $(command -v geopmread)"

geopm_version=$(geopmread --version 2>&1 | head -1)
say "${PASS_MARK} version: ${geopm_version}"

## 2. Access Service reachable

if command -v systemctl >/dev/null 2>&1; then
    geopmd_state=$(systemctl is-active geopm 2>/dev/null)
    [[ -z $geopmd_state ]] && geopmd_state=unknown
    if [[ $geopmd_state == active ]]; then
        say "${PASS_MARK} geopmd is active"
    else
        say "${FAIL_MARK} geopmd is ${geopmd_state}"
        fail "The Access Service is not running.  Ask an administrator to run
       'sudo systemctl start geopm' (and 'enable' it to survive reboot).
       Without it, unprivileged users cannot read signals or write controls."
    fi
fi

## 3. Basic signal read

if geopmread TIME board 0 >/dev/null 2>&1; then
    say "${PASS_MARK} signal read: TIME board 0"
else
    say "${FAIL_MARK} cannot read TIME board 0"
    fail "The most basic signal read failed.  Either geopmd is not running or
       the access list grants this user nothing.  See references/access-lists.md."
fi

## 4. Real telemetry

power_value=$(geopmread CPU_POWER board 0 2>/dev/null)
power_rc=$?
if (( power_rc == 0 )) && [[ -n $power_value ]]; then
    # A board reading of zero or negative Watts indicates a signal that exists
    # but is not actually wired to hardware on this platform.
    if awk -v v="$power_value" 'BEGIN{exit !(v > 0)}' 2>/dev/null; then
        say "${PASS_MARK} telemetry: CPU_POWER board 0 = ${power_value} W"
    else
        say "${FAIL_MARK} CPU_POWER board 0 returned implausible ${power_value}"
        fail "CPU_POWER reads but is not a plausible positive wattage, so energy
       objectives will not work.  The platform may lack RAPL support."
    fi
else
    say "${FAIL_MARK} cannot read CPU_POWER board 0"
    fail "Power telemetry is unavailable, so energy and efficiency objectives
       cannot be used.  Confirm CPU_POWER is in this user's access list and
       that the platform exposes RAPL."
fi

## 5. Controls granted to this user

granted_controls=""
access_ok=0
if command -v geopmaccess >/dev/null 2>&1; then
    if granted_controls=$(geopmaccess --controls 2>/dev/null); then
        access_ok=1
    else
        # A virtual environment built without --system-site-packages cannot
        # import PyGObject, which dasbus needs, so geopmaccess fails there even
        # though the access list itself is fine.  Falling back to the system
        # copy is the expected arrangement, not a misconfiguration: geopmaccess
        # only queries the daemon and need not match the client tool version.
        if [[ -x /usr/bin/geopmaccess ]] && granted_controls=$(/usr/bin/geopmaccess --controls 2>/dev/null); then
            access_ok=1
            say "${PASS_MARK} access list via /usr/bin/geopmaccess (expected inside a venv)"
        fi
    fi
fi

if (( ! access_ok )); then
    say "${FAIL_MARK} cannot query the access list"
    fail "geopmaccess could not run, so granted controls are unknown.  If you are
       using a virtual environment, rebuild it with --system-site-packages so
       that PyGObject is visible.  See references/client-venv.md."
fi

writable=()
for control in "${CANDIDATE_CONTROLS[@]}"; do
    if printf '%s\n' "$granted_controls" | grep -qx "$control"; then
        writable+=("$control")
    fi
done

if (( ${#writable[@]} )); then
    say "${PASS_MARK} writable controls: ${#writable[@]} of ${#CANDIDATE_CONTROLS[@]} candidates"
    for control in "${writable[@]}"; do
        say "           - ${control}"
    done
    # A granted MAX without its MIN passes this gate and then fails the
    # campaign, and cpu-freq also needs the governor control, so surface these
    # gaps here rather than an hour later.
    for control in "${writable[@]}"; do
        companion=${REQUIRED_CONTROLS[$control]:-}
        [[ -z $companion ]] && continue
        if ! printf '%s\n' "$granted_controls" | grep -qx "$companion"; then
            say "${WARN_MARK} ${control} is granted but ${companion} is not"
            if [[ $companion == CPU_FREQUENCY_GOVERNOR_CONTROL ]]; then
                fail "Sweeping cpu-freq also writes ${companion}=performance, so the
       campaign will fail partway without that grant.  Ask for it alongside
       ${control}."
            else
                fail "Sweeping that dimension pins the frequency, writing both the MAX and
       the MIN control, so the campaign will fail partway without
       ${companion}.  Ask for it alongside the MAX."
            fi
        fi
    done
elif (( access_ok )); then
    say "${FAIL_MARK} no sweepable control is granted to this user"
    # Name the controls, and separate a platform limitation from an access
    # problem: "not supported here" and "not granted to you" need different
    # people to fix them.
    supported_controls=$(geopmaccess --all --controls 2>/dev/null \
                         || /usr/bin/geopmaccess --all --controls 2>/dev/null)
    ungranted=(); unsupported=()
    for control in "${CANDIDATE_CONTROLS[@]}"; do
        if printf '%s\n' "$supported_controls" | grep -qx "$control"; then
            ungranted+=("$control")
        else
            unsupported+=("$control")
        fi
    done
    for control in "${ungranted[@]}"; do
        say "           - ${control}: supported here, NOT granted to you"
    done
    for control in "${unsupported[@]}"; do
        say "           - ${control}: not supported on this platform"
    done
    if (( ${#ungranted[@]} )); then
        fail "These controls exist on this system but are not in your access list:
       $(printf '%s ' "${ungranted[@]}")
       An administrator must grant at least one.  Generate the exact commands
       with scripts/geopm-gen-access.sh, or see references/access-lists.md."
    else
        fail "None of the controls a campaign would sweep exist on this platform.
       This is a hardware or build limitation, not an access-list problem, and
       no administrator can grant them.  Typical of a virtual machine, a
       container without hardware access, or WSL.  Use a bare-metal host."
    fi
fi

## 6. geopmopt available and usable

# geopmopt is absent from every tagged release to date; it ships only in
# development snapshots, so a virtual environment is the expected home for it.
if ! command -v geopmopt >/dev/null 2>&1; then
    say "${FAIL_MARK} geopmopt not found on PATH"
    fail "geopmopt is not installed.  It is not part of a tagged GEOPM release,
       so install a development snapshot into a virtual environment:
       see references/client-venv.md.  Pass --venv DIR to check that
       environment with this script."
else
    opt_out=$(geopmopt --list-controls 2>&1)
    opt_rc=$?
    if (( opt_rc != 0 )); then
        say "${FAIL_MARK} geopmopt cannot run"
        if [[ $opt_out == *'scikit-optimize is required'* ]]; then
            fail "geopmopt is installed but scikit-optimize is missing.  Do not add
       it to the system Python; build a virtual environment from the dev
       branch instead.  See references/client-venv.md."
        else
            fail "geopmopt exited ${opt_rc}.  First line of the error was:
       $(printf '%s\n' "$opt_out" | tail -1)"
        fi
    else
        say "${PASS_MARK} geopmopt: $(command -v geopmopt)"
        # A dimension is usable only when the platform resolved a native domain
        # and reported real bounds.  Bounds alone are not enough: unavailable
        # power dimensions still print hardcoded defaults next to an n/a domain.
        usable=$(printf '%s\n' "$opt_out" | awk 'NR>1 && NF>=6 && $2!="n/a" && $4!="n/a" && $5!="n/a" && $6!="n/a" {print $1}')
        usable_count=$(printf '%s' "$usable" | grep -c . || true)
        if (( usable_count > 0 )); then
            say "${PASS_MARK} sweepable dimensions: ${usable_count}"
            while IFS= read -r dim; do
                [[ -n $dim ]] && say "           - ${dim}"
            done <<< "$usable"
        else
            say "${FAIL_MARK} no sweep dimension has usable bounds"
            fail "geopmopt runs but every dimension reports n/a bounds, so there is
       nothing to search.  The platform may not expose the frequency and
       power limits GEOPM needs."
        fi
    fi
fi

## 7. Optional write probe

if (( WRITE_PROBE )); then
    if (( ${#writable[@]} == 0 )); then
        say "${WARN_MARK} skipping write probe: no candidate control is granted"
    else
        probe_control="${writable[0]}"
        # --control-domain is a geopmwrite option; geopmread spells it
        # --signal-domain and would reject the query.
        probe_domain=$(geopmwrite --control-domain "$probe_control" 2>/dev/null)
        probe_domain=${probe_domain:-board}
        current=$(geopmread "$probe_control" "$probe_domain" 0 2>/dev/null)
        if [[ -z $current ]]; then
            say "${WARN_MARK} write probe: cannot read current ${probe_control}"
        elif geopmwrite "$probe_control" "$probe_domain" 0 "$current" >/dev/null 2>&1; then
            say "${PASS_MARK} write probe: ${probe_control} rewritten to its current ${current}"
        else
            say "${FAIL_MARK} write probe: writing ${probe_control} was denied"
            fail "Writing ${probe_control} failed even though it appears in the access
       list.  Check that geopmd is running and that the list was applied."
        fi
    fi
fi

## Verdict

say
if (( ${#FAILURES[@]} == 0 )); then
    say "READY.  The readiness gate is met:"
    say "  - geopmopt lists at least one dimension with real bounds"
    say "  - geopmread returns a plausible power reading"
    say "  - at least one sweepable control is writable by this user"
    say
    say "Proceed to the geopm-optimize assistant."
    exit 0
fi

say "NOT READY.  ${#FAILURES[@]} issue(s) must be resolved:"
say
for issue in "${FAILURES[@]}"; do
    say "  * ${issue}"
    say
done
exit 1
