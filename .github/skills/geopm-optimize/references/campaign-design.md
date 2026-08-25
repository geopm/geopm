# Campaign design

How many trials, how long it will take, and what to check before committing to
it.

## Estimate before you run

```
total wall time  ≈  trials × single-run time × slowdown factor
```

The slowdown factor is above 1 because most trials run at reduced frequency or
power. Assume **1.5** as a working figure for the campaign average.

The **worst case** is larger and matters for the timeout rather than the total.
A trial at the bottom of the swept range approaches the ratio of the highest to
the lowest setting. Measured on a 3.7 GHz part swept down to 1.0 GHz, a
multi-core benchmark went from 17.0 s to 49.0 s — a factor of 2.9, close to but
below the 3.7 frequency ratio, because not all of the runtime scales with core
frequency.

For a 60-second workload and 40 trials: `40 × 60 × 1.5 ≈ 60 minutes`.

**Present this estimate and get agreement before starting.** A campaign is not
interruptible in any useful way — stopping early loses the model, though not
the machine's settings, which revert.

## Trial budget

Bayesian optimization spends `--n-initial-points` trials sampling randomly, then
uses a Gaussian Process to choose the rest. Too few trials and it never gets
past the random phase.

| Dimensions | `--trials` | `--n-initial-points` |
|---|---|---|
| 1 | 20 | 5 |
| 2 | 40 | 10 |
| 3 | 60 | 15 |
| 4+ | 80+ | 20 |

Rules of thumb:

- Keep `--n-initial-points` around a quarter of `--trials`. The default of 10
  out of 50 is close to that.
- Never run fewer trials than `--n-initial-points`, or the campaign is a pure
  random search.
- Each added dimension multiplies the space. Two dimensions of 27 grid points
  each is 729 combinations; 40 trials samples 5% of it. This works only because
  the response surface is smooth.
- Prefer fewer dimensions with more trials over more dimensions with few.

## Noise decides everything

The optimizer cannot distinguish an improvement smaller than the run-to-run
variation. Measure the noise floor first:

```bash
./scripts/geopm-check-workload.sh --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
```

| Metric spread | Implication |
|---|---|
| under 2% | Good. Standard budgets apply |
| 2–5% | Usable. Add ~50% more trials |
| over 5% | Only large effects are detectable. Lengthen the workload or quieten the machine before spending hours |

A real example of why this matters: in one campaign the same grid coordinate was
evaluated twice and produced scores of `-6.074` and `-3.157` — a factor of two
at an identical setting. Every other difference in that run was smaller than
that, so the reported "best configuration" carried no information.

Reduce noise by lengthening the workload, pinning threads, quiescing the
machine, and avoiding shared hosts. A workload under 30 seconds is usually
dominated by startup cost.

## Reproducibility

`--random-seed` defaults to `42`, so repeated campaigns explore identically.
That is useful for comparing two workloads under the same search, and
misleading if you take a repeat run as independent confirmation. Vary the seed
when you want a genuinely different exploration.

Record with every campaign:

- `geopmread --version`, including the commit suffix
- the exact command line
- the resolved bounds from `--list-controls`
- hostname and platform

Results do not transfer between machines. See
[campaign-results.md](campaign-results.md).

## Sampling period

`--sample-period` defaults to `0.01` seconds and controls how often
`geopmsession` samples for energy objectives. Shorter is more accurate and adds
overhead. The default is fine for runs of tens of seconds; raise it to `0.05`
or `0.1` for very long runs where the sample count would otherwise be large.

Only matters when the objective involves energy or power.

## Failure policy

`--penalty` decides what a failed trial does:

| Value | Behavior | Use when |
|---|---|---|
| `auto` (default) | Score it worse than any success, continue | Some settings may legitimately fail |
| `none` | Abort the run at the first failure | Any failure means the setup is wrong |
| a number | Fixed penalty | You know the right magnitude |

Prefer `none` for a first campaign. Discovering after 40 trials that the regex
never matched is a wasted hour; `auto` will happily penalize every trial and
report a meaningless winner.

Switch to `auto` once the smoke test passes and you expect some configurations
to time out legitimately.

## Timeout

`--application-timeout` defaults to **300 seconds**, which is a trap for
anything that normally takes more than about two minutes: reduced frequency
makes trials slower, and an overrun is scored as a failure rather than a slow
success.

Set it from a measured baseline with generous headroom. Four times the observed
maximum is a reasonable default, and that is what `geopm-check-workload.sh`
recommends. Three times is cutting it fine: the 17.0 s benchmark above took
49.0 s at the bottom of the range, so a 3x margin would have left barely two
seconds of slack.

## The mandatory smoke test

**Never start a full campaign without one.** Run a reduced version first:

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 2 --n-initial-points 2 \
         --penalty none \
         --verbosity 2 \
         --output-file /tmp/smoke.conf \
         -- ./workload.sh
```

`--penalty none` is deliberate: it makes the smoke test fail loudly instead of
absorbing the problem.

Confirm all four:

1. The workload launched and exited zero.
2. The metric was extracted on **both** trials — look for `score=` lines with
   plausible values.
3. Controls were actually written: the two trials used different coordinates and
   produced different scores.
4. A configuration file was written.

Then present the full-campaign time estimate and ask for confirmation.

`geopmopt` already validates the entire configuration — dimensions, bounds,
units, metric sources, expression references, constraint names, flag
dependencies — before launching anything, so syntax errors cost nothing. What
the smoke test adds is proof that the workload runs, the regex matches, and the
controls are writable. Those are exactly the failures that would otherwise
surface after an hour.

## A worked plan

For a 90-second CPU-bound benchmark on a two-socket machine:

```bash
# 1. What can this platform sweep?
./scripts/geopm-probe-controls.sh --venv ~/geopm-venv --workload cpu-bound

# 2. Baseline and noise floor
./scripts/geopm-check-workload.sh --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
#    -> mean 92s, metric spread 1.8%, suggests --application-timeout 300

# 3. Smoke test, about 5 minutes
geopmopt --sweep cpu-freq@board --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 2 --n-initial-points 2 --penalty none \
         --application-timeout 300 --verbosity 2 -- ./bench.sh

# 4. Estimate: 40 x 92s x 1.5 = about 92 minutes.  Confirm, then:
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 40 --n-initial-points 10 \
         --application-timeout 300 --verbosity 2 \
         --output-file best.conf \
         -- ./bench.sh 2>&1 | tee campaign.log

# 5. Verify the recommendation beats baseline by more than 1.8%
```

Step 5 is not optional. A campaign produces a hypothesis, not a result.
