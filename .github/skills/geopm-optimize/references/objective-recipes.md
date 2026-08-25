# Objective recipes

Named starting points. Pick by what the user actually wants, substitute their
sweep dimensions and regex, and adjust the trial budget per
[campaign-design.md](campaign-design.md).

Every recipe assumes the readiness gate has passed and `--list-controls` has
confirmed the dimensions exist on the target.

## Fastest run

No instrumentation needed. With no objective flags the optimizer minimizes
wall-clock runtime.

```bash
geopmopt --sweep cpu-freq@board \
         --trials 20 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

Use when the workload prints no figure of merit, or when "finish sooner" is the
whole goal. The answer is usually the maximum frequency unless the workload is
power-limited or memory-bound, which makes this a good sanity check that the
controls are having an effect at all.

## Maximize a figure of merit

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 40 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

The regex is maximized by default. Add a bare `--minimize` if the number is
better when lower, such as a latency or a time.

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'elapsed: ([0-9.]+) s' --minimize \
         -- ./workload.sh
```

## Minimize energy

No figure of merit required. `--efficiency DOMAIN` without `--metric-regex`
minimizes total energy over that domain.

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --efficiency cpu \
         --trials 40 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

Domain is `board`, `cpu`, or `gpu`. Expect the answer to be a *reduced*
frequency: energy is power times time, so slowing down helps until the extra
runtime costs more than the power saved. That turning point is what this finds.

**Watch for the degenerate case.** Minimizing energy with no performance
constraint will happily recommend the lowest frequency in the range if the
workload's power falls faster than its runtime grows. If a doubled runtime is
unacceptable, use the next recipe instead.

## Best performance per Watt

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --efficiency cpu \
         --trials 30 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

With both flags the objective becomes the figure of merit divided by average
power over the domain. This is the recipe for a sustainability or
efficiency-of-fleet question.

Equivalent, but explicit and extensible:

```bash
geopmopt --sweep cpu-freq@board \
         --metric fom=regex:'GFLOPS: ([0-9.]+)' \
         --energy-domain cpu \
         --metric eff=expr:'fom / power' \
         --maximize eff \
         -- ./workload.sh
```

Prefer the explicit form when you also want constraints.

## Minimize energy subject to a performance floor

The recipe most people actually want: save energy, but not at any cost.

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --efficiency cpu \
         --metric-bound 450 \
         --trials 40 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

`--metric-bound` requires both `--metric-regex` and `--efficiency`. The bound is
in the figure of merit's own units, so measure the baseline first and set it as
a fraction of that — 450 above is 90% of a 500 GFLOPS baseline.

The general-interface equivalent, which reads more clearly and allows several
constraints:

```bash
geopmopt --sweep cpu-freq@board \
         --metric fom=regex:'GFLOPS: ([0-9.]+)' \
         --energy-domain cpu \
         --minimize energy \
         --constraint 'fom >= 450' \
         -- ./workload.sh
```

## Fastest under a power cap

For a machine or rack with a power budget.

```bash
geopmopt --sweep cpu-freq@board --sweep cpu-power@board \
         --metric fom=regex:'GFLOPS: ([0-9.]+)' \
         --metric power=signal:CPU_POWER@board:mean \
         --maximize fom \
         --constraint 'power <= 250W' \
         --trials 40 --verbosity 2 \
         --output-file best.conf \
         -- ./workload.sh
```

Constraints are soft, so **verify the winner satisfied the cap** rather than
assuming it did. See [interpreting-results.md](interpreting-results.md).

Use `:max` instead of `:mean` if the cap is a hard instantaneous limit rather
than an average:

```bash
--metric power=signal:CPU_POWER@board:max --constraint 'power <= 250W'
```

## Several constraints at once

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric fom=regex:'items/s: ([0-9.]+)' \
         --metric power=signal:CPU_POWER@board:mean \
         --energy-domain cpu \
         --maximize fom \
         --constraint 'power <= 200W' \
         --constraint 'energy <= 15000J' \
         --trials 60 --verbosity 2 \
         -- ./workload.sh
```

Each constraint is normalized by its own bound, so a Joules limit and a Watts
limit contribute comparably to the penalty rather than the larger number
dominating.

## Energy-delay product

A common HPC figure that balances the two without an arbitrary bound.

```bash
geopmopt --sweep cpu-freq@board \
         --energy-domain cpu \
         --metric edp=expr:'energy * time' \
         --minimize edp \
         --trials 30 --verbosity 2 \
         -- ./workload.sh
```

`time` is always available and needs no definition. Use `energy * time * time`
for the ED2P variant, which weights runtime more heavily.

## Memory-bound workload

Lead with uncore frequency, and consider prefetchers.

```bash
geopmopt --sweep uncore-freq@board --sweep cpu-freq@board --sweep prefetch \
         --metric-regex 'bandwidth: ([0-9.]+) GB/s' \
         --trials 60 --verbosity 2 \
         -- ./stream.sh
```

A memory-bound code is often insensitive to core frequency, so a large
improvement from `cpu-freq` alone is a hint the workload is not as
memory-bound as assumed.

## Choosing between them

| The user says | Recipe |
|---|---|
| "make it finish faster" | Fastest run, or maximize FOM |
| "reduce our energy bill" | Minimize energy, with a floor if performance matters |
| "improve performance per watt" | Best performance per Watt |
| "we have a power budget" | Fastest under a power cap |
| "save energy without slowing down much" | Minimize energy subject to a floor |
| "balance both" | Energy-delay product |

When the answer is unclear, ask which they would accept: a 10% slowdown for a
20% energy saving, or not. That single question separates the constrained
recipes from the unconstrained ones.
