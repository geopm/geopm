# geopmopt flags

Every option, with its real default read from `get_parser()` in
`geopmdpy/geopmdpy/optimizer.py`. Verify against the target before use — the
interface has changed during development, and an older snapshot will differ:

```bash
geopmopt --help | grep -c -- --sweep    # 0 means the snapshot is too old
geopmread --version                     # record this with any campaign
```

## Search space

| Flag | Default | Meaning |
|---|---|---|
| `--sweep DIM` | none | Add a dimension. Repeatable. `DIM = CONTROL[@DOMAIN][=MIN:MAX:STEP]`. See [sweep-dimensions.md](sweep-dimensions.md) |
| `--list-controls` | — | Print available dimensions with domain, units, and detected bounds, then exit |

## Objective

| Flag | Default | Meaning |
|---|---|---|
| `--metric NAME=SOURCE` | none | Define a named metric. Repeatable. `regex:`, `signal:`, or `expr:` |
| `--maximize NAME` | none | Maximize the named metric |
| `--minimize [NAME]` | none | With a name, minimize it. Bare, flips the legacy `--metric-regex` sense |
| `--constraint 'NAME OP VALUE'` | none | Feasibility constraint. Repeatable. `<=`, `>=`, `<`, `>`, `==` |
| `--metric-regex PATTERN` | `None` | Legacy figure of merit scraped from stdout |
| `--energy-domain DOMAIN` | `None` | Sample energy and power over `board`, `cpu`, or `gpu`, enabling the reserved `energy` and `power` metrics |
| `--efficiency DOMAIN` | `None` | With `--metric-regex`, optimize FOM per Watt. Without it, minimize total energy over the domain |
| `--metric-bound VALUE` | `None` | Minimize energy subject to the FOM staying at or above VALUE. Requires `--metric-regex` **and** `--efficiency` |
| `--list-metrics` | — | Print reserved metrics and referenced signals, then exit |

With no objective flags at all, the objective is wall-clock runtime.

## Search control

| Flag | Default | Meaning |
|---|---|---|
| `--trials N` | `50` | Total evaluations, including the initial random ones |
| `--n-initial-points N` | `10` | Random evaluations before the Gaussian Process takes over |
| `--random-seed SEED` | `42` | Fixed by default, so repeat runs explore identically |
| `--sample-period SECONDS` | `0.01` | `geopmsession` sampling period for energy objectives |
| `--penalty auto\|none\|NUMBER` | `auto` | How a recoverable trial failure is scored |

`--penalty` decides what happens when a trial times out, exits non-zero, has no
regex match, or reports non-positive power or runtime:

- `auto` — score it worse than every success and continue.
- `none` — abort the whole run on the first failure.
- a number — use that fixed penalty.

Use `none` when any failure means the setup is wrong and continuing wastes
hours. Use `auto` when some configurations are legitimately expected to fail,
for instance a frequency floor that trips a watchdog.

## Application

| Flag | Default | Meaning |
|---|---|---|
| `--application-timeout SECONDS` | `300` | Per-trial timeout |
| `-- LAUNCH ...` | required | Everything after `--` is the command to run |

The default timeout is five minutes. A workload that normally takes four
minutes will fail intermittently under it, since lowering frequency makes runs
slower. Set the timeout from a measured baseline with generous headroom —
[geopm-check-workload.sh](../scripts/geopm-check-workload.sh) recommends one.

## Output

| Flag | Default | Meaning |
|---|---|---|
| `--output-file FILE` | `-` (stdout) | Best configuration, as a `geopmwrite` configuration file |
| `--defer-write` | off | Do not apply the configuration at the end. Requires `--output-file` |
| `--verbosity 0..3` | `1` | 0 ERROR, 1 WARNING, 2 INFO, 3 DEBUG |
| `--print-stdout` | off | Echo the workload's stdout into the log |

Use `--verbosity 2` for any real campaign: level 1 shows no per-trial progress,
so a long run looks like a hang. Add `--print-stdout` when a metric regex is
not matching.

## Dependency rules

Checked at parse time, before the workload runs:

| Rule | Error |
|---|---|
| `--maximize` and `--minimize NAME` are exclusive | `--maximize and --minimize NAME are mutually exclusive` |
| `--metric-bound` needs `--metric-regex` and `--efficiency` | `--metric-bound requires --metric-regex` |
| `--defer-write` needs a real `--output-file` | `Must specify a valid --output-file (not stdout) when using --defer-write` |

The last currently surfaces as an unhandled `ValueError` traceback rather than
a clean message.

## Everything is validated before launch

`geopmopt` parses and checks the entire configuration — dimensions, bounds,
units, metric sources, expression references, constraint names and operators,
and flag dependencies — before starting the workload. A malformed command costs
no trials:

```console
$ geopmopt --sweep cpu-freq=3GHz:1GHz -- ./workload.sh
Error: min exceeds max in '3GHz:1GHz'
$ echo $?
1
```

What is **not** checked up front is whether the controls are writable, whether
the regex will match, or whether the workload runs at all. That is what the
smoke test in [campaign-design.md](campaign-design.md) is for.

## Minimal invocations

Fastest configuration, no instrumentation needed:

```bash
geopmopt --sweep cpu-freq@board --trials 20 -- ./workload.sh
```

Maximize a printed figure of merit:

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 30 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

Lowest energy at no worse than 90% of baseline performance:

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --efficiency cpu \
         --metric-bound 450 \
         --trials 40 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```
