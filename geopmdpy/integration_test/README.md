# GPU Activity Agent Effectiveness Integration Test

This directory holds a **live-hardware** integration test for the Python
`geopmdpy.gpu_activity_agent`. Unlike the hermetic unit tests under
`geopmdpy/test` (which mock `pio`/`topo` and never touch hardware), this test
requires:

- a running **GEOPM service** with GPU signals/controls available,
- a **GPU** (primary path: Intel Data Center GPU via LevelZero), and
- an **AI inference workload** (ResNet-50) — by default run from a prebuilt
  Intel container, so the host only needs a container engine and the GPU driver.

It is **opt-in** and is *not* collected by `make check`.

## What it checks

Running the agent at several `--phi` policy values against a steady ResNet-50
inference load, and comparing against a read-only `geopmsession` monitor
baseline:

| Test | phi | Assertion |
|---|---|---|
| `test_phi0_no_performance_harm` | 0.0 | throughput within `GEOPM_GPU_FOM_TOL` of baseline (frequency pinned at F_max) |
| `test_phi05_energy_benefit_vs_monitor` | 0.5 | GPU energy below the monitor baseline |
| `test_phi05_dynamic_frequency` | 0.5 | agent issues > 1 frequency write **and** `GPU_CORE_FREQUENCY_STATUS` varies |
| `test_phi1_energy_saving_extreme` | 1.0 | GPU energy below baseline (performance harm expected, not asserted) |

> **phi semantics:** `phi < 0.5` biases toward performance (at `phi = 0`
> frequency is pinned at F_max — no harm); `phi > 0.5` biases toward energy
> savings (at `phi = 1` frequency is pinned at F_efficient); `phi = 0.5` spans
> the full range and is the only setting that dynamically tracks activity.

## Prerequisites (one-time)

Ensure the host has the Intel GPU kernel driver (`i915`/`xe`) and a container
engine, then pull the pinned image:

```bash
docker pull intel/intel-extension-for-pytorch:2.8.10-xpu
# or: podman pull docker.io/intel/intel-extension-for-pytorch:2.8.10-xpu
```

Optional smoke check that the container can see the GPU:

```bash
docker run --rm --device /dev/dri \
    --group-add "$(getent group render | cut -d: -f3)" \
    intel/intel-extension-for-pytorch:2.8.10-xpu \
    python -c "import torch, intel_extension_for_pytorch; print(torch.xpu.is_available(), torch.xpu.device_count())"
# expect: True <n>
```

## Running

```bash
GEOPM_RUN_GPU_INTEGRATION=1 python -m geopmdpy.integration_test
# or a single module:
GEOPM_RUN_GPU_INTEGRATION=1 \
    python -m unittest geopmdpy.integration_test.TestGPUActivityAgentInference -v
```

Without `GEOPM_RUN_GPU_INTEGRATION=1`, or without a GPU/service/workload, the
test skips cleanly.

## Configuration (environment variables)

Workload / deployment:

| Variable | Default | Meaning |
|---|---|---|
| `GEOPM_GPU_WORKLOAD_NATIVE` | `0` | `1` runs `ipex_resnet50_infer.py` natively instead of in a container (host needs `torch` + `intel_extension_for_pytorch`) |
| `GEOPM_GPU_CONTAINER_ENGINE` | `docker` | Container engine (`docker` or rootless `podman`) |
| `GEOPM_GPU_WORKLOAD_IMAGE` | `intel/intel-extension-for-pytorch:2.8.10-xpu` | Container image (pin a version tag) |
| `GEOPM_GPU_SELINUX_DISABLE` | unset | Non-empty adds `--security-opt label=disable` (rootless podman on SELinux) |

Measurement / tolerances:

| Variable | Default | Meaning |
|---|---|---|
| `GEOPM_GPU_WORKLOAD_SEC` | `30` | Workload timed duration (s) |
| `GEOPM_GPU_BATCH_SIZE` | `64` | Inference batch size |
| `GEOPM_GPU_PERIOD` | `0.02` | Agent/monitor sample period (s) |
| `GEOPM_GPU_FOM_TOL` | `0.05` | Allowed fractional throughput drop at phi=0 |
| `GEOPM_GPU_ENERGY_MARGIN` | `1.0` | `energy(phi) < margin * baseline` (use e.g. `0.98` to require a minimum drop) |
| `GEOPM_GPU_FREQ_STD_MIN_HZ` | `1e7` | Min frequency std-dev (Hz) proving dynamic control |

## How it works

- A read-only `geopmsession` monitor is the measurement instrument for **every**
  run: it launches the workload (`-- run_resnet50.sh ...`), traces `GPU_ENERGY`,
  `GPU_CORE_FREQUENCY_STATUS`, and `GPU_CORE_ACTIVITY` across **all** GPU
  domains, and exits when the workload finishes. The workload prints
  `FOM (images/sec): <n>` to stdout, which the monitor forwards and the harness
  parses.
- For a controlled run, the agent (`python -m geopmdpy.gpu_activity_agent`) runs
  concurrently as the single frequency **writer** while the monitor reads
  (GEOPM permits one writer + many readers). The agent is stopped with `SIGINT`
  after the workload finishes, which reverts its controls and prints the
  `GPU Frequency Requests` summary used by the dynamic-frequency assertion.
- Energy is the sum of per-domain `GPU_ENERGY` deltas over the trace window;
  frequency dynamism is the max per-domain std-dev of `GPU_CORE_FREQUENCY_STATUS`.

## Files

| File | Purpose |
|---|---|
| `TestGPUActivityAgentInference.py` | The test class and orchestration |
| `_util.py` | geopmdpy-native skip guards + trace/FoM parsing helpers |
| `apps/ipex_resnet50_infer.py` | ResNet-50 FP16 inference benchmark (XPU/CUDA) |
| `apps/run_resnet50.sh` | Container/native wrapper the harness invokes |
| `apps/Dockerfile` | Optional bake-in image (zero-mount `docker run`) |
| `__main__.py` | Explicit discovery runner for this directory only |
