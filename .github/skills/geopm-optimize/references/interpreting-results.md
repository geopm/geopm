# Interpreting results

What a campaign produces, how to apply it, and — more importantly — how to tell
whether the answer means anything.

## What you get

Two things: a log of trials on stderr, and a configuration file.

```
INFO: Starting Bayesian optimization with 6 evaluations...
INFO: Evaluation 1: coordinate=[22], score=-6.605
INFO: Evaluation 2: coordinate=[5], score=-6.074
INFO: Evaluation 3: coordinate=[21], score=-6.458
INFO: Evaluation 4: coordinate=[25], score=-7.223
INFO: Evaluation 5: coordinate=[27], score=-5.802
INFO: Evaluation 6: coordinate=[5], score=-3.157
INFO: Optimization completed!
INFO: Best metric: 7.223
INFO: Best coordinate: [25]
INFO: Number of evaluations: 6
Best configuration:
CPU_FREQUENCY_MAX_CONTROL board 0 3500000000.0
```

The log appears only at `--verbosity 2` or higher. At the default level 1 a
campaign prints nothing until it finishes, which is indistinguishable from a
hang.

### Reading the log

**`score` is in minimize sense.** A maximized figure of merit appears negated,
so `score=-6.605` means the workload reported 6.605. Lower score is always
better; `-7.223` is the best row above.

**`coordinate` is an index into the sweep grid, not a setting.** Convert it
using the bounds from `--list-controls`:

```
value = min + index x step
```

For a `cpu-freq` grid of `min=1e9, step=1e8`, coordinate 25 means
`1e9 + 25 x 1e8 = 3.5 GHz`. With several dimensions the coordinate has one
entry per `--sweep`, in the order the flags were given.

### The configuration file

`--output-file` writes lines in `geopmwrite` batch format:

```
CPU_FREQUENCY_MAX_CONTROL board 0 3500000000.0
```

`CONTROL_NAME DOMAIN_TYPE DOMAIN_INDEX VALUE`, one per swept dimension.

## Applying the result

The settings a campaign applied **do not survive it**. Verified on the run
above: the control read 3.7 GHz before, and 3.7 GHz again afterwards, even
though the campaign selected 3.5 GHz. Session save and restore reverts
everything when `geopmopt` exits.

To actually use the configuration, hold a session open for the duration of the
work. Note that `geopmsession` still needs a signal request stream even when
you only want it to apply controls, so pass one with `-i`; otherwise it will
consume the surrounding script as stdin and fail with
`Unable to convert values into a read request`:

```bash
printf 'TIME board 0\n' > sig.conf
geopmsession -i sig.conf --control-config best.conf -o /dev/null \
    -- ./production-run.sh
```

Verified behavior on a live system: the control read 3.7 GHz before, 3.515 GHz
inside the session, and 3.7 GHz again afterwards.

That 3.515 GHz is worth noting. The configuration requested 3.5 GHz exactly,
but the hardware snapped to the nearest supported step. **Read back what was
actually applied** rather than assuming the requested value took effect:

```bash
geopmread CPU_FREQUENCY_MAX_CONTROL package 0
```

For an interactive session, keep a `geopmsession` running in another terminal,
or apply with `geopmwrite` and accept that it lasts only as long as that
process.

`--defer-write` suppresses the end-of-campaign application, which is what you
want when the campaign is exploratory.

## Deciding whether the result is real

This is the part that matters, and the example above is a cautionary one.

### Check the noise floor first

Coordinate `[5]` was evaluated twice, at the same 1.5 GHz setting, and scored
`-6.074` and `-3.157`. **The same configuration produced a 2x different
result.** Any difference between configurations smaller than that spread is
meaningless.

Compare the winning score against that floor. Here the best (`-7.223`) and the
worst non-repeat (`-5.802`) differ by less than the noise seen at a single
setting, so **this campaign established nothing**, despite reporting a
confident-looking best configuration.

Establish the floor before you start, with repeated identical runs:

```bash
./scripts/geopm-check-workload.sh --regex 'GFLOPS: ([0-9.]+)' --runs 3 -- ./bench.sh
```

If it reports a metric spread above about 5%, expect to need many more trials,
a longer workload, or a quieter machine.

### Look for physically implausible orderings

In the example, 3.7 GHz (coordinate 27) scored *worse* than 3.5 GHz for a
CPU-bound workload. That ordering has no physical explanation and is a strong
signal that noise dominates.

Sanity checks worth applying:

- A CPU-bound workload should get faster with frequency, up to the point where
  power capping intervenes.
- A memory-bound workload should be insensitive to core frequency and sensitive
  to uncore frequency.
- Energy should fall as frequency falls, until the longer runtime outweighs the
  lower power.

When the result contradicts the physics, suspect the measurement.

### Other signs the campaign did not converge

| Symptom | Meaning |
|---|---|
| Best value sits at a search-space boundary | The optimum may lie outside the range. Widen it with `=MIN:MAX:STEP` |
| Best equals the platform default | Either the default is genuinely optimal, or the controls had no effect. Confirm they were writable |
| All scores within the noise floor | No usable signal. Longer runs, more trials, or a quieter machine |
| `UserWarning: The objective has been evaluated at point ... before` | The optimizer is re-sampling. With few trials this is normal; if frequent, the grid is small relative to the budget |

That warning appeared in the example above and is emitted by `scikit-optimize`,
not GEOPM. It means the model wanted a point it had already tried and a random
one was substituted.

## Verifying the recommendation

A campaign result is a hypothesis. Test it directly, with enough repetitions to
beat the noise floor:

```bash
# Baseline, three runs
for i in 1 2 3; do ./bench.sh; done

# Recommended configuration, three runs
printf 'TIME board 0\n' > sig.conf
for i in 1 2 3; do
    geopmsession -i sig.conf --control-config best.conf -o /dev/null -- ./bench.sh
done
```

If the recommended configuration does not reproducibly beat the baseline by
more than the noise floor, it is not a real improvement. Report that honestly
rather than presenting the optimizer's choice as a finding.

## Reporting

A useful summary states:

- The objective, and whether constraints were satisfied.
- The recommended configuration in physical units, not coordinates.
- The improvement over baseline, **with the noise floor alongside it**.
- The trial count and how much of the space was explored.
- The GEOPM version and host, since results are not portable between machines.

Example of an honest negative result:

> Swept `cpu-freq@board` over 1.0–3.7 GHz in 6 trials. Best was 3.5 GHz at
> 7.22 GFLOPS versus 6.39 at the 3.7 GHz default. However, repeated runs at a
> single setting varied by a factor of two, so this difference is within noise.
> No reliable improvement was demonstrated. A longer workload and at least 20
> trials would be needed to draw a conclusion.

## Constraints need a separate check

Constraints are soft: a violating trial is penalized, not discarded, and if no
trial is feasible the least-infeasible one is returned. So a campaign always
reports a winner even when your constraint was impossible.

Confirm the winner actually satisfied it by measuring under the recommended
configuration:

```bash
printf 'TIME board 0\nCPU_POWER board 0\n' > sig.conf
geopmsession -i sig.conf -r report.yaml -p 0.05 \
    --control-config best.conf -- ./bench.sh
```

Then compare the reported power against the bound you set.
