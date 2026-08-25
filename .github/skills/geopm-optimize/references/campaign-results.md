# Campaign results

Where to keep what a campaign produced, so a later session can compare runs or
resume the work.

`geopmopt` writes only the winning configuration via `--output-file`. Everything
else — the trial history, the command line, the platform identity — exists only
in the terminal. Capture it deliberately.

## Layout

One directory per campaign, under a per-user root:

```
${XDG_DATA_HOME:-$HOME/.local/share}/geopm/campaigns/
└── 2026-08-24T17-32-46_saruman_dgemm/
    ├── campaign.yaml      # metadata and outcome
    ├── command.txt        # exact command line
    ├── best.conf          # --output-file result
    ├── campaign.log       # full stderr trial history
    ├── controls.txt       # --list-controls at campaign time
    └── baseline.txt       # geopm-check-workload.sh output
```

Directory name: `TIMESTAMP_HOST_LABEL`, with an ISO-8601 timestamp using `-`
instead of `:` so it is safe on any filesystem.

## campaign.yaml

YAML to match `geopmsession --report-out`, which is the format the rest of
GEOPM uses for reports.

```yaml
schema: geopm-campaign/1
label: dgemm
created: "2026-08-24T17:32:46-07:00"

host:
  hostname: saruman.ra.intel.com
  cpu_model: "Intel(R) Xeon(R) Gold 6148 CPU @ 2.40GHz"
  packages: 2
  cpus: 80

geopm:
  version: "3.2.1.dev380+g9854e3735"
  source: "venv:/home/user/geopm-venv"

workload:
  command: "/tmp/bench10.sh"
  baseline_seconds: 6.27
  baseline_metric: 6.393
  metric_spread_percent: 7.7

search:
  dimensions:
    - name: cpu-freq
      domain: board
      min: 1.0e9
      max: 3.7e9
      step: 1.0e8
  trials: 6
  n_initial_points: 3
  random_seed: 42

objective:
  kind: maximize
  metric: fom
  source: "regex:GFLOPS: ([0-9.]+)"
  constraints: []

result:
  best_metric: 7.223
  best_coordinate: [25]
  best_settings:
    CPU_FREQUENCY_MAX_CONTROL@board:0: 3.5e9
  baseline_metric: 6.393
  improvement_percent: 13.0
  noise_floor_percent: 7.7
  verdict: inconclusive
  notes: >
    Improvement is within the measured noise floor.  The same coordinate
    scored -6.074 and -3.157 on repeat evaluation.  Not a reliable result.
```

Two fields carry most of the value.

`noise_floor_percent` records the run-to-run variation measured **before** the
campaign. Without it, a later reader cannot tell whether a 13% improvement was
real.

`verdict` is one of `improved`, `inconclusive`, `no_effect`, or `failed`, and
must be set honestly. A campaign that produced a number but could not
distinguish it from noise is `inconclusive`, not `improved`.

## Recording a campaign

```bash
LABEL=dgemm
ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/geopm/campaigns"
DIR="$ROOT/$(date -Is | tr ':' '-')_$(hostname -s)_${LABEL}"
mkdir -p "$DIR"

geopmopt --list-controls > "$DIR/controls.txt"

geopmopt --sweep cpu-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 40 --verbosity 2 \
         --output-file "$DIR/best.conf" \
         -- ./bench.sh 2>&1 | tee "$DIR/campaign.log"
```

Save the command line itself, since reconstructing it from memory is
error-prone:

```bash
history 1 | sed 's/^ *[0-9]* *//' > "$DIR/command.txt"
```

## Using prior campaigns

At the start of a session, check for earlier work on the same host and
workload:

```bash
ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/geopm/campaigns"
ls -1 "$ROOT" 2>/dev/null | tail -10
grep -l "label: dgemm" "$ROOT"/*/campaign.yaml 2>/dev/null
```

Offer to:

- **Compare** — did the recommendation change, and is the difference larger
  than the noise floor?
- **Extend** — rerun with more trials or a different seed. The optimizer cannot
  resume a previous model, so this is a fresh search, not a continuation.
- **Re-verify** — check that a past recommendation still holds after a firmware,
  kernel, or GEOPM change.

## Results are not portable

A campaign's answer applies to the host it ran on. Do not reuse a configuration
across machines without re-verifying, because:

- Bounds differ. A frequency valid on one part is out of range on another.
- Silicon varies. Manufacturing variation alone shifts the optimum.
- The workload may be configured differently — thread count, problem size,
  affinity.
- Firmware and microcode changes alter power and frequency behavior.

Record `geopmread --version` and the CPU model precisely so a stale result can
be recognized later.

## Cleaning up

Campaign directories are small — logs and a few text files — but accumulate.
Prune old ones periodically:

```bash
find "${XDG_DATA_HOME:-$HOME/.local/share}/geopm/campaigns" \
     -maxdepth 1 -type d -mtime +90 -print
```

Review before deleting; an inconclusive campaign is still evidence that a
particular approach did not work, which is worth keeping longer than a
successful one.
