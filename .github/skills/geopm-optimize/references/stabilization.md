# Stabilizing a workload: the levers, and who owns them

Run-to-run variation is the enemy of optimization. The
[sensitivity check](sensitivity.md) fails a dimension when one step of the
control moves the metric by less than the noise, and every lever below exists to
shrink that noise so the signal can be seen.

**The rule that governs this whole file:** the assistant enumerates these
levers and asks which are exercised; it does **not** guess a pinning line or
write a kernel/BIOS/OS setting on the user's behalf. How a workload should be
launched, and how a given machine may be reconfigured, are facts only the user
has. Present the menu, ask the interview questions, and let the user supply the
launch wrapper or apply the machine change. If a lever is not in use, ask
whether the user *can* enable it on this system before assuming it is off the
table.

The levers are grouped by who can safely act:

- **Assistant-actionable** — deterministic and reversible within a `geopmopt`
  or `geopmsession` session (step size, run length, campaign hygiene).
- **Launch-time, user-owned** — how the workload binds to the hardware
  (process and thread affinity, NUMA placement).
- **Machine-wide, user-owned** — OS, kernel-command-line, and BIOS settings
  that affect every workload on the node.

## What GEOPM already does for you

Do not re-implement these; they are defaults in current `geopmopt`:

- **`cpu-freq` sweep caps at `CPU_FREQUENCY_STICKER`** (base/nominal frequency)
  rather than the turbo maximum, so grid points stay at frequencies the
  hardware honours. Sweep above the sticker only when the user explicitly opts
  into the turbo range.
- **The performance governor is forced while sweeping `cpu-freq`**
  (`CPU_FREQUENCY_GOVERNOR_CONTROL=performance`). `CPU_FREQUENCY_MAX_CONTROL`
  alone is only a cap; other governors can still scale below it. The forced
  governor makes a requested frequency stick.
- **Settings are applied inside a session and reverted on exit**, so a campaign
  does not leave the machine altered.

## Assistant-actionable levers

### Coarsen the control step

The quickest fix for a MARGINAL dimension, and it needs nothing from the user.
A larger step makes one grid move clear the noise. The sensitivity script prints
the recommended sweep specification. See remedy 1 in [sensitivity.md](sensitivity.md).

### Lengthen each run

Noise is dominated by start-up and scheduling effects that do not grow with
runtime, so a 4x longer run roughly halves the relative spread. Aim for at least
30 seconds per run. This needs the user to change the iteration count or problem
size, but it is a simple ask.

### Campaign hygiene

Vary `--random-seed` only when you want a genuinely independent repeat; the
default `42` makes campaigns reproducible. Record the noise floor with every
campaign so a later session can tell signal from noise.

## Launch-time levers (ask the user; never guess)

These control how the workload binds to CPUs and memory. **Ask interview
questions 9 and 10; do not emit an affinity line as if it were correct for this
workload.**

### Process affinity — set by the launcher

Restricts which CPUs the process and its children may use. It bounds the
process to a CPU mask but lets threads float *within* the mask. Common
mechanisms the user might already use, or could add:

| Mechanism | Typical use |
|---|---|
| `taskset -c <cpu-list>` | wrap a single process on a fixed core set |
| `numactl --cpunodebind=<node> --membind=<node>` | pin process and its memory to one NUMA node |
| `numactl --physcpubind=<cpu-list>` | pin to specific CPUs |
| `cgroups` / `systemd` `AllowedCPUs` | container or service scoping |
| `srun --cpu-bind=...`, `mpirun --bind-to core` | HPC launchers |

Pin memory as well as CPUs when the workload touches a lot of it: a run that
lands on remote memory is slower for reasons unrelated to the control being
swept.

### Thread affinity — set by the runtime

Pins each thread to a CPU *inside* the process mask. Process pinning without
thread pinning still allows threads to migrate within the mask — a frequent
cause of jitter in OpenMP codes such as a threaded STREAM. The right setting
depends on the runtime:

| Runtime | Controls |
|---|---|
| OpenMP (portable) | `OMP_PROC_BIND` (`close`/`spread`/`master`), `OMP_PLACES` (`cores`/`threads`/`sockets` or an explicit list), `OMP_NUM_THREADS` |
| Intel OpenMP | `KMP_AFFINITY` (e.g. `granularity=fine,compact,1,0`), `KMP_HW_SUBSET` |
| GNU libgomp | `GOMP_CPU_AFFINITY` |
| MPI | rank-to-core binding via the launcher (see above) |
| pthreads/TBB | application-specific `pthread_setaffinity_np`, TBB arena/observer |

The user must state which of these apply and supply the exact values. When they
do, verify that the process mask and the thread placement are consistent (do not
pin threads to CPUs outside the process mask).

## Machine-wide levers (propose to the user; the user applies them)

These affect every workload on the node and often require root, a reboot, or a
BIOS visit. Propose the relevant ones and let the user decide. Sources:
[pyperf's system-tuning guide](https://pyperf.readthedocs.io/en/latest/system.html)
and [LLVM's benchmarking tips](https://llvm.org/docs/Benchmarking.html).

### Frequency and power

- **Performance governor** for all CPUs. For a `cpu-freq` campaign GEOPM forces
  this already; for other campaigns the user may want it set system-wide
  (`cpupower frequency-set -g performance`, or writing `scaling_governor`).
- **Disable Turbo Boost** to remove frequency drift from thermal/core-count
  effects (`intel_pstate/no_turbo`, or the BIOS). Removing turbo trades peak
  performance for reproducibility — the user's call.
- **Fix the uncore frequency** so it does not scale with demand between runs.
- **Pin `scaling_min_freq` to the max** so the core does not idle down mid-run.

### Idle states (C-states)

Deep C-states add wake-up latency that varies between runs. Options, from least
to most aggressive: BIOS C-state limits, `intel_idle.max_cstate=1` /
`processor.max_cstate=1` on the kernel command line, or holding
`/dev/cpu_dma_latency` open with a `0` written to it for the duration of the
run. `idle=poll` is the extreme (and power-hungry) form.

### Scheduler isolation

- **`isolcpus=<cpu-list>`** on the kernel command line removes CPUs from the
  general scheduler so only explicitly-pinned work lands there.
- **`nohz_full=<cpu-list>`** stops the scheduler tick on those CPUs. Note the
  documented caveat: `nohz_full` with the `intel_pstate` driver needs a fixed
  frequency or the frequency becomes unstable.
- **`rcu_nocbs=<cpu-list>`** offloads RCU callbacks off the isolated CPUs.
- **`irqbalance` off** plus pinning IRQs away from the benchmark CPUs
  (`/proc/irq/*/smp_affinity`) keeps interrupts off the hot cores.
- **`cset shield`** / cpuset is a userspace way to reserve cores without a
  reboot.

### Topology

- **Disable SMT / Hyper-Threading** (BIOS, or offline the sibling via
  `/sys/devices/system/cpu/cpuN/online`) when sibling contention adds variance.
- **Respect NUMA topology** when choosing the pinning mask; keep a run within
  one node unless the workload is deliberately cross-node.

### Memory and OS noise

- **Huge pages** (transparent or explicit) can cut page-fault variance for
  memory-heavy codes; test rather than assume.
- **tmpfs for inputs/outputs** removes storage-system variability from timed
  regions.
- **Leave ASLR enabled.** Disabling `randomize_va_space` can *hide* real
  layout-sensitivity and produce a single lucky layout; pyperf explicitly warns
  against turning it off. Static linking is the safer way to remove
  dynamic-loader variation when it is a concern.
- **Drop other tenants.** A shared build, cron job, or background service is
  indistinguishable from a control effect and will move between trials.

### BIOS profile

Many server BIOSes expose a "maximum performance" or "static high" profile that
bundles Turbo, C-state, uncore, and prefetcher choices. Ask the user which
profile the machine is set to and whether they can change it; do not assume.

## How to use this file in a session

1. During the interview (questions 9–10) ask what process and thread pinning is
   in place and what machine settings the user can change.
2. When the sensitivity check reports MARGINAL/NOT READY, apply the
   assistant-actionable levers directly (coarser step, longer run).
3. For the launch-time and machine-wide levers, present the relevant options
   from the tables above, ask the user to supply the exact pinning wrapper or to
   apply the setting, then re-run the sensitivity check to measure the effect.
4. Record which levers were in force alongside the campaign results, so a later
   run can reproduce the conditions.
