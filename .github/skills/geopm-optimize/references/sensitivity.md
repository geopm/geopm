# Sensitivity: can this workload be optimized at all?

Run this **before** any campaign. It answers the question that decides whether
optimizing is possible: **is one step of the control larger than the
run-to-run noise?**

If it is not, the optimizer cannot tell one grid point from its neighbour. It
will fit noise, report a confident-looking best setting, and that setting will
not reproduce. No amount of extra trials fixes it — more samples of noise are
still noise.

```bash
./scripts/geopm-sensitivity.sh --venv ~/geopm-venv \
    --dimension cpu-freq --repeats 3 -- ./workload.sh
```

Exit 0 means optimizable as configured. Exit 1 means apply a remedy first.

## What it measures

Three quantities, each from repeated runs at a fixed setting:

| Quantity | Meaning |
|---|---|
| **Noise floor** | Spread across identical runs at one setting. The resolution limit |
| **One-step change** | Effect of moving the control by a single grid step. The signal |
| **Full-range change** | Effect of the whole range. Confirms the knob does anything |

The verdict is the ratio of one-step change to noise:

| Signal / noise | Verdict | Meaning |
|---|---|---|
| >= 2 | READY | Neighbouring grid points are distinguishable |
| 1 to 2 | MARGINAL | Broad trends resolve, the exact best setting does not |
| < 1 | NOT READY | The search would fit noise |
| Full range < noise | Insensitive | The knob does nothing here; use a different dimension |

## Turbo makes the top of the range meaningless

**Above `CPU_FREQUENCY_STICKER`, a requested frequency is an upper bound the
hardware need not reach.** In the turbo range the achieved frequency is set by
power, thermal, and core-count limits, so several different requests can
produce the same operating point.

Measured on a Xeon Gold 6148 (sticker 2.4 GHz, max 3.7 GHz) under a 40-core
load:

| Requested | Achieved |
|---|---|
| 3.7 GHz | 2.72 GHz |
| 3.3 GHz | 2.73 GHz |
| 2.4 GHz | 2.46 GHz |
| 2.0 GHz | 2.23 GHz |

Requesting 3.7 GHz and 3.3 GHz gave the **same** achieved frequency. Every grid
point from roughly 2.8 GHz to 3.7 GHz is one operating point with ten different
labels. A search allowed into that region will happily report a best frequency
from it, and the result will not reproduce.

This is why the script anchors its reference at the **sticker** rather than the
maximum. Measuring the same workload both ways:

| Reference | One-step change | Noise | Signal / noise | Verdict |
|---|---|---|---|---|
| Turbo maximum, 3.7 GHz | 0.38% | 0.61% | 0.62 | NOT READY |
| Sticker, 2.4 GHz | **6.29%** | 3.43% | **1.83** | MARGINAL |

Sixteen times more signal, from the same workload and the same step size. The
turbo measurement was not telling you about the workload; it was telling you
that the setting was being ignored.

The script reports the turbo headroom under load and, when it is small, tells
you to cap the sweep:

```bash
--sweep cpu-freq@board=1e+09:2400000000:1e+08
```

Capping at the sticker also stops the optimizer wasting trials in a region
where every point is identical.

## Remedies, cheapest first

### 1. Pin the workload

Scheduler migration between cores and sockets is the largest and most easily
removed source of variation, and it costs nothing to try.

```bash
taskset -c 0-19 ./workload.sh
numactl --cpunodebind=0 --membind=0 ./workload.sh    # one socket, local memory
```

Pin memory as well as CPUs when the workload touches a lot of it: a run that
lands on remote memory is slower for reasons unrelated to frequency. Set the
runtime's own affinity too, for example `OMP_PROC_BIND=close` with
`OMP_PLACES=cores`.

Measured effect on the same benchmark:

| | Noise floor | Signal / noise | Verdict |
|---|---|---|---|
| Unpinned, 40 cores across 2 sockets | 3.43% | 1.83 | MARGINAL |
| `numactl` pinned, 20 cores on 1 socket | **1.82%** | **2.23** | **READY** |

Pinning alone moved this workload from unoptimizable to optimizable, with no
change to the step size, the workload length, or the trial budget. Try it
first, then re-run the check.

### 2. Use a coarser step

If one step is too small to resolve, take bigger steps. The script computes how
many are needed and prints the sweep specification:

```bash
--sweep cpu-freq@board=1e+09:3.7e+09:400000000
```

A frequency grid of 7 to 15 points is ample for a Bayesian search. A 28-point
grid whose neighbours are indistinguishable is worse than a 7-point grid whose
neighbours are not.

This assumes the response is locally linear, which holds well enough for
frequency and power over a few steps.

### 3. Make each run longer

Noise is dominated by start-up and scheduling effects that do not grow with
runtime, so relative spread shrinks as the run lengthens. Roughly, a 4x longer
run halves the relative spread. Increase the iteration count or problem size
rather than simply looping the same short run.

Aim for at least 30 seconds. Below about 10 seconds, start-up cost usually
swamps any frequency effect.

### 4. Quieten the machine

Stop other work, and do not share the host during a campaign. A build running
alongside the benchmark is indistinguishable from a frequency effect, and it
will move between trials.

### 5. Average several runs per grid point

Only after the above. `geopmopt` evaluates a grid point with a single run, so
averaging has to happen inside the launch command. Repeating R times shrinks
the noise of the mean by sqrt(R), so resolving a signal half the size of the
noise takes roughly 16 runs per trial — which multiplies an already long
campaign by 16.

The script can generate the wrapper for you:

```bash
./scripts/geopm-sensitivity.sh --venv ~/geopm-venv \
    --emit-wrapper ./repeat.sh -- ./workload.sh
```

It runs the workload N times and prints the **median**, which is more robust to
one slow run than the mean:

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'FOM: ([0-9.eE+-]+)' \
         -- ./repeat.sh
```

Verify the wrapper by hand before using it, and remember every trial now costs N
workload runs. Re-estimate the campaign wall time accordingly.

## Checking more than one dimension

Sensitivity is per dimension. A workload can be insensitive to core frequency
and strongly sensitive to uncore frequency, which is the signature of a
memory-bound code.

```bash
for dim in cpu-freq uncore-freq cpu-power; do
    ./scripts/geopm-sensitivity.sh --venv ~/geopm-venv \
        --dimension "$dim" --repeats 3 --skip-range -- ./workload.sh
done
```

Sweep only the dimensions that pass. A dimension whose full range moves the
metric less than the noise contributes nothing but trials, and it enlarges the
search space the optimizer has to cover.

## Cost

Three settings times `--repeats` runs, plus two short runs for the turbo check.
With `--repeats 3` that is 11 runs, roughly 4 minutes for a 20 second workload.
`--skip-range` removes 3 of them but then cannot distinguish "the knob does
nothing" from "the step is too small".

That is a fraction of a campaign, and it is the difference between a result and
an artifact. The campaign recorded in
[interpreting-results.md](interpreting-results.md) spent 441 seconds to produce
a recommendation that was 11% *slower* than doing nothing; this check would have
predicted that in a quarter of the time.
