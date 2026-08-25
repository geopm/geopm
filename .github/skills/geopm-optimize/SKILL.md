---
name: geopm-optimize
description: 'Tune hardware settings for a workload with geopmopt. Use for "optimize my benchmark", "best CPU frequency for my workload", "reduce energy consumption", "performance per watt", "energy efficiency tuning", "power cap sweep", "what frequency should I run at", "geopmopt". Guides a user who knows their workload but not GEOPM through choosing sweep dimensions, defining an objective and constraints, running a Bayesian optimization campaign, and judging whether the result is real. Requires GEOPM to be installed; delegates to geopm-install when it is not.'
argument-hint: 'Describe your workload and what you want to optimize'
---

# GEOPM Optimize Assistant

Take a user who knows how to run their workload but has never used GEOPM, and
produce a hardware configuration that measurably improves it — or an honest
statement that no improvement was demonstrated.

## Prerequisite: the readiness gate

Do not start until all three hold:

- `geopmopt --list-controls` runs and lists at least one dimension with real
  (non-`n/a`) min, max, and step values.
- `geopmread` returns a plausible power reading.
- At least one sweepable control is writable by the invoking user.

Check it:

```bash
./scripts/geopm-verify-install.sh --venv ~/geopm-venv   # from the geopm-install skill
```

It exits 0 exactly when the gate is met. **If it fails, hand off to the
`geopm-install` assistant rather than working around it.** In particular,
`geopmopt` is absent from every tagged GEOPM release, so on most systems it
requires a virtual environment built from the `dev` branch.

If the user is new to GEOPM, read
[Concepts](../../../docs/source/concepts.rst) first.

## Scope: one node

v1 tunes and measures **only the node running `geopmopt`**. If the workload is
distributed, say so before proposing anything, and offer to tune a single
representative node instead. Multi-node support is deferred pending a
`geopmsession --hostfile` transport; see `plan-for-agents.md`.

Also flag the shared-machine problem: a campaign changes package-wide or
board-wide settings, so it affects every other workload on the node and is
distorted by them in turn. Prefer a quiet machine.

## Procedure

### 1. Interview

Ask these in order. Each answer maps to a flag or a recipe.

1. What single command runs your workload end to end?
2. Roughly how long does one run take?
3. Does it print a figure of merit? Paste one run's output.
4. Is that number better when higher or lower?
5. Does it vary between identical runs?
6. CPU-only, or does it use GPUs?
7. What is your goal — fastest, lowest energy, best performance per watt,
   lowest energy subject to a performance floor, or fastest under a power cap?
8. Single machine, or a distributed run?

Mapping:

| Answer | Consequence |
|---|---|
| No figure of merit | Default objective is runtime, or use `--energy-domain` |
| Lower is better | Bare `--minimize` |
| Varies noticeably | Measure the noise floor before budgeting trials |
| GPU workload | `gpu-freq`, `gpu-power` — confirm they are not `n/a` |
| Goal | Selects a recipe from [objective-recipes.md](references/objective-recipes.md) |
| Distributed | State the single-node limitation |

### 2. Probe the platform

```bash
./scripts/geopm-probe-controls.sh --venv ~/geopm-venv --workload cpu-bound
```

Reports which dimensions are usable, why the others are not, and suggests a
starting set. Never assume a dimension exists — bounds are platform-specific,
and an unavailable dimension can still print plausible-looking numbers next to
an `n/a` domain.

See [sweep-dimensions.md](references/sweep-dimensions.md).

### 3. Baseline the workload

```bash
./scripts/geopm-check-workload.sh --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
```

Establishes runtime, a recommended `--application-timeout`, whether the regex
matches, and the noise floor.

Regex help: [metric-regex.md](references/metric-regex.md).

### 4. Sensitivity check — mandatory

**The single most important step. Do not skip it.**

```bash
./scripts/geopm-sensitivity.sh --venv ~/geopm-venv \
    --dimension cpu-freq --repeats 3 -- ./bench.sh
```

A noise floor on its own is not enough. What decides whether optimization is
possible is whether **one step of the control moves the metric further than the
noise does**. If it does not, the optimizer cannot distinguish neighbouring grid
points; it will fit noise and report a best setting that does not reproduce.
More trials do not help — more samples of noise are still noise.

The script exits 0 when the workload is optimizable as configured, and 1 with
ranked remedies when it is not. Run it for each dimension you intend to sweep,
and sweep only the ones that pass.

Two effects dominate in practice, both documented with measurements in
[sensitivity.md](references/sensitivity.md):

- **Turbo.** Above `CPU_FREQUENCY_STICKER` a requested frequency is only an
  upper bound. Under load, requesting 3.7 GHz and 3.3 GHz produced the *same*
  achieved 2.72 GHz on a test machine, making a third of the sweep range one
  operating point with many labels. Cap the sweep at the sticker.
- **Pinning.** Scheduler migration is usually the largest removable source of
  variation. On the same workload, `numactl --cpunodebind=0 --membind=0` cut the
  noise floor from 3.43% to 1.82% and moved the verdict from MARGINAL to READY
  with no other change. Try it first.

When a remedy is needed, prefer them in the order the script prints: pin, then
coarsen the step, then lengthen the run, then quieten the machine, and only then
average repeated runs per grid point — that last one multiplies campaign cost by
the repeat count.

### 5. Compose the command

Pick a recipe from
[objective-recipes.md](references/objective-recipes.md); check flag semantics
in [flags.md](references/flags.md) and metric or constraint grammar in
[metrics-and-constraints.md](references/metrics-and-constraints.md).

### 6. Smoke test — mandatory

```bash
geopmopt --sweep cpu-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 2 --n-initial-points 2 \
         --penalty none --verbosity 2 \
         --output-file /tmp/smoke.conf \
         -- ./bench.sh
```

`--penalty none` makes failures abort rather than be absorbed. Confirm: the
workload ran, the metric was extracted on both trials, the two trials used
different coordinates and produced different scores, and a configuration file
was written.

`geopmopt` already validates the whole configuration before launching anything,
so syntax errors cost nothing. The smoke test proves the parts it cannot check:
that the workload runs, the regex matches, and the controls are writable.

### 7. Estimate, then confirm

```
total ≈ trials × single-run time × 1.5
```

**Present the estimate and get explicit agreement before starting.** A 40-trial
campaign on a 90-second workload is about 90 minutes.

See [campaign-design.md](references/campaign-design.md) for trial budgets.

### 8. Run

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 40 --n-initial-points 10 \
         --application-timeout 300 --verbosity 2 \
         --output-file best.conf \
         -- ./bench.sh 2>&1 | tee campaign.log
```

Always `--verbosity 2`: at the default, nothing prints until completion and a
campaign is indistinguishable from a hang.

### 9. Interpret honestly

Compare the improvement against the **noise floor from step 3**. Then verify by
running the recommended configuration against the baseline several times.

See [interpreting-results.md](references/interpreting-results.md).

### 10. Record

Save the log, the configuration, the command line, and the noise floor to the
campaign store so a later session can compare. See
[campaign-results.md](references/campaign-results.md).

## Judging the result

A campaign **always** reports a best configuration, including when the data
supports nothing. Before presenting it as a finding, check:

- Is the improvement larger than the run-to-run noise?
- Is the ordering physically plausible? A CPU-bound workload should not get
  slower at higher frequency.
- Did the winner actually satisfy the constraints? They are soft — an
  impossible constraint yields the least-infeasible trial, not an error.
- Is the best value at a search-space boundary, suggesting the optimum lies
  outside the range?

Reporting "no reliable improvement was demonstrated" is a correct and useful
outcome. Presenting a noise artifact as a tuning result is not.

## Applying a result

Settings do not survive the campaign — session save and restore reverts them.
To use a configuration, hold a session open:

```bash
printf 'TIME board 0\n' > sig.conf
geopmsession -i sig.conf --control-config best.conf -o /dev/null -- ./production-run.sh
```

`geopmsession` needs a signal request even when only applying controls;
omitting `-i` makes it consume the surrounding script as stdin.

## Safety

- Present a wall-time estimate and get confirmation before any full campaign.
- Never skip the sensitivity check. A campaign on a dimension whose single step
  is below the noise floor cannot produce a real result, and running one wastes
  the user's hours to manufacture an artifact.
- Never skip the smoke test.
- Never sweep a frequency range above `CPU_FREQUENCY_STICKER` without saying
  that requests there are not guaranteed and may all resolve to the same
  achieved frequency.
- Never write controls outside a `geopmopt` or `geopmsession` session.
- State plainly that a campaign repeatedly changes power and frequency limits
  for the whole node, and confirm the user is authorized to do that on this
  machine.
- Never assert that a signal, control, sweep dimension, or flag exists without
  verifying it on the target with `--list-controls`, `--list-metrics`, or
  `geopmread --info-all`.
- Never imply the campaign covers more than the node running `geopmopt`.

## Troubleshooting

[troubleshooting.md](references/troubleshooting.md) covers regex misses,
timeouts, `n/a` dimensions, noise-dominated results, unsatisfiable constraints,
and the `geopmsession` stdin trap.
