# Sweep dimensions

What `geopmopt` can search, and how to say it. Transcribed from
`geopmdpy/geopmdpy/grid.py` and verified against a live system.

## Always check the target first

Bounds are discovered from the platform, so the same command behaves
differently on different machines:

```bash
geopmopt --list-controls
```

Real output from a two-socket Skylake Xeon:

```
CONTROL       DOMAIN    UNITS   MIN           MAX           STEP
cpu-freq      cpu       Hz      1e+09         3.7e+09       1e+08
uncore-freq   package   Hz      1e+09         2.4e+09       1e+08
cpu-power     package   W       73            150           1
gpu-freq      n/a       Hz      n/a           n/a           n/a
gpu-power     n/a       W       200           n/a           1
board-power   n/a       W       200           6000          1
prefetch      cpu       level   0             4             1
```

**A dimension is usable only when its DOMAIN is not `n/a`.** Bounds alone are
not sufficient: `board-power` above shows a plausible-looking `200 … 6000` range
while being entirely unavailable, because those numbers are hardcoded defaults
that are printed whether or not the control exists. On this host only
`cpu-freq`, `uncore-freq`, `cpu-power`, and `prefetch` can actually be swept.

Never assume a dimension exists. Run `--list-controls` on the machine that will
run the campaign.

## Grammar

```
--sweep CONTROL[@DOMAIN][=MIN:MAX:STEP]
```

Repeatable; each occurrence adds a dimension to the search space.

| Part | Default | Notes |
|---|---|---|
| `CONTROL` | required | One of the names below |
| `@DOMAIN` | the control's native domain | `board`, `package`, `cpu`, ... |
| `=MIN:MAX:STEP` | auto-detected from the platform | Slice semantics, see below |

### Dimension names

Each has a short alias and a longer spelling; both work.

| Alias | Long form | Underlying control | Units |
|---|---|---|---|
| `cpu-freq` | `cpu-frequency` | `CPU_FREQUENCY_MAX_CONTROL` | frequency |
| `uncore-freq` | `cpu-uncore-frequency` | `CPU_UNCORE_FREQUENCY_MAX_CONTROL` | frequency |
| `cpu-power` | — | `POWERCAP::CPU_POWER_LIMIT` | power |
| `gpu-freq` | `gpu-frequency` | `GPU_CORE_FREQUENCY_MAX_CONTROL` | frequency |
| `gpu-power` | — | `GPU_POWER_LIMIT_CONTROL` | power |
| `board-power` | — | `BOARD_POWER_LIMIT_CONTROL` | power |
| `prefetch` | `prefetch-disable` | MSR prefetcher-disable bits | level |

An unknown name is rejected immediately:

```
Error: unknown control 'not-a-dim'; see --list-controls
```

### Units

Suffixes are case-insensitive. A bare number is taken to be in the raw unit
(Hz, W), so `3000000000` and `3GHz` are the same.

| Category | Accepted suffixes |
|---|---|
| frequency | `Hz`, `kHz`, `MHz`, `GHz` |
| power | `W`, `kW` |
| level | none allowed |

```
Error: unrecognized unit 'Watts'; use one of Hz, kHz, MHz, GHz
Error: unit 'Hz' is not allowed for a level control
```

### Range overrides use slice semantics

Any field may be omitted, but **at least one `:` must be present** — a lone
value is rejected as ambiguous rather than guessed at.

| Spec | Meaning |
|---|---|
| `cpu-freq=1.2GHz:3GHz:100MHz` | all three |
| `cpu-freq=1.2GHz:3GHz` | bounds only, auto step |
| `cpu-freq=::200MHz` | step only, auto bounds |
| `cpu-freq=1.2GHz::` | minimum only |
| `cpu-freq=2GHz` | **rejected** |

```
Error: invalid range '2GHz'; expected MIN:MAX[:STEP] with ':' separators
Error: min exceeds max in '3GHz:1GHz'
Error: step in '1GHz:3GHz:0' must be positive
```

## Where the bounds come from

Auto-detection reads platform signals. If a signal is unavailable — commonly
because it is not in the user's access list — that field renders `n/a` and the
dimension becomes unusable.

| Dimension | Min | Max | Step |
|---|---|---|---|
| `cpu-freq` | `CPU_FREQUENCY_MIN_AVAIL` | `CPU_FREQUENCY_MAX_AVAIL` | `CPU_FREQUENCY_STEP` |
| `uncore-freq` | `CPU_FREQUENCY_MIN_AVAIL` | current `CPU_UNCORE_FREQUENCY_MAX_CONTROL` | `CPU_FREQUENCY_STEP` |
| `cpu-power` | `CPU_POWER_MIN_AVAIL` | `CPU_POWER_LIMIT_DEFAULT` | 1 W |
| `gpu-power` | `LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL` | `LEVELZERO::GPU_POWER_LIMIT_DEFAULT` | 1 W |
| `board-power` | 200 W (hardcoded) | 6000 W (hardcoded) | 1 W |
| `prefetch` | 0 | 4 | 1 |

Two consequences worth knowing:

- `cpu-power`'s maximum is the **default power limit**, not a hardware
  ceiling, so the sweep explores at or below nominal rather than above it.
- `uncore-freq`'s maximum is read from the control's *current* value, so a
  previously lowered uncore limit narrows the search space. Check with
  `geopmread CPU_UNCORE_FREQUENCY_MAX_CONTROL package 0` if the range looks
  small.

If a dimension shows `n/a` but the hardware exists, suspect the access list
before the hardware: see the bounds-signals table in
[access-lists.md](../../geopm-install/references/access-lists.md).

## Domains

Omitting `@DOMAIN` uses the control's native domain, which is what
`--list-controls` prints. Choosing a coarser domain sets every instance
together and keeps the search space small:

```bash
geopmopt --sweep cpu-freq@board      # one knob for the whole machine
geopmopt --sweep cpu-freq@package    # one knob per socket
geopmopt --sweep cpu-freq            # native: per-CPU on this host
```

Prefer `@board` unless the workload is deliberately asymmetric across sockets.
A per-package sweep on a two-socket machine doubles the dimensionality for a
result that is usually the same on both sockets.

An empty domain is rejected:

```
Error: empty domain in --sweep 'cpu-freq@'
```

## The prefetch dimension

`prefetch` is not a frequency or a power cap: it is an integer level from 0 to
4 that progressively disables hardware prefetchers, in this order:

1. `DCU_HW_PREFETCHER_DISABLE`
2. `L2_HW_PREFETCHER_DISABLE`
3. `DCU_IP_PREFETCHER_DISABLE`
4. `L2_ADJACENT_PREFETCHER_DISABLE`

Level 0 leaves all enabled; level 4 disables all four. Worth including for
memory-bound workloads where prefetching wastes bandwidth, and worth omitting
otherwise, since it rarely helps compute-bound codes and costs trials.

It is not documented in the `geopmopt` man page; the behavior above is read
from `grid.py`.

## The uncore dimension pins rather than caps

Sweeping `uncore-freq` writes **both** the maximum and the minimum control to
the same value, fixing the uncore at one frequency:

```
CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 1300000000.0
CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 1300000000.0
```

This follows from `get_config()` in `grid.py`, which mirrors any `*_MAX_*`
control onto the matching `*_MIN_*` control when one exists.
`CPU_FREQUENCY_MIN_CONTROL` is explicitly excluded, so `cpu-freq` caps normally
and only `uncore-freq` and `gpu-freq` pin.

Two consequences:

- **The access list needs both controls.** Granting only
  `CPU_UNCORE_FREQUENCY_MAX_CONTROL` lets the readiness gate pass and then fails
  the campaign partway with a permission error. Request
  `CPU_UNCORE_FREQUENCY_MIN_CONTROL` alongside it, and
  `GPU_CORE_FREQUENCY_MIN_CONTROL` for `gpu-freq`.
- **Pinning can be worse than the default.** Normally the uncore scales
  dynamically with demand. Fixing it removes that adaptation, so a swept
  configuration can lose to doing nothing even at a frequency that looked good
  during the campaign. Verify the recommendation against the baseline; see
  [interpreting-results.md](interpreting-results.md) for a measured case where
  the "optimized" setting was 11% slower.

## Frequencies above the sticker are not guaranteed

`CPU_FREQUENCY_MAX_AVAIL` reports the turbo maximum, but **above
`CPU_FREQUENCY_STICKER` a requested frequency is only an upper bound**. What the
hardware actually delivers is set by power, thermal, and core-count limits, so
several distinct requests can produce the same operating point.

Measured on a Xeon Gold 6148 (sticker 2.4 GHz, max avail 3.7 GHz) under a
40-core load:

| Requested | Achieved |
|---|---|
| 3.7 GHz | 2.72 GHz |
| 3.3 GHz | 2.73 GHz |
| 2.4 GHz | 2.46 GHz |
| 2.0 GHz | 2.23 GHz |

The top two requests are the same operating point. On the default 100 MHz grid
that makes roughly ten of the twenty-eight `cpu-freq` points indistinguishable,
and a search allowed into that region will report a "best" frequency from it
that does not reproduce.

Check the landmarks on your target, then cap the sweep:

```bash
geopmread CPU_FREQUENCY_STICKER package 0
--sweep cpu-freq@board=1e+09:2400000000:1e+08
```

The headroom depends on load: the same machine running 20 cores on one socket
had 15% turbo headroom where 40 cores across two sockets had 9%. Measure rather
than assume, with
[geopm-sensitivity.sh](../scripts/geopm-sensitivity.sh), which reports achieved
versus requested frequency and warns when the dead zone is large.

## Choosing dimensions

Each added dimension multiplies the space the optimizer must explore, so the
trial budget has to grow with it — see
[campaign-design.md](campaign-design.md).

Start with:

| Workload | First choice | Then |
|---|---|---|
| CPU-bound | `cpu-freq@board` | add `uncore-freq@board` |
| Memory-bound | `uncore-freq@board` | add `cpu-freq@board`, consider `prefetch` |
| Power-limited | `cpu-power@board` | add `cpu-freq@board` |
| GPU-bound | `gpu-freq@board` | add `gpu-power@board` |

`cpu-freq` and `cpu-power` interact: both ultimately limit the same silicon, so
sweeping both often produces a flat region rather than a sharp optimum. Prefer
one unless you specifically want to study the interaction.

Let [geopm-probe-controls.sh](../scripts/geopm-probe-controls.sh) recommend a
starting set based on what the target actually supports.

## Validation happens before anything runs

Every specification is parsed and checked before the workload is launched, so a
malformed sweep costs no trials:

```console
$ geopmopt --sweep cpu-freq=3GHz:1GHz -- ./workload.sh
Error: min exceeds max in '3GHz:1GHz'
$ echo $?
1
```

Note that `--list-metrics` returns before sweeps are parsed, so it cannot be
used to check sweep syntax. Use a real invocation, or
[geopm-probe-controls.sh](../scripts/geopm-probe-controls.sh).
