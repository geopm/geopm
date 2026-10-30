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
  (non-`n/a`) and numerically sane min/max/step (max > 0, min <= max, step > 0).
- `geopmread` returns a plausible power reading.
- At least one dimension is **fully** grantable: its primary control is
  writable *and* every control `geopmopt` writes for it is granted -- the
  `performance` governor for `cpu-freq`, and the paired `*_MIN_*` control for
  `uncore-freq`/`gpu-freq` where the platform exposes it. A writable primary
  control alone is not enough.

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
`geopmsession --hostfile` transport.

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
9. How is the workload placed on the hardware today? Ask separately about
   **process** affinity (is the launch already wrapped in `taskset`,
   `numactl`, `srun --cpu-bind`, `mpirun --bind-to`, a cgroup, ...?) and
   **thread** affinity (does the runtime pin threads — `OMP_PROC_BIND` /
   `OMP_PLACES`, `KMP_AFFINITY`, `GOMP_CPU_AFFINITY`, a TBB/pthread scheme,
   ...?). If neither is set, can the user add it, and do they know the right
   incantation for this workload?
10. Are any OS or platform settings in place to reduce run-to-run noise —
    performance governor, Turbo disabled, isolated cores (`isolcpus` /
    `nohz_full`), limited C-states, SMT disabled, fixed uncore, BIOS profile?
    Which of these can the user change on this machine?

Do not guess the answers to 9 and 10. The user is the sole source of truth for
how their workload should be launched and how this machine may be configured.
The assistant's job is to enumerate the levers in
[stabilization.md](references/stabilization.md) and ask which are exercised —
never to invent a `taskset`/`numactl` line or write a kernel/BIOS setting on the
user's behalf.

Mapping:

| Answer | Consequence |
|---|---|
| No figure of merit | Default objective is runtime, or use `--energy-domain` |
| Lower is better | Bare `--minimize` |
| Varies noticeably | Measure the noise floor before budgeting trials |
| GPU workload | `gpu-freq`, `gpu-power` — confirm they are not `n/a` |
| Goal | Selects a recipe from [objective-recipes.md](references/objective-recipes.md) |
| Distributed | State the single-node limitation |
| No/partial pinning | Offer the levers in [stabilization.md](references/stabilization.md); ask the user to supply the launch wrapper — do not guess it |
| Noise settings unknown | Walk the [stabilization.md](references/stabilization.md) checklist and ask which the user can apply |

### 2. Probe the platform

```bash
./scripts/geopm-probe-controls.sh --venv ~/geopm-venv --workload cpu-bound
```

Reports which dimensions are usable, why the others are not, and suggests a
starting set. Never assume a dimension exists — bounds are platform-specific,
and an unavailable dimension can still print plausible-looking numbers next to
an `n/a` domain.

**Default dimensions: `cpu-freq`, `uncore-freq`, and `cpu-power`** — whichever
of those the probe reports usable, plus `gpu-freq`/`gpu-power` on a GPU
workload. Pick from these unless the user asks otherwise.

**Do not include `prefetch` by default**, even when the probe reports it
usable. Neither helper can constrain it, so a campaign including it has no
baseline reference and no sensitivity screening — its contribution could not
be verified. Add it only when the user explicitly asks to study prefetching,
and then say plainly that its share of any improvement is unverified.

See [sweep-dimensions.md](references/sweep-dimensions.md).

### 3. Baseline the workload

Run once per candidate dimension identified in step 2, passing `--dimension`
(and `--venv`, matching steps 2 and 4) so the baseline is measured under the
same forced conditions that dimension's sweep will actually run under --
governor for `cpu-freq`, and the paired `*_MIN_*` control for uncore/GPU
frequency (`cpu-freq` is MAX-only; `grid.py` excludes
`CPU_FREQUENCY_MIN_CONTROL`) -- not faster, unconstrained defaults the campaign
can never reach:

```bash
./scripts/geopm-check-workload.sh --venv ~/geopm-venv --dimension cpu-freq \
    --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
```

These per-dimension runs are for screening: they establish the noise floor and
a timeout, and they feed step 4's one-knob-at-a-time sensitivity check.
**They are not the reference to judge the campaign against** when more than one
dimension will be swept -- see step 7a.

`prefetch` is the one exception. Neither `geopm-check-workload.sh` nor
`geopm-sensitivity.sh` accepts `--dimension prefetch`, because `geopmopt`
expands a prefetch level into four ordered MSR writes rather than one value.
It is also not a default sweep dimension. If the user has explicitly asked to
sweep it, the baseline simply does not constrain it — say so, and report its
contribution to any improvement as **unverified**.

Establishes runtime, a recommended `--application-timeout`, whether the regex
matches, and the noise floor. See
[sweep-dimensions.md](references/sweep-dimensions.md) for the dimension names
`--dimension` accepts.

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

`prefetch` cannot be tested here either — `geopm-sensitivity.sh` has no control
mapping for it and exits 2. Record it as **untested** rather than running that
command, and weigh it on the recipe's advice instead (worth trying for
memory-bound codes, rarely for compute-bound ones).

Two things to know about `cpu-freq`, both documented with measurements in
[sensitivity.md](references/sensitivity.md):

- **Turbo and the governor are handled by default now.** `geopmopt` caps the
  `cpu-freq` sweep at `CPU_FREQUENCY_STICKER` (the base/nominal frequency) and
  prepends `CPU_FREQUENCY_GOVERNOR_CONTROL=performance` whenever `cpu-freq` is
  swept, so requested frequencies stick instead of being a soft cap under a
  scaling governor. You no longer cap the sweep by hand. Only sweep above the
  sticker if the user *explicitly* wants to study turbo, and then warn that
  requests there are an upper bound the hardware need not reach — several
  distinct requests can resolve to the same achieved frequency.
- **Pinning is the largest removable source of variation, but it is the
  user's call.** Do not prescribe `numactl`/`taskset` or invent an affinity
  line. Ask how the workload is pinned (interview questions 9–10), present the
  levers in [stabilization.md](references/stabilization.md), and let the user
  supply the launch wrapper. Then re-run the check.

When a remedy is needed, apply the ones the agent can do safely and
deterministically first — coarsen the step, lengthen the run — and treat pinning
and machine/OS changes as things to *propose to the user*, not to guess. The
script prints the remedies in that spirit; the last one, averaging repeated runs
per grid point, multiplies campaign cost by the repeat count, so reach for it
only after the others.

### 5. Compose the command

Pick a recipe from
[objective-recipes.md](references/objective-recipes.md); check flag semantics
in [flags.md](references/flags.md) and metric or constraint grammar in
[metrics-and-constraints.md](references/metrics-and-constraints.md).

**Ask whether to save the recommended configuration, and where.**
`geopmopt` only writes a `geopmwrite`-format configuration file when
`--output-file FILE` is given — the default is `-` (stdout), so without this
flag the winning configuration appears once in the terminal output and is
never saved anywhere. Before the full campaign (step 8), ask the user:

- Do they want the winning configuration written to a file? This is required
  to *apply* the result later (`geopmsession --control-config`, see
  [interpreting-results.md](references/interpreting-results.md)) and to record
  the campaign per [campaign-results.md](references/campaign-results.md).
- If yes, what filename and location? Offer a sensible default — e.g.
  `best.conf` inside the campaign's directory under
  [campaign-results.md](references/campaign-results.md)'s layout — but let the
  user override it. Never invent or silently choose a path they have not
  agreed to.

Use their answer as `--output-file` in every command from here on (the smoke
test may still use a disposable path such as `/tmp/smoke.conf`, since it is
only validating the plumbing, not producing the campaign's real result).

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

### 7a. Combined baseline — the reference the result is judged against

Once the final dimension set is fixed, take **one** baseline with all of those
dimensions constrained together, by repeating `--dimension`:

```bash
./scripts/geopm-check-workload.sh --venv ~/geopm-venv \
    --dimension cpu-freq --dimension uncore-freq \
    --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
```

Skipping this and reusing step 3's per-dimension numbers compares the winner
against a run the campaign could never produce. A `cpu-freq + uncore-freq`
campaign constrains *both* on every trial, while step 3's cpu-freq baseline
left uncore unconstrained and its uncore-freq baseline left CPU turbo
unconstrained — neither is reachable by that campaign, so an improvement
measured against either is not a real comparison.

**Pass every dimension in the final set except `prefetch`**, which neither
helper accepts and which is not a default dimension. If it is being swept on
explicit request, state that the baseline leaves the prefetchers unconstrained
and that prefetch's share of any improvement is unverified.

With a single swept dimension this is the same run as step 3; say so and reuse
it rather than repeating the measurement.

### 8. Run

```bash
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' \
         --trials 40 --n-initial-points 10 \
         --application-timeout 300 --verbosity 2 \
         --output-file best.conf \
         -- ./bench.sh 2>&1 | tee campaign.log
```

`--output-file` here is whatever the user agreed to in step 5 — do not fall
back to a hardcoded name like the `best.conf` shown above without asking.

Always `--verbosity 2`: at the default, nothing prints until completion and a
campaign is indistinguishable from a hang.

### 9. Interpret honestly

Compare the improvement against the **combined baseline from step 7a**, using
**that run's noise floor** — it measured the workload under the same joint
constraints the campaign applies. Step 3's per-dimension noise floors left the
other controls unconstrained, so they are not the right yardstick here. Then
verify by running the recommended configuration against that same combined
baseline several times.

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
- `geopmopt` defaults the `cpu-freq` upper bound to `CPU_FREQUENCY_STICKER` and
  forces the performance governor while sweeping. Do not raise the sweep into
  the turbo range unless the user explicitly asks; when they do, state that
  requests above the sticker are not guaranteed and may all resolve to the same
  achieved frequency.
- Never invent a pinning, affinity, or launch wrapper, and never write a kernel
  command-line, BIOS, or OS tunable on the user's behalf. Enumerate the options
  in [stabilization.md](references/stabilization.md) and ask; the user is the
  source of truth for how the workload runs and how the machine may be changed.
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
