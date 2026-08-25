#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#  Run a workload once at default settings to establish a baseline before a
#  tuning campaign: how long it takes, whether it succeeds, and whether the
#  proposed metric regex matches its output.  Changes no hardware settings.

set -uo pipefail

REGEX=""
BASELINE_RUNS=1
KEEP_OUTPUT=""

print_usage() {
    cat <<'USAGE'
Usage: geopm-check-workload.sh [OPTION]... -- COMMAND [ARG]...

Run COMMAND at default hardware settings and report what a geopmopt campaign
needs to know: exit status, wall time, a recommended --application-timeout,
and whether --metric-regex would match.

Options:
  --regex PATTERN   Candidate --metric-regex.  Must contain one capturing
                    group.  Reported but not required.
  --runs N          Repeat N times to gauge run-to-run variation (default 1).
                    Two or three runs are enough to spot a noisy workload.
  --save-output FILE  Keep the last run's stdout for regex development.
  -h, --help        Print this help message and exit.

No hardware settings are changed and no GEOPM session is opened.

Exit status:
  0  the workload ran and, if a regex was given, it matched
  1  the workload failed, or the regex did not match
  2  usage error
USAGE
}

while (( $# )); do
    case "$1" in
        --regex) [[ $# -ge 2 ]] || { echo "--regex requires an argument" >&2; exit 2; }
                 REGEX="$2"; shift ;;
        --runs) [[ $# -ge 2 ]] || { echo "--runs requires an argument" >&2; exit 2; }
                BASELINE_RUNS="$2"; shift ;;
        --save-output) [[ $# -ge 2 ]] || { echo "--save-output requires an argument" >&2; exit 2; }
                       KEEP_OUTPUT="$2"; shift ;;
        --) shift; break ;;
        -h|--help) print_usage; exit 0 ;;
        *) echo "geopm-check-workload.sh: unknown option '$1'" >&2
           print_usage >&2; exit 2 ;;
    esac
    shift
done

if (( $# == 0 )); then
    echo "geopm-check-workload.sh: no command given after --" >&2
    print_usage >&2
    exit 2
fi

if ! [[ $BASELINE_RUNS =~ ^[0-9]+$ ]] || (( BASELINE_RUNS < 1 )); then
    echo "geopm-check-workload.sh: --runs must be a positive integer" >&2
    exit 2
fi

if [[ -n $REGEX ]] && ! python3 -c "
import re, sys
p = re.compile(sys.argv[1])
sys.exit(0 if p.groups == 1 else 1)
" "$REGEX" 2>/dev/null; then
    echo "geopm-check-workload.sh: --regex must be valid and have exactly one" >&2
    echo "  capturing group.  Example: 'GFLOPS: ([0-9.]+)'" >&2
    exit 2
fi

echo "Workload baseline check"
echo "==============================================================="
echo "  Command : $*"
[[ -n $REGEX ]] && echo "  Regex   : $REGEX"
echo "  Runs    : $BASELINE_RUNS"
echo

stdout_file=$(mktemp) || exit 1
times_file=$(mktemp) || exit 1
values_file=$(mktemp) || exit 1
trap 'rm -f "$stdout_file" "$times_file" "$values_file"' EXIT

failures=0
for (( run = 1; run <= BASELINE_RUNS; run++ )); do
    start=$(date +%s.%N)
    "$@" > "$stdout_file" 2>&1
    rc=$?
    end=$(date +%s.%N)
    elapsed=$(awk -v s="$start" -v e="$end" 'BEGIN{printf "%.3f", e - s}')
    echo "$elapsed" >> "$times_file"

    if (( rc != 0 )); then
        printf '  run %d: FAILED, exit %d after %ss\n' "$run" "$rc" "$elapsed"
        failures=$((failures + 1))
        continue
    fi

    if [[ -n $REGEX ]]; then
        value=$(python3 -c "
import re, sys
pattern = re.compile(sys.argv[1])
text = open(sys.argv[2], errors='replace').read()
matches = pattern.findall(text)
# Take the last match: workloads often print per-iteration values and the
# final one is the summary figure.
print(matches[-1] if matches else '')
" "$REGEX" "$stdout_file" 2>/dev/null)
        if [[ -n $value ]]; then
            printf '  run %d: ok, %ss, metric = %s\n' "$run" "$elapsed" "$value"
            echo "$value" >> "$values_file"
        else
            printf '  run %d: ok, %ss, REGEX DID NOT MATCH\n' "$run" "$elapsed"
            failures=$((failures + 1))
        fi
    else
        printf '  run %d: ok, %ss\n' "$run" "$elapsed"
    fi
done

[[ -n $KEEP_OUTPUT ]] && cp "$stdout_file" "$KEEP_OUTPUT"

echo
mean_time=$(awk '{s+=$1; n++} END{if(n) printf "%.3f", s/n; else print 0}' "$times_file")
max_time=$(awk 'NR==1||$1>m{m=$1} END{printf "%.3f", m+0}' "$times_file")
echo "  Mean wall time : ${mean_time}s"

if (( BASELINE_RUNS > 1 )); then
    spread=$(awk -v mean="$mean_time" '
        NR==1{min=max=$1}
        {if($1<min)min=$1; if($1>max)max=$1}
        END{if(mean>0) printf "%.1f", 100*(max-min)/mean; else print 0}' "$times_file")
    echo "  Time spread    : ${spread}% of mean"
fi

if [[ -s $values_file ]] && (( BASELINE_RUNS > 1 )); then
    metric_spread=$(awk '
        NR==1{min=max=$1}
        {s+=$1; n++; if($1<min)min=$1; if($1>max)max=$1}
        END{if(n && s/n>0) printf "%.1f", 100*(max-min)/(s/n); else print 0}' "$values_file")
    echo "  Metric spread  : ${metric_spread}% of mean"
fi

# Give the optimizer generous headroom: lowering frequency or capping power
# makes every trial slower than this baseline, and a timeout is scored as a
# failed trial rather than a slow one.
timeout_rec=$(awk -v t="$max_time" 'BEGIN{v=t*3; if(v<60) v=60; printf "%d", v+0.5}')
echo "  Suggested      : --application-timeout ${timeout_rec}"

echo
echo "  Assessment:"

if (( failures > 0 )); then
    echo "  - ${failures} of ${BASELINE_RUNS} run(s) failed or did not produce the metric."
    echo "    Fix this before starting a campaign; every trial would fail the"
    echo "    same way."
    if [[ -n $REGEX ]]; then
        echo "    Inspect the real output with --save-output and test the pattern"
        echo "    against it before spending trials."
    fi
fi

short=$(awk -v t="$mean_time" 'BEGIN{print (t < 10) ? 1 : 0}')
long=$(awk -v t="$mean_time" 'BEGIN{print (t > 1800) ? 1 : 0}')

if (( short )); then
    echo "  - Under 10s per run.  Startup cost and measurement noise will"
    echo "    likely swamp the effect of a frequency or power change.  Increase"
    echo "    the problem size or iteration count so a run takes 30s or more."
fi

if (( long )); then
    hours=$(awk -v t="$mean_time" 'BEGIN{printf "%.1f", 30*t/3600}')
    echo "  - Over 30 minutes per run.  A 30-trial campaign would take about"
    echo "    ${hours} hours.  Consider a shorter representative configuration."
fi

if (( BASELINE_RUNS > 1 )) && [[ -s $values_file ]]; then
    noisy=$(awk '
        NR==1{min=max=$1}
        {s+=$1; n++; if($1<min)min=$1; if($1>max)max=$1}
        END{if(n && s/n>0) print (100*(max-min)/(s/n) > 5) ? 1 : 0; else print 0}' "$values_file")
    if (( noisy )); then
        echo "  - Metric varies more than 5% between identical runs.  The"
        echo "    optimizer cannot distinguish a real improvement from noise"
        echo "    smaller than this, so treat close results as ties."
    fi
fi

if (( failures == 0 && ! short && ! long )); then
    est30=$(awk -v t="$mean_time" 'BEGIN{printf "%.1f", 30*t/60}')
    echo "  - Suitable for a campaign.  30 trials would take roughly"
    echo "    ${est30} minutes at baseline speed, and longer in practice"
    echo "    because reduced settings slow each run."
fi

(( failures > 0 )) && exit 1
exit 0
