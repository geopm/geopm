#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#  Read-only probe of a system under test.  Emits KEY=VALUE lines for machine
#  consumption followed by a human-readable summary.  Makes no changes and
#  always exits 0 so that a caller can parse the findings even when GEOPM is
#  entirely absent.

set -uo pipefail

PROBE_NET=0
SUMMARY_ONLY=0
KV_ONLY=0

print_usage() {
    cat <<'USAGE'
Usage: geopm-probe-system.sh [OPTION]...

Report the facts an installer needs to choose a GEOPM install path.  Performs
no modifications and requires no privileges.

Options:
  --net             Also test reachability of PyPI and GitHub.  Off by default
                    because a proxy-less environment makes these calls block
                    until they time out.
  --kv              Print only the KEY=VALUE block.
  --summary         Print only the human-readable summary.
  -h, --help        Print this help message and exit.

Exit status is 0 whenever the probe itself ran, including when GEOPM is not
installed.  Callers should read the KEY=VALUE output to decide what to do.
USAGE
}

while (( $# )); do
    case "$1" in
        --net) PROBE_NET=1 ;;
        --kv) KV_ONLY=1 ;;
        --summary) SUMMARY_ONLY=1 ;;
        -h|--help) print_usage; exit 0 ;;
        *) echo "geopm-probe-system.sh: unknown option '$1'" >&2
           print_usage >&2; exit 2 ;;
    esac
    shift
done

declare -a KEYS=()
declare -A FACTS=()

record() {
    KEYS+=("$1")
    FACTS["$1"]="$2"
}

# Collapse whitespace so that every value stays on a single KEY=VALUE line.
clean() {
    tr '\n' ' ' | sed 's/  */ /g; s/^ //; s/ $//'
}

have() {
    command -v "$1" >/dev/null 2>&1
}

# Capture stdout, falling back to $1 only when nothing was produced.  Commands
# such as `systemctl is-enabled` print a meaningful word and still exit
# non-zero, so a plain `|| echo` would append a second line to the value.
run_or() {
    local fallback=$1; shift
    local out
    out=$("$@" 2>/dev/null | clean)
    if [[ -z $out ]]; then
        printf '%s' "$fallback"
    else
        printf '%s' "$out"
    fi
}

## Operating system and hardware

if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    record OS_ID "${ID:-unknown}"
    record OS_VERSION "${VERSION_ID:-unknown}"
    record OS_NAME "${PRETTY_NAME:-unknown}"
else
    record OS_ID unknown
    record OS_VERSION unknown
    record OS_NAME unknown
fi

record KERNEL "$(uname -r)"
record ARCH "$(uname -m)"

if have lscpu; then
    record CPU_MODEL "$(lscpu | sed -n 's/^Model name:[[:space:]]*//p' | head -1 | clean)"
    record CPU_VENDOR "$(lscpu | sed -n 's/^Vendor ID:[[:space:]]*//p' | head -1 | clean)"
    record CPU_COUNT "$(lscpu | sed -n 's/^CPU(s):[[:space:]]*//p' | head -1 | clean)"
else
    record CPU_MODEL unknown
    record CPU_VENDOR unknown
    record CPU_COUNT "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo unknown)"
fi

## Privilege and environment

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    record PRIVILEGE root
elif sudo -n true 2>/dev/null; then
    record PRIVILEGE sudo_nopasswd
elif have sudo; then
    record PRIVILEGE sudo_password
else
    record PRIVILEGE none
fi

if [[ -f /.dockerenv ]] || grep -qE '(docker|lxc|kubepods|containerd)' /proc/1/cgroup 2>/dev/null; then
    record CONTAINER yes
else
    record CONTAINER no
fi

if have systemctl && [[ -d /run/systemd/system ]]; then
    record SYSTEMD yes
else
    record SYSTEMD no
fi

## Hardware access mechanisms

if [[ -e /dev/cpu/0/msr ]]; then
    if [[ -r /dev/cpu/0/msr ]]; then
        record MSR_DEVICE readable
    else
        record MSR_DEVICE present_not_readable
    fi
else
    record MSR_DEVICE absent
fi

if have nvidia-smi; then
    record GPU_NVIDIA yes
else
    record GPU_NVIDIA no
fi

if compgen -G '/sys/class/drm/card*' >/dev/null 2>&1; then
    record GPU_DRM yes
else
    record GPU_DRM no
fi

if compgen -G '/dev/dri/render*' >/dev/null 2>&1; then
    record GPU_RENDER_NODE yes
else
    record GPU_RENDER_NODE no
fi

## Python

if have python3; then
    record PYTHON3 "$(python3 --version 2>&1 | clean)"
    if python3 -c 'import venv' >/dev/null 2>&1; then
        record PYTHON_VENV_MODULE yes
    else
        record PYTHON_VENV_MODULE no
    fi
else
    record PYTHON3 absent
    record PYTHON_VENV_MODULE no
fi

if [[ -n "${VIRTUAL_ENV:-}" ]]; then
    record ACTIVE_VIRTUALENV "$VIRTUAL_ENV"
else
    record ACTIVE_VIRTUALENV none
fi

## GEOPM client tools

for tool in geopmread geopmwrite geopmsession geopmaccess geopmopt geopmgrid geopmd; do
    key="TOOL_$(echo "$tool" | tr '[:lower:]' '[:upper:]')"
    record "$key" "$(command -v "$tool" 2>/dev/null || echo absent)"
done

if have geopmread; then
    record GEOPM_VERSION "$(run_or absent geopmread --version)"
else
    record GEOPM_VERSION absent
fi

# geopmopt only exists in development snapshots; a tagged release does not
# ship it, so its absence is expected rather than a fault.
if have geopmopt; then
    # Capture first: piping into grep under `set -o pipefail` would report the
    # non-zero exit of geopmopt rather than whether the pattern matched.
    geopmopt_out=$(geopmopt --list-controls 2>&1)
    geopmopt_rc=$?
    if (( geopmopt_rc == 0 )); then
        record GEOPMOPT_STATUS working
    elif [[ $geopmopt_out == *'scikit-optimize is required'* ]]; then
        record GEOPMOPT_STATUS missing_optimize_extra
    else
        record GEOPMOPT_STATUS broken
    fi
    unset geopmopt_out geopmopt_rc
else
    record GEOPMOPT_STATUS absent
fi

if have python3 && python3 -c 'import geopmdpy' >/dev/null 2>&1; then
    record GEOPMDPY_PATH "$(python3 -c 'import geopmdpy,os;print(os.path.dirname(geopmdpy.__file__))' 2>/dev/null | clean)"
else
    record GEOPMDPY_PATH absent
fi

if [[ -e /usr/include/geopm_pio.h ]]; then
    record LIBGEOPMD_HEADERS present
else
    record LIBGEOPMD_HEADERS absent
fi

if have ldconfig && ldconfig -p 2>/dev/null | grep -q 'libgeopmd\.so'; then
    record LIBGEOPMD_RUNTIME present
else
    record LIBGEOPMD_RUNTIME absent
fi

## Access Service state

if have systemctl; then
    record GEOPMD_ACTIVE "$(run_or unknown systemctl is-active geopm)"
    record GEOPMD_ENABLED "$(run_or unknown systemctl is-enabled geopm)"
else
    record GEOPMD_ACTIVE unknown
    record GEOPMD_ENABLED unknown
fi

if have geopmread && geopmread TIME board 0 >/dev/null 2>&1; then
    record SIGNAL_READ ok
else
    record SIGNAL_READ failed
fi

if have geopmaccess; then
    # grep -c exits 1 on a zero count, so count without relying on exit status.
    record ACCESS_SIGNALS "$(geopmaccess 2>/dev/null | grep -c . || true)"
    record ACCESS_CONTROLS "$(geopmaccess --controls 2>/dev/null | grep -c . || true)"
else
    record ACCESS_SIGNALS unknown
    record ACCESS_CONTROLS unknown
fi

## Network

# Proxy variables are frequently set in ~/.bashrc, which bash sources only for
# interactive shells.  A non-interactive `ssh host command` therefore inherits
# no proxy and every network call blocks until it times out.  Detect that case
# explicitly so the caller does not misread it as a broken network.
if [[ -n "${http_proxy:-}${https_proxy:-}" ]]; then
    record PROXY_ENV set
elif grep -qsiE '^[[:space:]]*(export[[:space:]]+)?https?_proxy=' "$HOME/.bashrc" 2>/dev/null; then
    record PROXY_ENV defined_in_bashrc_but_not_exported
else
    record PROXY_ENV unset
fi

if (( PROBE_NET )); then
    if have curl; then
        for target in "PYPI|https://pypi.org/simple/" "GITHUB|https://github.com"; do
            name="${target%%|*}"
            url="${target#*|}"
            if curl -sS --max-time 20 -o /dev/null "$url" 2>/dev/null; then
                record "NET_${name}" reachable
            else
                record "NET_${name}" unreachable
            fi
        done
    else
        record NET_PYPI unknown
        record NET_GITHUB unknown
    fi
else
    record NET_PYPI not_probed
    record NET_GITHUB not_probed
fi

## Output

if (( ! SUMMARY_ONLY )); then
    for key in "${KEYS[@]}"; do
        printf '%s=%s\n' "$key" "${FACTS[$key]}"
    done
fi

if (( KV_ONLY )); then
    exit 0
fi

fact() { printf '%s' "${FACTS[$1]:-unknown}"; }

echo
echo "GEOPM system probe: $(hostname)"
echo "==============================================================="
printf '  System      : %s (%s), kernel %s\n' "$(fact OS_NAME)" "$(fact ARCH)" "$(fact KERNEL)"
printf '  Processor   : %s, %s CPUs\n' "$(fact CPU_MODEL)" "$(fact CPU_COUNT)"
printf '  Privilege   : %s\n' "$(fact PRIVILEGE)"
printf '  Container   : %s,  systemd: %s\n' "$(fact CONTAINER)" "$(fact SYSTEMD)"
printf '  GPU         : nvidia=%s drm=%s render_node=%s\n' \
    "$(fact GPU_NVIDIA)" "$(fact GPU_DRM)" "$(fact GPU_RENDER_NODE)"
printf '  Python      : %s (venv module: %s)\n' "$(fact PYTHON3)" "$(fact PYTHON_VENV_MODULE)"
echo
printf '  GEOPM       : %s\n' "$(fact GEOPM_VERSION)"
printf '  geopmd      : active=%s enabled=%s\n' "$(fact GEOPMD_ACTIVE)" "$(fact GEOPMD_ENABLED)"
printf '  Signal read : %s\n' "$(fact SIGNAL_READ)"
printf '  Granted     : %s signals, %s controls\n' "$(fact ACCESS_SIGNALS)" "$(fact ACCESS_CONTROLS)"
printf '  geopmopt    : %s\n' "$(fact GEOPMOPT_STATUS)"
echo
echo "  Findings:"

if [[ "$(fact SIGNAL_READ)" != ok ]]; then
    if [[ "$(fact GEOPM_VERSION)" == absent ]]; then
        echo "  - GEOPM is not installed.  Install the Access Service first."
    elif [[ "$(fact GEOPMD_ACTIVE)" != active ]]; then
        echo "  - geopmd is not active; start it before reading signals."
    else
        echo "  - Signal reads fail even though geopmd is active.  Check the"
        echo "    access lists with geopmaccess."
    fi
fi

case "$(fact GEOPMOPT_STATUS)" in
    absent)
        echo "  - geopmopt is not installed.  It is not part of a tagged GEOPM"
        echo "    release, so a virtual environment built from the dev branch"
        echo "    is required.  See references/client-venv.md."
        ;;
    missing_optimize_extra)
        echo "  - geopmopt is present but scikit-optimize is missing, so it"
        echo "    cannot run.  Do not add the dependency to the system Python;"
        echo "    create a dev virtual environment instead."
        ;;
    broken)
        echo "  - geopmopt is present but fails to run.  Capture the error with"
        echo "    'geopmopt --list-controls' and see references/troubleshooting.md."
        ;;
esac

if [[ "$(fact ACCESS_CONTROLS)" == 0 ]]; then
    echo "  - No controls are granted to this user.  An administrator must"
    echo "    extend the access list before any tuning is possible."
fi

if [[ "$(fact PRIVILEGE)" == none || "$(fact PRIVILEGE)" == sudo_password ]]; then
    echo "  - Installing system packages needs privileges this session may not"
    echo "    have non-interactively.  Prefer the client virtual environment."
fi

if [[ "$(fact PROXY_ENV)" == defined_in_bashrc_but_not_exported ]]; then
    echo "  - A proxy is configured in ~/.bashrc, which bash reads only for"
    echo "    interactive shells.  Network access will hang in this"
    echo "    non-interactive session unless the proxy is exported explicitly."
fi

if [[ "$(fact CONTAINER)" == yes ]]; then
    echo "  - Running inside a container.  Reaching a host geopmd needs the"
    echo "    gRPC proxy described in references/container.md."
fi

echo
exit 0
