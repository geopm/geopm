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

## Turbo and the governor: handled by default

`geopmopt` now defaults the `cpu-freq` sweep to sensible bounds and settings, so
the manual work-arounds that earlier versions of this skill described are no
longer yours to apply:

- **The upper bound is the sticker, not the turbo maximum.** The default max for
  a `cpu-freq` sweep auto-detects `CPU_FREQUENCY_STICKER` (base/nominal
  frequency) rather than `CPU_FREQUENCY_MAX_AVAIL` (turbo max). You no longer
  cap the sweep by hand.
- **The performance governor is forced.** Whenever `cpu-freq` is a swept
  dimension, `geopmopt` prepends `CPU_FREQUENCY_GOVERNOR_CONTROL=performance`.
  `CPU_FREQUENCY_MAX_CONTROL` alone is only a *cap*: under `powersave`,
  `schedutil`, or another scaling governor the core can still run below the
  requested value, so the requested frequency would not stick. The forced
  governor makes it stick.

The sensitivity probe applies the same governor while it measures, so its
numbers reflect what the campaign will see.

**Why the sticker matters** (the reason the default exists). Above the sticker a
requested frequency is only an upper bound the hardware need not reach. In the
turbo range the achieved frequency is set by power, thermal, and core-count
limits, so several different requests produce the same operating point.

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
labels. Anchoring at the sticker keeps every grid point at a setting the
hardware actually honours. Measuring the same workload both ways:

| Reference | One-step change | Noise | Signal / noise | Verdict |
|---|---|---|---|---|
| Turbo maximum, 3.7 GHz | 0.38% | 0.61% | 0.62 | NOT READY |
| Sticker, 2.4 GHz | **6.29%** | 3.43% | **1.83** | MARGINAL |

Sixteen times more signal, from the same workload and the same step size. The
turbo measurement was not telling you about the workload; it was telling you
that the setting was being ignored.

**Opting into turbo.** Only sweep above the sticker if the user explicitly wants
to study the turbo range, and then say plainly that requests there are an upper
bound and may all resolve to the same achieved frequency:

```bash
--sweep cpu-freq@board=1e+09:3.7e+09:1e+08   # user opted into turbo
```

## Remedies

Apply them in two groups. The first group the assistant can do directly and
deterministically. The second group depends on how the workload is launched and
how the machine may be configured — **the user is the source of truth for those,
and the assistant must ask rather than guess.** The full menu of stabilization
levers, with the questions to ask, is in [stabilization.md](stabilization.md).

### 1. Use a coarser step (assistant can apply directly)

If one step is too small to resolve, take bigger steps. The script computes how
many are needed and prints the sweep specification:

```bash
--sweep cpu-freq@board=1e+09:2.4e+09:400000000
```

A frequency grid of 7 to 15 points is ample for a Bayesian search. A 28-point
grid whose neighbours are indistinguishable is worse than a 7-point grid whose
neighbours are not. Increasing the step is often the quickest way to turn a
MARGINAL dimension into a resolvable one, and unlike pinning it needs nothing
from the user.

This assumes the response is locally linear, which holds well enough for
frequency and power over a few steps.

### 2. Make each run longer

Noise is dominated by start-up and scheduling effects that do not grow with
runtime, so relative spread shrinks as the run lengthens. Roughly, a 4x longer
run halves the relative spread. Increase the iteration count or problem size
rather than simply looping the same short run. This needs the user's help to
change the workload, but it is a simple ask.

Aim for at least 30 seconds. Below about 10 seconds, start-up cost usually
swamps any frequency effect.

### 3. Pin the workload (ask the user — do not prescribe)

Scheduler migration between cores and sockets is usually the largest single
source of run-to-run variation, so pinning is the most effective lever. It is
also the one the assistant must **not** guess: how a workload should be pinned
depends on the workload and its runtime, and only the user knows it.

Two distinct kinds of pinning matter, and they are set in different places:

- **Process affinity** restricts which CPUs the process (and its children) may
  run on. It is set by the *launcher* — `taskset`, `numactl`, `cgroups`,
  `srun --cpu-bind`, `mpirun --bind-to`, and so on. This bounds the process to a
  CPU mask but still lets threads float *within* that mask.
- **Thread affinity** pins each thread to a specific CPU inside the mask. It is
  set by the *runtime* — for OpenMP, `OMP_PROC_BIND` and `OMP_PLACES` (or
  `KMP_AFFINITY` for the Intel runtime, `GOMP_CPU_AFFINITY` for libgomp); other
  runtimes have their own scheme. Process pinning without thread pinning can
  still leave threads migrating within the mask, which is exactly the kind of
  jitter that shows up as noise in an OpenMP STREAM run.

Do not emit a `taskset`/`numactl`/`OMP_PLACES` line as if it were correct for
this workload. Instead ask interview questions 9–10: is process affinity set, is
thread affinity set, and if not can the user add them? Then let the user supply
the launch wrapper and re-run this check. See
[stabilization.md](stabilization.md) for the questions and the option menu.

### 4. Quieten and tune the machine (ask the user)

Stop other work, and do not share the host during a campaign — a build running
alongside the benchmark is indistinguishable from a frequency effect. Beyond
that, a number of OS, kernel-command-line, and BIOS settings reduce jitter
(isolated cores, limited C-states, disabled SMT, fixed uncore, a performance
BIOS profile). These change the whole machine, so the assistant proposes them
and the user decides and applies them. The catalogue is in
[stabilization.md](stabilization.md).

### 5. Average several runs per grid point (last resort)

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

Three settings times `--repeats` runs, plus two short runs for the turbo check
when the sweep reaches into the turbo range (with current `geopmopt`'s
sticker-capped default it usually does not). With `--repeats 3` that is 9 to 11
runs, roughly 4 minutes for a 20 second workload. `--skip-range` removes 3 of
them but then cannot distinguish "the knob does nothing" from "the step is too
small".

That is a fraction of a campaign, and it is the difference between a result and
an artifact. The campaign recorded in
[interpreting-results.md](interpreting-results.md) spent 441 seconds to produce
a recommendation that was 11% *slower* than doing nothing; this check would have
predicted that in a quarter of the time.
