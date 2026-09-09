# Troubleshooting a campaign

Install-side problems (`geopmopt` missing, controls not granted, `geopmd` down)
belong to the `geopm-install` skill. This page covers failures once `geopmopt`
runs.

## The regex never matches

Every trial is scored as a failure and the campaign reports a meaningless
winner.

```bash
# Capture real output, then iterate offline
./scripts/geopm-check-workload.sh --save-output out.txt --regex 'YOUR PATTERN' -- ./bench.sh
```

Usual causes: the number goes to stderr rather than stdout; the pattern has
zero or several capturing groups; or the workload prints something different
under a frequency cap. See [metric-regex.md](metric-regex.md).

Use `--penalty none` on the smoke test so this aborts immediately instead of
being absorbed.

## Every trial times out

`--application-timeout` defaults to **300 seconds**. Reduced frequency makes
trials slower than the baseline, so a workload that normally takes four minutes
will overrun intermittently — and a timeout is scored as a failure, not a slow
success.

```bash
./scripts/geopm-check-workload.sh --runs 3 -- ./bench.sh    # recommends a timeout
```

Three times the observed maximum is a reasonable setting.

**A timed-out trial's launched process is not guaranteed to be terminated.**
Observed directly: after a trial was reported as `Application evaluation timed
out after N seconds` (with `--penalty none`, which aborts the run), the
launched workload binary was still running and consuming CPU minutes later.
Check for and kill orphans before retrying:

```bash
ps aux | grep '<your launch command>'
```

An orphaned workload process can also keep its enclosing `geopmsession`
context alive, which is one way a write-mode lock ends up looking stuck even
though the trial that opened it already reported failure — see the next
entry and [the install skill's troubleshooting page](../../geopm-install/references/troubleshooting.md#write-access-rejected-by-another-session-even-right-after-installing).

## Write access rejected by another session

```
<geopm> Runtime error: SDBus: ... The PID <new> requested write access, but
the geopm service already has write mode client with PID or SID of <old>
```

Not an access-list or control-name problem, even though a control name appears
in the message just before it. GEOPM's write-mode lock is tied to the
*session leader* of whichever process first opened it, and stays held for as
long as that session leader is alive — including across a `geopmd` restart.
Most often caused by running one campaign's commands from more than one
terminal, or by a timed-out trial's orphaned process (previous entry) keeping
a session open. Full diagnosis and fix:
[geopm-install troubleshooting](../../geopm-install/references/troubleshooting.md#write-access-rejected-by-another-session-even-right-after-installing).
Rule of thumb: issue every command for one campaign from the same
terminal/session; never switch terminals or move to a background/async
execution context mid-campaign.

## No dimension is sweepable

```
CONTROL       DOMAIN    UNITS   MIN     MAX     STEP
gpu-freq      n/a       Hz      n/a     n/a     n/a
board-power   n/a       W       200     6000    1
```

A dimension is usable only when its **domain** is not `n/a`. Bounds alone are
not enough — unavailable power dimensions print hardcoded defaults next to an
`n/a` domain, as `board-power` does above.

```bash
./scripts/geopm-probe-controls.sh --venv ~/geopm-venv
```

It distinguishes "the platform does not implement this" from "the service
supports it but you were not granted it", which look identical in
`--list-controls`.

## Permission denied writing a swept control

The readiness gate should have caught this. If a campaign fails partway with a
control write error, the access list changed or the campaign is sweeping a
dimension the gate did not check.

```bash
geopmaccess --controls | grep -x CPU_FREQUENCY_MAX_CONTROL
```

Return to the `geopm-install` skill.

## Results are indistinguishable from noise

The most common outcome of a badly designed campaign, and the easiest to
mistake for success.

Symptoms:

- Scores at different settings differ by less than repeated runs at one
  setting.
- Physically implausible ordering, such as a CPU-bound workload getting slower
  at higher frequency.
- The optimizer re-samples points it already tried.

Measure the floor:

```bash
./scripts/geopm-check-workload.sh --regex '...' --runs 3 -- ./bench.sh
```

Above about 5% spread, only large effects are detectable. Lengthen the
workload, pin threads, quiesce the machine, or accept that the answer will be
coarse. More trials do not fix noise — they only average it more slowly.

## The optimizer keeps re-evaluating the same point

```
UserWarning: The objective has been evaluated at point [np.int64(25)] before,
using random point [np.int64(5)]
```

Emitted by `scikit-optimize`, not GEOPM. The model wanted a point it had
already tried, so a random one was substituted. Occasional occurrences are
normal. Frequent ones mean the grid is small relative to the trial budget, or
the response is so noisy the model cannot make progress. Coarsen the grid with
a larger step, or reduce the trial count.

## The best result is at a boundary

The optimum may lie outside the search range. Widen it:

```bash
--sweep cpu-freq@board=800MHz:3.7GHz
```

Note that `cpu-power`'s auto-detected maximum is the *default* power limit, not
a hardware ceiling, so the sweep never explores above nominal unless you say
so explicitly.

## The best result equals the platform default

Either the default really is optimal — common for CPU-bound work at maximum
frequency — or the controls had no effect. Distinguish the two by checking that
different coordinates produced different scores in the log. If every trial
scored roughly the same, the workload is insensitive to what you swept, or the
writes are not reaching the hardware.

```bash
geopmread CPU_FREQUENCY_MAX_CONTROL package 0    # inside a session
```

## A constraint is never satisfied

Constraints are soft. If no trial is feasible, the least-infeasible one is
returned, so **the campaign reports a winner regardless**. It never errors on
an impossible constraint.

Verify the winner actually satisfied it:

```bash
printf 'TIME board 0\nCPU_POWER board 0\n' > sig.conf
geopmsession -i sig.conf -r report.yaml -p 0.05 \
    --control-config best.conf -- ./bench.sh
```

If nothing can satisfy the bound, relax it or widen the sweep so a feasible
region exists.

## The campaign appears to hang

At the default `--verbosity 1` nothing is printed until completion. Use
`--verbosity 2` to see per-trial progress.

If it is genuinely stuck, the workload is probably waiting on input. `geopmopt`
launches it non-interactively, so anything prompting will block until the
timeout.

## geopmsession fails when applying a result

```
Error: Unable to convert values into a read request: ...
```

`geopmsession` reads signal requests from stdin even when you only want it to
apply controls, so it consumed the surrounding script. Give it an explicit
request file:

```bash
printf 'TIME board 0\n' > sig.conf
geopmsession -i sig.conf --control-config best.conf -o /dev/null -- ./bench.sh
```

## The applied setting differs from the requested one

Requesting 3.5 GHz and reading back 3.515 GHz is normal: hardware snaps to the
nearest supported step. Always read back what was actually applied rather than
assuming the requested value took effect.

## The settings did not persist

Correct and by design. Session save and restore reverts every control when
`geopmopt` exits. To use a configuration, hold a session open for the duration
of the work — see [interpreting-results.md](interpreting-results.md).

## geopmopt's command line does not match the documentation

An older development snapshot has a different interface: it requires
`--metric-regex` and uses per-control flags such as `--cpu-frequency` instead
of `--sweep DIM`.

```bash
geopmopt --help | grep -c -- --sweep    # 0 means too old
```

Reinstall from current `dev`; see the `geopm-install` skill.

## --list-metrics accepts an invalid --sweep

Not a bug you can work around, but worth knowing: `--list-metrics` returns
before sweep specifications are parsed, so it validates metrics only. A
malformed `--sweep` still exits non-zero on a real invocation, before the
workload runs.

## Collecting information

```bash
geopmread --version
geopmopt --list-controls
geopmopt --list-metrics
./scripts/geopm-probe-controls.sh --venv ~/geopm-venv
./scripts/geopm-check-workload.sh --regex '...' --runs 3 -- ./bench.sh
```

Report issues at https://github.com/geopm/geopm/issues/new/choose with the
GEOPM version, the exact command, and the error text.
