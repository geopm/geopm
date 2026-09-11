#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#  Decide whether a workload can be optimized at all, before spending hours on
#  a campaign.  Measures run-to-run variability at a fixed setting, then the
#  change produced by one step of a control, and compares them.  A knob whose
#  single step moves the figure of merit by less than the noise cannot be
#  optimized: the search will fit noise and report a winner that does not
#  reproduce.
#
#  Settings are applied inside a GEOPM session and reverted on exit.

set -uo pipefail

VENV=""
REGEX=""
DIMENSION="cpu-freq"
DOMAIN="board"
REPEATS=3
SKIP_RANGE=0
EMIT_WRAPPER=""

print_usage() {
    cat <<'USAGE'
Usage: geopm-sensitivity.sh [OPTION]... -- COMMAND [ARG]...

Measure whether COMMAND is sensitive enough to a control to be worth
optimizing.  Reports the run-to-run noise floor, the effect of one step of the
control, and the effect of the control's full range, then recommends how to
proceed.

Options:
  --venv DIR        Use GEOPM tools from DIR/bin.
  --regex PATTERN   Figure of merit to scrape from stdout, one capturing group.
                    Without it, wall-clock runtime is used (lower is better).
  --dimension DIM   Control to test (default: cpu-freq).  See --list-controls.
  --domain DOMAIN   Domain to apply it at (default: board).
  --repeats N       Runs per setting (default: 3).  More runs give a tighter
                    estimate; 5 is better if you can afford the time.
  --skip-range      Skip the full-range measurement.  Halves the time, but
                    cannot then tell "knob does nothing" from "step too small".
  --emit-wrapper F  Write a repeat-and-average wrapper script to F, sized from
                    the measurement, for use as the geopmopt launch command.
  -h, --help        Print this help message and exit.

Exit status:
  0  the workload is optimizable with this control as configured
  1  it is not, or not without a change; the report says which
  2  usage error
USAGE
}

while (( $# )); do
    case "$1" in
        --venv) VENV="${2:?--venv requires an argument}"; shift ;;
        --regex) REGEX="${2:?--regex requires an argument}"; shift ;;
        --dimension) DIMENSION="${2:?--dimension requires an argument}"; shift ;;
        --domain) DOMAIN="${2:?--domain requires an argument}"; shift ;;
        --repeats) REPEATS="${2:?--repeats requires an argument}"; shift ;;
        --skip-range) SKIP_RANGE=1 ;;
        --emit-wrapper) EMIT_WRAPPER="${2:?--emit-wrapper requires an argument}"; shift ;;
        --) shift; break ;;
        -h|--help) print_usage; exit 0 ;;
        *) echo "geopm-sensitivity.sh: unknown option '$1'" >&2
           print_usage >&2; exit 2 ;;
    esac
    shift
done

(( $# )) || { echo "geopm-sensitivity.sh: no command given after --" >&2; exit 2; }
if ! [[ $REPEATS =~ ^[0-9]+$ ]] || (( REPEATS < 2 )); then
    echo "geopm-sensitivity.sh: --repeats must be an integer of 2 or more" >&2
    exit 2
fi

if [[ -n $VENV ]]; then
    [[ -x "$VENV/bin/geopmopt" ]] || { echo "geopm-sensitivity.sh: no geopmopt in '$VENV/bin'" >&2; exit 2; }
    PATH="$VENV/bin:$PATH"; export PATH
fi

for tool in geopmopt geopmsession geopmread; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "geopm-sensitivity.sh: $tool not found on PATH.  See the geopm-install skill." >&2
        exit 2
    }
done

if [[ -n $REGEX ]] && ! command -v python3 >/dev/null 2>&1; then
    echo "geopm-sensitivity.sh: python3 is required to match --regex." >&2
    exit 2
fi

## Resolve the control's grid from the platform

controls=$(geopmopt --list-controls 2>&1) || {
    echo "geopm-sensitivity.sh: could not list controls." >&2
    printf '%s\n' "$controls" | tail -3 >&2
    exit 2
}

read -r ctl_domain ctl_min ctl_max ctl_step < <(
    printf '%s\n' "$controls" | awk -v d="$DIMENSION" 'NR>1 && $1==d {print $2, $4, $5, $6}')

if [[ -z ${ctl_domain:-} ]]; then
    echo "geopm-sensitivity.sh: '$DIMENSION' is not a known dimension." >&2
    printf '%s\n' "$controls" >&2
    exit 2
fi
if [[ $ctl_domain == n/a || $ctl_min == n/a || $ctl_max == n/a || $ctl_step == n/a ]]; then
    echo "geopm-sensitivity.sh: '$DIMENSION' is not usable on this platform" >&2
    echo "  (domain=$ctl_domain min=$ctl_min max=$ctl_max step=$ctl_step)." >&2
    echo "  A dimension is usable only when its domain resolved." >&2
    exit 1
fi

# The dimension name maps to the GEOPM control that geopmwrite understands.
# This must match grid.py exactly: cpu-power in particular is
# POWERCAP::CPU_POWER_LIMIT (POWERCAP iogroup), not the similarly named
# CPU_POWER_LIMIT_CONTROL alias (MSRIOGroup) -- the two are different controls
# despite sharing a description, and testing the wrong one here would measure
# sensitivity to a control geopmopt never actually sweeps.
case "$DIMENSION" in
    cpu-freq|cpu-frequency)                  CONTROL=CPU_FREQUENCY_MAX_CONTROL ;;
    uncore-freq|cpu-uncore-frequency)        CONTROL=CPU_UNCORE_FREQUENCY_MAX_CONTROL ;;
    cpu-power)                               CONTROL=POWERCAP::CPU_POWER_LIMIT ;;
    gpu-freq|gpu-frequency)                  CONTROL=GPU_CORE_FREQUENCY_MAX_CONTROL ;;
    gpu-power)                               CONTROL=GPU_POWER_LIMIT_CONTROL ;;
    board-power)                             CONTROL=BOARD_POWER_LIMIT_CONTROL ;;
    *) echo "geopm-sensitivity.sh: no control mapping for '$DIMENSION'." >&2
       echo "  Supported: cpu-freq, uncore-freq, cpu-power, gpu-freq, gpu-power, board-power" >&2
       exit 2 ;;
esac

# geopmopt prepends CPU_FREQUENCY_GOVERNOR_CONTROL=performance whenever cpu-freq
# is swept, because CPU_FREQUENCY_MAX_CONTROL is only a cap under a scaling
# governor and the requested frequency would not stick.  Mirror that here so the
# sensitivity measurement is taken under the same conditions as the campaign.
GOVERNOR_LINE=""
if [[ $DIMENSION == cpu-freq || $DIMENSION == cpu-frequency ]]; then
    if geopmread CPU_FREQUENCY_GOVERNOR_CONTROL board 0 >/dev/null 2>&1; then
        GOVERNOR_LINE="CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0"
    fi
fi

REF=$ctl_max
REF_LABEL="maximum"
STICKER=""
TURBO_NOTE=""

# Above the sticker frequency the processor is in the turbo range, where the
# requested setting is an upper bound the hardware need not reach.  Under a
# many-core load the achieved frequency is limited by power and thermals
# instead, so neighbouring requests in that range can be indistinguishable.
# Anchor the reference at the sticker so the measurement reflects a setting the
# hardware actually honours.
if [[ $DIMENSION == cpu-freq || $DIMENSION == cpu-frequency ]]; then
    STICKER=$(geopmread CPU_FREQUENCY_STICKER package 0 2>/dev/null)
    if [[ -n $STICKER ]] && awk -v s="$STICKER" -v m="$ctl_max" 'BEGIN{exit !(s > 0 && s < m)}'; then
        REF=$STICKER
        REF_LABEL="sticker"
        TURBO_NOTE="yes"
    fi
fi

STEP_SETTING=$(awk -v m="$REF" -v s="$ctl_step" 'BEGIN{printf "%.0f", m - s}')
MIN_SETTING=$ctl_min

if awk -v a="$STEP_SETTING" -v b="$ctl_min" 'BEGIN{exit !(a < b)}'; then
    echo "geopm-sensitivity.sh: one step below the maximum falls under the minimum;" >&2
    echo "  this control has fewer than two grid points and cannot be swept." >&2
    exit 1
fi

## Measurement helpers

tmp_out=$(mktemp) || exit 1
sig_conf=$(mktemp) || exit 1
ctl_conf=$(mktemp) || exit 1
trap 'rm -f "$tmp_out" "$sig_conf" "$ctl_conf"' EXIT
printf 'TIME board 0\n' > "$sig_conf"

# Run the workload once at the given control value and print one number: the
# scraped figure of merit, or the elapsed seconds when no regex was supplied.
measure_once() {
    local setting=$1 start end rc
    shift
    if [[ -n $setting ]]; then
        { [[ -n $GOVERNOR_LINE ]] && printf '%s\n' "$GOVERNOR_LINE"
          printf '%s %s 0 %s\n' "$CONTROL" "$DOMAIN" "$setting"; } > "$ctl_conf"
        start=$(date +%s.%N)
        geopmsession -i "$sig_conf" --control-config "$ctl_conf" -o /dev/null \
            -- "$@" > "$tmp_out" 2>&1
        rc=$?
        end=$(date +%s.%N)
    else
        start=$(date +%s.%N)
        "$@" > "$tmp_out" 2>&1
        rc=$?
        end=$(date +%s.%N)
    fi
    (( rc == 0 )) || { echo ""; return 1; }
    if [[ -n $REGEX ]]; then
        python3 -c "
import re, sys
m = re.compile(sys.argv[1]).findall(open(sys.argv[2], errors='replace').read())
print(m[-1] if m else '')
" "$REGEX" "$tmp_out" 2>/dev/null
    else
        awk -v s="$start" -v e="$end" 'BEGIN{printf "%.4f", e - s}'
    fi
}

# Collect REPEATS samples at one setting; echo "mean spread_percent".
measure_setting() {
    local setting=$1; shift
    local label=$1; shift
    local values=() v i
    for (( i = 1; i <= REPEATS; i++ )); do
        v=$(measure_once "$setting" "$@")
        if [[ -z $v ]]; then
            echo "  ${label}: run $i FAILED (workload exited non-zero, or the regex did not match)" >&2
            return 1
        fi
        values+=("$v")
        printf '    %-22s run %d: %s\n' "$label" "$i" "$v" >&2
    done
    # The trailing newline matters: `read` reports failure on EOF without one,
    # even when it has assigned the variables.
    printf '%s\n' "${values[@]}" | awk '
        NR==1{min=max=$1}
        {s+=$1; n++; if($1<min)min=$1; if($1>max)max=$1}
        END{m=s/n; printf "%.6f %.4f\n", m, (m>0 ? 100*(max-min)/m : 0)}'
}

# Mean achieved CPU frequency while the workload runs at a requested setting.
# The gap between requested and achieved is what exposes a turbo dead zone.
achieved_freq() {
    local setting=$1; shift
    local report; report=$(mktemp) || return 1
    printf 'CPU_FREQUENCY_STATUS package 0\n' > "$ctl_conf.sig"
    { [[ -n $GOVERNOR_LINE ]] && printf '%s\n' "$GOVERNOR_LINE"
      printf '%s %s 0 %s\n' "$CONTROL" "$DOMAIN" "$setting"; } > "$ctl_conf"
    geopmsession -i "$ctl_conf.sig" --control-config "$ctl_conf" \
        -r "$report" -f yaml -p 0.2 -o /dev/null -- "$@" > /dev/null 2>&1
    grep -A6 'CPU_FREQUENCY_STATUS' "$report" 2>/dev/null \
        | sed -n 's/.*mean: *//p' | head -1
    rm -f "$report" "$ctl_conf.sig"
}

metric_label="figure of merit"
[[ -z $REGEX ]] && metric_label="runtime (s)"

echo "GEOPM sensitivity check: $(hostname)"
echo "==============================================================="
echo "  Command    : $*"
echo "  Dimension  : ${DIMENSION} (${CONTROL}) at ${DOMAIN}"
echo "  Grid       : min=${ctl_min} max=${ctl_max} step=${ctl_step}"
[[ -n $STICKER ]] && echo "  Sticker    : ${STICKER}"
echo "  Reference  : ${REF} (${REF_LABEL})"
[[ -n $GOVERNOR_LINE ]] && echo "  Governor   : performance (forced, mirrors geopmopt)"
echo "  Metric     : ${metric_label}"
echo "  Repeats    : ${REPEATS} per setting"
echo

if [[ -n $TURBO_NOTE ]]; then
    cat <<MSG
  Note: the range above ${STICKER} is turbo, where a requested frequency is an
  upper bound the hardware need not reach.  Measuring from the sticker instead
  of the maximum keeps the reference at a setting that is actually honoured.

MSG
fi

echo "  Measuring..." >&2
read -r ref_mean noise < <(measure_setting "$REF" "reference (${REF_LABEL})" "$@") || exit 1
read -r step_mean step_spread < <(measure_setting "$STEP_SETTING" "one step down" "$@") || exit 1
range_mean=""; range_spread=""
if (( ! SKIP_RANGE )); then
    read -r range_mean range_spread < <(measure_setting "$MIN_SETTING" "minimum" "$@") || exit 1
fi
echo >&2

pct_change() { awk -v a="$1" -v b="$2" 'BEGIN{printf "%.2f", (a>0 ? 100*(b-a>0?b-a:a-b)/a : 0)}'; }

step_delta=$(pct_change "$ref_mean" "$step_mean")
range_delta=""
[[ -n $range_mean ]] && range_delta=$(pct_change "$ref_mean" "$range_mean")

echo "  Results"
echo "  -------------------------------------------------------------"
printf '  %-26s %s\n' "Noise floor (spread at a" ""
printf '  %-26s %s%%\n' "  fixed setting)" "$noise"
printf '  %-26s %s\n' "Mean at ${REF_LABEL}" "$ref_mean"
printf '  %-26s %s   (%s%% change, spread %s%%)\n' \
    "Mean one step down" "$step_mean" "$step_delta" "$step_spread"
if [[ -n $range_mean ]]; then
    printf '  %-26s %s   (%s%% change, spread %s%%)\n' \
        "Mean at minimum" "$range_mean" "$range_delta" "$range_spread"
fi

snr=$(awk -v d="$step_delta" -v n="$noise" 'BEGIN{printf "%.2f", (n>0 ? d/n : (d>0?99:0))}')
printf '  %-26s %s\n' "One-step signal / noise" "$snr"
echo

turbo_dead=0
gap=""
if [[ -n $TURBO_NOTE ]]; then
    echo "  Turbo range check..." >&2
    ach_max=$(achieved_freq "$ctl_max" "$@")
    ach_ref=$(achieved_freq "$REF" "$@")
    if [[ -n $ach_max && -n $ach_ref ]]; then
        printf '  %-26s %s\n' "Achieved asking for max" "$ach_max"
        printf '  %-26s %s\n' "Achieved asking for sticker" "$ach_ref"
        # If asking for the maximum yields barely more than asking for the
        # sticker, every request in between is the same request.
        gap=$(awk -v a="$ach_max" -v b="$ach_ref" 'BEGIN{printf "%.2f", (b>0 ? 100*(a-b)/b : 0)}')
        printf '  %-26s %s%%\n' "Turbo headroom under load" "$gap"
        if awk -v g="$gap" 'BEGIN{exit !(g < 10)}'; then
            turbo_dead=1
        fi
        echo
    fi
fi

## Verdict

verdict=0
echo "  Assessment"
echo "  -------------------------------------------------------------"

range_is_flat=0
if [[ -n $range_delta ]]; then
    if awk -v r="$range_delta" -v n="$noise" 'BEGIN{exit !(r < n)}'; then
        range_is_flat=1
    fi
fi

if (( range_is_flat )); then
    cat <<MSG
  This workload is INSENSITIVE to ${DIMENSION}.

  Across the control's entire range the metric changed by ${range_delta}%, which
  is smaller than the ${noise}% run-to-run noise.  Optimizing this dimension
  cannot succeed: any winner would be noise.

  Drop ${DIMENSION} and test a different dimension.  A workload that does not
  respond to core frequency is often memory-bound, so try uncore-freq.
MSG
    verdict=1
elif awk -v s="$snr" 'BEGIN{exit !(s >= 2)}'; then
    cat <<MSG
  READY.  One step of ${DIMENSION} changes the metric by ${step_delta}%, which is
  ${snr}x the ${noise}% noise floor.  The optimizer can resolve individual grid
  points, so the default step of ${ctl_step} is appropriate.
MSG
elif awk -v s="$snr" 'BEGIN{exit !(s >= 1)}'; then
    cat <<MSG
  MARGINAL.  One step changes the metric by ${step_delta}% against a ${noise}%
  noise floor, a ratio of ${snr}.  The optimizer will resolve broad trends but
  not neighbouring grid points, and the reported best setting will be
  approximate.  Apply one of the remedies below, or accept a coarse answer and
  verify it carefully.
MSG
    verdict=1
else
    cat <<MSG
  NOT READY.  One step changes the metric by only ${step_delta}%, below the
  ${noise}% noise floor (ratio ${snr}).  The optimizer cannot tell one grid point
  from its neighbour, so it will fit noise and report a best setting that does
  not reproduce.
MSG
    verdict=1
fi

if (( turbo_dead )); then
    cat <<MSG

  TURBO DEAD ZONE.  Asking for the maximum ${ctl_max} achieved only ${gap}% more
  frequency than asking for the sticker ${STICKER}.  Above the sticker a
  requested frequency is an upper bound, and under this load the achieved
  frequency is set by power and thermal limits instead.  Every grid point
  between the achieved ceiling and ${ctl_max} is therefore the same operating
  point wearing different labels.

  Current geopmopt already caps the cpu-freq sweep at the sticker by default, so
  you normally get this for free.  Seeing this warning means the sweep range
  reaches into the turbo region -- either an older geopmopt that still defaults
  to the turbo max, or a user who explicitly opted in.  Keep the sweep at or
  below the sticker unless the user asked for turbo:

       --sweep ${DIMENSION}@${DOMAIN}=${ctl_min}:${STICKER}:${ctl_step}

  A search allowed into the turbo range will report a "best" frequency from the
  dead zone, and that result will not reproduce.
MSG
fi

if (( ! range_is_flat )) && ! awk -v s="$snr" 'BEGIN{exit !(s >= 2)}'; then
    # Steps needed for the change to reach twice the noise, assuming the
    # response is locally linear in the control setting.
    steps=$(awk -v d="$step_delta" -v n="$noise" 'BEGIN{v=(d>0 ? 2*n/d : 99); printf "%d", (v<1?1:int(v+0.999))}')
    coarse=$(awk -v s="$ctl_step" -v k="$steps" 'BEGIN{printf "%.0f", s*k}')
    # The suggested step must evenly divide (max - min), or geopmopt rejects it
    # at parse time with "Grid ... is not evenly divisible by step size".
    # Rather than hunting for a coarser step that happens to divide the whole
    # range, raise min by the remainder of (max - min) / coarse: that remainder
    # is always smaller than one coarse step, so min moves by less than a
    # step, and the shifted range divides evenly by construction.
    read -r new_min shifted degenerate < <(awk -v mn="$ctl_min" -v mx="$ctl_max" -v c="$coarse" 'BEGIN{
        r = (mx - mn) % c
        nm = (r == 0) ? mn : mn + r
        printf "%.0f %d %d", nm, (r == 0 ? 0 : 1), (nm >= mx ? 1 : 0)
    }')
    grid_points=$(awk -v mn="$new_min" -v mx="$ctl_max" -v c="$coarse" 'BEGIN{printf "%d", (mx-mn)/c + 1.5}')
    # Averaging R runs shrinks the noise of the mean by sqrt(R).
    reps=$(awk -v d="$step_delta" -v n="$noise" 'BEGIN{v=(d>0 ? (2*n/d)^2 : 999); printf "%d", (v<2?2:int(v+0.999))}')

    cat <<MSG

  Remedies.  The first two the assistant can apply directly.  The rest depend on
  how the workload is launched and how this machine may be configured -- the
  USER is the source of truth for those, so ask; do not guess a pinning line or
  change a machine setting on the user's behalf.  See references/stabilization.md
  for the full menu and the questions to ask.

MSG
    if (( degenerate )); then
        cat <<MSG
  1. Use a coarser step: NOT POSSIBLE on this platform.  A step of ${coarse}
     is needed to clear the noise, but that is at least as large as the whole
     range (${ctl_min} to ${ctl_max}), so no min adjustment leaves room for
     even the endpoints.  Widen the range instead, or move to remedy 2 or 3
     below.
MSG
    else
        cat <<MSG
  1. Use a coarser step (assistant can apply directly).  A change of ${steps}
     step(s) should clear the noise:

       --sweep ${DIMENSION}@${DOMAIN}=${new_min}:${ctl_max}:${coarse}
MSG
        if (( shifted )); then
            cat <<MSG

     min was raised from ${ctl_min} to ${new_min} -- a shift smaller than one
     step -- so the range divides evenly by ${coarse}, as geopmopt requires.
MSG
        fi
        cat <<MSG

     That leaves ${grid_points} grid points, which is ample for a search, and
     it needs nothing from the user.
MSG
    fi

    cat <<MSG

  2. Make each run longer.  Noise is dominated by start-up and scheduling
     effects that do not grow with runtime, so a workload that runs 4x longer
     typically has roughly half the relative spread.  Ask the user to increase
     the iteration count or problem size.

  3. Pin the workload (ask the user -- do not prescribe).  Scheduler migration
     between cores and sockets is usually the largest single source of
     run-to-run variation, so pinning is the most effective lever -- but how to
     pin depends on the workload and its runtime, and only the user knows it.
     There are two distinct kinds:
       - Process affinity, set by the launcher (taskset, numactl, cgroups,
         srun/mpirun binding).  It bounds the process to a CPU mask but lets
         threads float within it.
       - Thread affinity, set by the runtime (OpenMP OMP_PROC_BIND/OMP_PLACES,
         KMP_AFFINITY, GOMP_CPU_AFFINITY, ...).  Without it, threads still
         migrate within the mask -- a common cause of jitter in OpenMP codes.
     Ask which are in use and whether the user can add them, then let the user
     supply the launch wrapper and re-run this check.

  4. Quieten and tune the machine (ask the user).  Stop other work and do not
     share the host during a campaign.  Machine-wide settings (isolated cores,
     limited C-states, disabled SMT, fixed uncore, a performance BIOS profile)
     also reduce jitter but change the whole node -- propose them and let the
     user apply them.  See references/stabilization.md.

  5. Average several runs per grid point (last resort).  Repeating ${reps} times
     per trial would shrink the noise of the mean by sqrt(${reps}) and make one
     step resolvable, at ${reps}x the campaign cost.  Use this only after 1 to 4.
MSG

    if [[ -n $EMIT_WRAPPER ]]; then
        cat > "$EMIT_WRAPPER" <<WRAP
#!/bin/bash
#  Repeat-and-average wrapper generated by geopm-sensitivity.sh.
#
#  geopmopt evaluates a grid point by running this once, so the averaging has
#  to happen inside.  Runs the workload ${reps} times and prints the median,
#  which is more robust to a single slow run than the mean.
#
#  Each geopmopt trial now costs ${reps} workload runs.  Budget accordingly.

set -uo pipefail
REPEATS=\${REPEATS:-${reps}}
values=()
for (( i = 1; i <= REPEATS; i++ )); do
WRAP
        if [[ -n $REGEX ]]; then
            cat >> "$EMIT_WRAPPER" <<WRAP
    out=\$($* 2>&1) || exit 1
    v=\$(printf '%s' "\$out" | python3 -c "
import re, sys
m = re.compile(r'''${REGEX}''').findall(sys.stdin.read())
print(m[-1] if m else '')
")
    [[ -n \$v ]] || { echo "wrapper: metric not found" >&2; exit 1; }
    values+=("\$v")
done
printf '%s\n' "\${values[@]}" | sort -g | awk '{a[NR]=\$1} END{printf "FOM: %s\n", (NR%2 ? a[(NR+1)/2] : (a[NR/2]+a[NR/2+1])/2)}'
WRAP
            wrapper_regex='FOM: ([0-9.eE+-]+)'
        else
            cat >> "$EMIT_WRAPPER" <<WRAP
    s=\$(date +%s.%N)
    $* > /dev/null 2>&1 || exit 1
    e=\$(date +%s.%N)
    values+=("\$(awk -v a="\$s" -v b="\$e" 'BEGIN{printf "%.4f", b-a}')")
done
printf '%s\n' "\${values[@]}" | sort -g | awk '{a[NR]=\$1} END{printf "FOM: %s\n", (NR%2 ? a[(NR+1)/2] : (a[NR/2]+a[NR/2+1])/2)}'
WRAP
            wrapper_regex='FOM: ([0-9.eE+-]+)'
        fi
        chmod +x "$EMIT_WRAPPER"
        cat <<MSG

  Wrapper written to ${EMIT_WRAPPER}.  Use it as the launch command, and scrape
  its output.  Because it prints elapsed time or the raw metric as "FOM:", tell
  geopmopt to minimize or maximize as appropriate:

       geopmopt --sweep ${DIMENSION}@${DOMAIN} \\
                --metric-regex '${wrapper_regex}' \\
                -- ${EMIT_WRAPPER}

  Verify it first:  ${EMIT_WRAPPER}
MSG
    fi
fi

echo
if (( verdict == 0 )); then
    echo "  Next: run the smoke test, then the campaign."
else
    echo "  Next: apply a remedy and re-run this check before starting a campaign."
fi

exit "$verdict"
