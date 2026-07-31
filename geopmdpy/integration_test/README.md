# GPU Activity Agent Effectiveness Integration Test

This directory holds a **live-hardware** integration test for the Python
`geopmdpy.gpu_activity_agent`. Unlike the hermetic unit tests under
`geopmdpy/test` (which mock `pio`/`topo` and never touch hardware), this test
requires:

- a running **GEOPM service** with GPU signals/controls available,
- a **GPU** (primary path: Intel Data Center GPU via LevelZero), and
- a local **SYCL/oneAPI compiler toolchain** (for example `icpx`) or a
  previously built local benchmark binary.  No network access or container
  image is required for the default path.

It is **opt-in** and is *not* collected by `make check`.

## Terminology

The word "driver" appears in two unrelated senses in this directory:

- **kernel driver** — the operating-system GPU driver (`i915` or `xe`) that
  exposes the device; a host prerequisite.
- **workload driver** — a program under `apps/` that generates GPU load and
  prints a single `FOM (<unit>): <n>` line: the default SYCL benchmark
  (`gpu_activity_benchmark`) or the optional PyTorch scripts
  (`ipex_resnet50_infer.py`, `torch_decode_infer.py`).

Unqualified uses of "driver" below refer to the workload driver.

## Why three workloads

The agent sets GPU frequency proportional to compute activity:

```
f_request = f_efficient + (f_max - f_efficient) * (activity / utilization)
```

so it can only save energy or move the frequency **when the GPU stalls**
(activity drops below saturation). A single compute-saturated benchmark keeps
activity pinned near 1.0 and would make the energy/dynamic assertions fail —
not because the agent is broken, but because there is nothing to exploit.
Three profiles are therefore exercised:

| Class | Profile | What the agent can do |
|---|---|---|
| `TestGPUActivityAgentSteadyState` | back-to-back compute-saturated SYCL inference proxy | nothing — **control** for no-harm only |
| `TestGPUActivityAgentServing` | over-provisioned serving proxy with idle gaps (regime 1) | drop frequency during gaps |
| `TestGPUActivityAgentDecode` | batch-1, memory-bound decode proxy (regime 2) | lower frequency of busy-but-idle compute — agent's strongest case |

## What it checks

Each class runs its workload under a read-only `geopmsession` monitor baseline
and at several `--phi` values, then asserts:

**SteadyState (control)** — expectations adjusted for a saturated workload:

| Test | phi | Assertion |
|---|---|---|
| `test_phi0_no_performance_harm` | 0.0 | throughput within `GEOPM_GPU_FOM_TOL` of baseline (pinned F_max) |
| `test_phi05_no_harm_when_saturated` | 0.5 | throughput within tolerance — agent stays at F_max, does no harm |
| `test_phi1_energy_saving_extreme` | 1.0 | GPU energy below baseline (static clamp; perf harm not asserted) |

It intentionally does **not** assert phi=0.5 energy savings or dynamic control,
because a saturated workload gives the agent no stalls to exploit.

**Serving (regime 1)** and **Decode (regime 2)** — the agent's value is asserted:

| Test | phi | Assertion |
|---|---|---|
| `test_phi0_no_performance_harm` | 0.0 | throughput within tolerance of baseline |
| `test_phi05_dynamic_frequency` | 0.5 | agent issues more frequency writes than the initial one-write-per-control-domain pass **and** `GPU_CORE_FREQUENCY_STATUS` varies |
| `test_phi05_energy_benefit_vs_monitor` | 0.5 | GPU energy below the monitor baseline |
| `test_phi1_energy_saving_extreme` | 1.0 | GPU energy below baseline (perf harm not asserted) |

> **phi semantics:** `phi < 0.5` biases toward performance (at `phi = 0`
> frequency is pinned at F_max — no harm); `phi > 0.5` biases toward energy
> savings (at `phi = 1` frequency is pinned at F_efficient); `phi = 0.5` spans
> the full range and is the only setting that dynamically tracks activity.

## Prerequisites (one-time)

Ensure the host has the Intel GPU kernel driver (`i915`/`xe`), Level Zero, and
the oneAPI compiler environment loaded so `icpx` (or `dpcpp`) is on `PATH`.
The default benchmark is built locally on first use:

```bash
cd geopm/geopmdpy/integration_test/apps
make
./build/gpu_activity_benchmark --profile steady --duration 2
# expect: FOM (images/sec): <n>
```

If the compiler is not on `PATH`, set `GEOPM_GPU_BENCH_CXX` to the compiler
path.  The resulting binary is cached under `integration_test/apps/build/` by
default; set `GEOPM_GPU_BENCH_BUILD_DIR` to place it elsewhere.

The PyTorch/container workload drivers are optional alternatives to the default
SYCL benchmark.  The integration tests do not use them by default, and they
require a local image cache or a native PyTorch/IPEX install.

## Running

`integration_test` is a **top-level package that sits beside the installed
`geopmdpy` module, not inside it** (just like the `test/` unit-test package).
Run it from the `geopm/geopmdpy` source directory so the package is importable;
do **not** use a `geopmdpy.integration_test` dotted path (no such submodule
exists):

```bash
cd geopm/geopmdpy
python -m integration_test
# or a single scenario:
python -m unittest \
    integration_test.TestGPUActivityAgentInference.TestGPUActivityAgentServing -v
```

Without a GPU/service/workload, the test skips cleanly.

## Configuration (environment variables)

Workload / deployment:

| Variable | Default | Meaning |
|---|---|---|
| `GEOPM_GPU_WORKLOAD_BACKEND` | `local` | `local` uses the self-built SYCL benchmark (default). Other values fall through to the optional Python/container path. |
| `GEOPM_GPU_BENCH_CXX` | auto (`icpx`, `dpcpp`, `clang++`) | SYCL compiler used by the `apps/Makefile` |
| `GEOPM_GPU_BENCH_BUILD_DIR` | `integration_test/apps/build` | Build/cache directory for the local benchmark binary |
| `GEOPM_GPU_WORKLOAD_NATIVE` | `0` | Optional Python-driver path only: `1` runs Python drivers natively instead of in a container (host needs `torch` + `intel_extension_for_pytorch`) |
| `GEOPM_GPU_CONTAINER_ENGINE` | `docker` | Optional Python-driver path only: container engine (`docker` or rootless `podman`) |
| `GEOPM_GPU_WORKLOAD_IMAGE` | `intel/intel-extension-for-pytorch:2.8.10-xpu` | Optional Python-driver path only: container image (pin a version tag) |
| `GEOPM_GPU_SELINUX_DISABLE` | unset | Optional Python-driver path only: non-empty adds `--security-opt label=disable` |

Measurement / tolerances:

| Variable | Default | Meaning |
|---|---|---|
| `GEOPM_GPU_WORKLOAD_SEC` | `30` | Workload timed duration (s) |
| `GEOPM_GPU_BATCH_SIZE` | `64` | Image-profile FoM scaling factor |
| `GEOPM_GPU_DUTY_CYCLE` | `0.5` | Serving-mode GPU-active fraction (idle gap = `1 - duty`) |
| `GEOPM_GPU_DECODE_HIDDEN` | `8192` | Decode model hidden dimension (larger = more memory-bound) |
| `GEOPM_GPU_DECODE_LAYERS` | `8` | Decode projection-layer count |
| `GEOPM_GPU_BENCH_ELEMENTS` | `1048576` | Steady/serving local benchmark vector length |
| `GEOPM_GPU_COMPUTE_INTENSITY` | `2048` | FMAs per element for steady/serving profiles |
| `GEOPM_GPU_DECODE_INTENSITY` | `1` | Memory passes per decode token |
| `GEOPM_GPU_PERIOD` | `0.02` | Agent/monitor sample period (s) |
| `GEOPM_GPU_FOM_TOL` | `0.05` | Allowed fractional throughput drop at phi=0 |
| `GEOPM_GPU_ENERGY_MARGIN` | `1.0` | `energy(phi) < margin * baseline` (use e.g. `0.98` to require a minimum drop) |
| `GEOPM_GPU_FREQ_STD_MIN_HZ` | `1e7` | Min frequency std-dev (Hz) proving dynamic control |

## How it works

- A read-only `geopmsession` monitor is the measurement instrument for **every**
  run: it launches the workload (`-- run_workload.sh <driver> ...`), traces
  `GPU_ENERGY`, `GPU_CORE_FREQUENCY_STATUS`, and `GPU_CORE_ACTIVITY` across
  **all** GPU domains, and exits when the workload finishes. For the default
  path, `run_workload.sh gpu_activity_benchmark ...` builds the local SYCL
  benchmark on demand, runs it on a Level Zero GPU, and prints `FOM (<unit>):
  <n>` to stdout for the harness to parse.
- For a controlled run, the agent (`python -m geopmdpy.gpu_activity_agent`) runs
  concurrently as the single frequency **writer** while the monitor reads
  (GEOPM permits one writer + many readers). The agent is stopped with `SIGINT`
  after the workload finishes, which reverts its controls and prints the
  `GPU Frequency Requests` and `Agent Domain` summary used by the
  dynamic-frequency assertion.  The assertion requires more writes than the
  number of controlled domains, because the agent always performs one initial
  write per GPU/GPU-chip control domain before any dynamic re-tuning occurs.
- Energy is the sum of per-domain `GPU_ENERGY` deltas over the trace window;
  frequency dynamism is the max per-domain std-dev of `GPU_CORE_FREQUENCY_STATUS`.

## Files

| File | Purpose |
|---|---|
| `TestGPUActivityAgentInference.py` | Shared harness + the three scenario test classes |
| `_util.py` | geopmdpy-native skip guards + trace/FoM parsing helpers |
| `apps/gpu_activity_benchmark.cpp` | Local SYCL/oneAPI benchmark with `steady`, `serving`, and `decode` profiles |
| `apps/Makefile` | Builds/caches the local benchmark binary |
| `apps/ipex_resnet50_infer.py` | ResNet-50 FP16 benchmark: `steady` and `serving` modes |
| `apps/torch_decode_infer.py` | Batch-1, memory-bound autoregressive decode benchmark |
| `apps/run_workload.sh` | Container/native wrapper the harness invokes (driver = arg 1) |
| `apps/Dockerfile` | Optional bake-in image (both drivers, no volume mount) |
| `__main__.py` | Explicit discovery runner for this directory only |
