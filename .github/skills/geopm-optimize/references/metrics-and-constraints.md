# Metrics and constraints

How to tell `geopmopt` what to measure and what to optimize. Transcribed from
`geopmdpy/geopmdpy/metrics.py` and verified against a live system.

There are two interfaces. The **legacy** one is a single figure of merit scraped
from stdout with `--metric-regex`. The **general** one composes several named
metrics with `--metric`, selects one as the objective with
`--maximize`/`--minimize NAME`, and restricts the search with `--constraint`.
Prefer the general interface for anything beyond "make this number bigger".

## Reserved metrics

Four names carry a known unit, so constraints on them are unambiguous.

```console
$ geopmopt --list-metrics
METRIC      UNIT  AVAILABILITY
time        s     always measured
energy      J     needs --energy-domain (or --efficiency), or a signal: metric
power       W     needs --energy-domain (or --efficiency), or a signal: metric
fom         arb   needs a regex: metric (or --metric-regex)
```

`time` is **immutable** — always available and impossible to redefine:

```
Error: metric name 'time' is reserved and provided automatically
```

`energy` and `power` only exist once you tell the optimizer where to measure
them, either with `--energy-domain DOMAIN` or by defining a `signal:` metric of
your own.

## Defining metrics

```
--metric NAME=SOURCE
```

Repeatable. `NAME` must match `[A-Za-z_][A-Za-z0-9_]*`. `SOURCE` is one of
three prefixed forms; the prefix is mandatory.

```
Error: invalid metric source 'CPU_POWER'; expected PREFIX:VALUE (regex:, signal:, or expr:)
Error: unknown metric provider 'foo'; use regex:, signal:, or expr:
```

### regex: scrape a number from stdout

```bash
--metric fom=regex:'GFLOPS: ([0-9.]+)'
```

One capturing group, matched against the workload's standard output. Unit is
`arb` (dimensionless). See [metric-regex.md](metric-regex.md) for how to build
one reliably.

### signal: measure a GEOPM signal during the run

```
signal:SIGNAL@DOMAIN[:AGG]
```

```bash
--metric p=signal:CPU_POWER@board:mean
--metric e=signal:CPU_ENERGY@package        # aggregation defaulted
```

**Domain is restricted to `board`, `cpu`, or `gpu`** — narrower than GEOPM's
full topology, because the optimizer samples at index 0 of the named domain:

```
Error: metric 'p' uses unsupported signal domain 'package';
       use one of board, cpu, gpu
```

Aggregations are `delta`, `mean`, `max`, `min`:

```
Error: unknown aggregation 'median'; use one of delta, mean, max, min
```

Omitting the aggregation picks one from the signal's *behavior*:

| Signal behavior | Default | Typical signals |
|---|---|---|
| monotone (counter) | `delta` | `CPU_ENERGY`, `BOARD_ENERGY` |
| variable | `mean` | `CPU_POWER`, `CPU_FREQUENCY_STATUS` |
| constant | rejected | — |
| label | rejected | — |

The default is usually right: energy counters want the increase over the run,
instantaneous readings want the average. State it explicitly when you want
something else, such as `:max` for a peak power constraint.

Units are inferred from the signal name — `ENERGY` gives J, `POWER` gives W,
`FREQUENCY` gives Hz, `TIME` gives s, anything else `arb`. A metric bound to a
reserved name inherits that name's unit instead.

### expr: derive from other metrics

```bash
--metric eff=expr:'fom / power'
--metric edp=expr:'energy * time'
```

Only arithmetic is permitted: `+ - * / // % **`, unary `+`/`-`, parentheses,
numeric literals, and names of other metrics. No function calls, attribute
access, or comparisons — the expression is parsed to an AST and validated
against an allow-list, never `eval`'d.

References are checked up front:

```
Error: metric 'e' references undefined metric 'nosuch'
```

## Choosing the objective

Exactly one of:

```bash
--maximize NAME
--minimize NAME
```

```
Error: --maximize and --minimize NAME are mutually exclusive
```

`--minimize` with no argument is the legacy form: it flips the sense of the
`--metric-regex` figure of merit, which is maximized by default.

If you give neither, the objective defaults to wall-clock runtime — or to total
energy when `--efficiency DOMAIN` is supplied.

## Constraints

```
--constraint 'NAME OP VALUE'
```

Quote it as a single argument. Repeatable. `OP` is one of `<=`, `>=`, `<`, `>`,
`==`.

```bash
--constraint 'power <= 250W'
--constraint 'energy <= 5000J'
--constraint 'fom >= 100'
```

```
Error: invalid constraint 'p =< 5'; expected 'NAME OP VALUE' with OP one of <=, >=, <, >, ==
Error: constraint references undefined metric 'nosuch'
```

A bare number is interpreted in the metric's canonical unit, so
`'power <= 250'` and `'power <= 250W'` are identical. A suffix on a
dimensionless metric is an error.

### Constraints are soft, not hard

This matters for interpreting results. A violation does not discard the trial;
it adds a penalty to the score:

```
score = objective + Σ weight × scale × max(0, violation)
```

where `scale = 1/|bound|`, which normalizes across units so a Joules constraint
and a Watts constraint contribute comparably.

Selection then prefers feasibility:

- Among trials that satisfy every constraint, the best objective wins.
- If **no** trial is feasible, the one with the smallest total violation wins.

So a campaign always returns something. **Check whether the winner actually
satisfied your constraints** rather than assuming it did — an impossible
constraint yields the least-bad violation, not an error.

## Worked examples

Maximize a figure of merit under a power cap:

```bash
geopmopt --sweep cpu-freq@board \
         --metric fom=regex:'GFLOPS: ([0-9.]+)' \
         --metric power=signal:CPU_POWER@board:mean \
         --maximize fom \
         --constraint 'power <= 250W' \
         --trials 30 \
         -- ./bench.sh
```

Minimize energy while keeping performance above a floor:

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric fom=regex:'items/s: ([0-9.]+)' \
         --energy-domain cpu \
         --minimize energy \
         --constraint 'fom >= 1000' \
         -- ./bench.sh
```

Optimize a derived efficiency metric:

```bash
geopmopt --sweep cpu-freq@board \
         --metric fom=regex:'GFLOPS: ([0-9.]+)' \
         --energy-domain cpu \
         --metric eff=expr:'fom / power' \
         --maximize eff \
         -- ./bench.sh
```

The third is equivalent to `--metric-regex` plus `--efficiency cpu`, but states
the objective explicitly and can be extended with constraints.

## Validation is free

Every metric and constraint is parsed before the workload launches, so a
malformed specification costs no trials and no wall time:

```console
$ geopmopt --sweep cpu-freq --constraint 'nosuch <= 5' -- ./bench.sh
Error: constraint references undefined metric 'nosuch'
$ echo $?
1
```

Use `--list-metrics` to see the reserved names and the signals your `--metric`
flags reference. Note it returns **before** `--sweep` is parsed, so it validates
metrics but not sweep dimensions.
