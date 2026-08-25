# GEOPM — Agent Navigation Guide

Orientation for AI agents working in the GEOPM (Global Extensible Open Power
Manager) repository. Read this before searching the tree; it tells you where
things live, which components matter most, and how to build, test, and verify
changes.

## What GEOPM is

GEOPM provides **fine-grained, low-latency, access-controlled reads and writes
of platform power/energy/frequency metrics and control knobs on Linux**. The
distinguishing feature is *safe, session-scoped hardware control for
unprivileged users*: a user may change a hardware setting, and the **GEOPM
Access Service** (`geopmd`) automatically restores the prior value when the
user's session ends.

Two major components:

| Component | Purpose | C/C++ | Python |
|---|---|---|---|
| **GEOPM Access Service** | Privileged daemon exposing signals/controls with per-user access control | [libgeopmd](libgeopmd) | [geopmdpy](geopmdpy) |
| **GEOPM Runtime Service** | Unprivileged HPC control framework (agents, MPI profiling) | [libgeopm](libgeopm) | [geopmpy](geopmpy) |

**Focus your attention on the Access Service side** (`libgeopmd` + `geopmdpy`).
That is where the primary supported use cases live. See
[Runtime Service transition](#runtime-service-transition) for how to handle the
HPC side.

## Primary use cases

Most user requests map to one of these `geopmdpy` command line tools. Prefer
these over library-level work unless the task explicitly requires C/C++ changes.

| Tool | Implementation | Man page | Use it for |
|---|---|---|---|
| `geopmread` | [geopmdpy/geopmdpy/read.py](geopmdpy/geopmdpy/read.py) | [geopmread.1.rst](docs/source/geopmread.1.rst) | One-shot read of a signal at a topology domain; discover available signals |
| `geopmwrite` | [geopmdpy/geopmdpy/write.py](geopmdpy/geopmdpy/write.py) | [geopmwrite.1.rst](docs/source/geopmwrite.1.rst) | Write a control (power cap, frequency limit); discover available controls |
| `geopmsession` | [geopmdpy/geopmdpy/session.py](geopmdpy/geopmdpy/session.py) | [geopmsession.1.rst](docs/source/geopmsession.1.rst) | Time-series traces, YAML/CSV summary reports, control application for the duration of a run, daemonized collection, Python "agents" |
| `geopmopt` | [geopmdpy/geopmdpy/optimizer.py](geopmdpy/geopmdpy/optimizer.py) | [geopmopt.1.rst](docs/source/geopmopt.1.rst) | Bayesian optimization of control settings against an application objective |
| `geopmgrid` | [geopmdpy/geopmdpy/grid.py](geopmdpy/geopmdpy/grid.py) | [geopmgrid.1.rst](docs/source/geopmgrid.1.rst) | Exhaustive parameter sweeps; defines the control grid `geopmopt` searches |
| `geopmaccess` | [geopmdpy/geopmdpy/access.py](geopmdpy/geopmdpy/access.py) | [geopmaccess.1.rst](docs/source/geopmaccess.1.rst) | Administrator management of per-user/per-group signal and control allow lists |
| `geopmexporter` | [geopmdpy/geopmdpy/exporter.py](geopmdpy/geopmdpy/exporter.py) | [geopmexporter.1.rst](docs/source/geopmexporter.1.rst) | Prometheus metric export |
| `geopmd` | [geopmdpy/geopmdpy/\_\_main\_\_.py](geopmdpy/geopmdpy/__main__.py) | — | The Access Service daemon itself |

Console-script entry points are declared in
[geopmdpy/setup.cfg](geopmdpy/setup.cfg) — check there first when asked "where
does command X live?".

### Representative commands

```bash
geopmread CPU_POWER board 0                       # total board CPU power, Watts
geopmread --info-all                              # every signal, with description
geopmwrite CPU_FREQUENCY_MAX_CONTROL board 0 3.0e9

# 10 s trace at 1 Hz of time and per-package core frequency
printf 'TIME board 0\nCPU_FREQUENCY_STATUS package 0\n' | geopmsession -p 1.0 -t 10.0

# Trace + YAML summary report scoped to an application run
printf 'TIME board 0\nCPU_ENERGY package *\n' | \
    geopmsession -o trace.csv -r report.yaml -p 0.01 -- ./workload.sh

# Search CPU and uncore frequency for the best figure of merit
geopmopt --sweep cpu-freq@board --sweep uncore-freq@board \
         --metric-regex 'GFLOPS: ([0-9.]+)' --trials 30 -- ./bench.sh
```

## Core concepts

Understanding these four ideas is enough to reason about most of the codebase.

- **Signal** — a readable value (`CPU_POWER`, `CPU_ENERGY`,
  `CPU_FREQUENCY_STATUS`, `TIME`, `GPU_POWER`, ...).
- **Control** — a writable setting (`CPU_POWER_LIMIT_CONTROL`,
  `CPU_FREQUENCY_MAX_CONTROL`, ...).
- **Domain** — the topology scope of a signal/control: `board`, `package`,
  `core`, `cpu`, `memory`, `gpu`, `nic`, etc. Every read/write names a domain
  type *and* a domain index. `*` means "native domain" (2nd field) or "all
  indices" (3rd field). See [geopm_topo.3.rst](docs/source/geopm_topo.3.rst).
- **IOGroup** — the plugin abstraction that supplies signals/controls to
  `PlatformIO`. Every hardware backend (MSR, sysfs, SST, NVML, DCGM,
  LevelZero, ...) is an `IOGroup`. See [geopm_pio.7.rst](docs/source/geopm_pio.7.rst)
  and the per-backend `docs/source/geopm_pio_*.7.rst` pages.

Signal and control *names* are the stable public API; treat renames as breaking
changes.

## Safety and the access model

Hardware writes are mediated — do not reason about them as raw MSR pokes.

- **`geopmd` is the safety mechanism.** Unprivileged users reach controls only
  through the Access Service, never directly.
- **Session save/restore**: when a client session first writes a control,
  `geopmd` snapshots the prior value and restores it when the session closes
  (shell exit, process death, timeout). Relevant code:
  `SaveControl` in [libgeopmd/src](libgeopmd/src) and the session lifecycle in
  [geopmdpy/geopmdpy/service.py](geopmdpy/geopmdpy/service.py) and
  [geopmdpy/geopmdpy/system_files.py](geopmdpy/geopmdpy/system_files.py).
- **Access lists**: an administrator uses `geopmaccess` to grant specific
  signals/controls to users or groups. A `geopmwrite` that fails with a
  permission error is usually a missing allow-list entry, not a bug.
- When suggesting commands that mutate hardware state, say so explicitly and
  note that the change reverts at session end. See
  [security.rst](docs/source/security.rst) and [admin.rst](docs/source/admin.rst).

## Repository map

| Path | Contents |
|---|---|
| [libgeopmd](libgeopmd) | C/C++ Access Service implementation — `PlatformIO`, `PlatformTopo`, all `IOGroup` backends, batch server, `geopmbatch` |
| [geopmdpy](geopmdpy) | Python bindings (CFFI) + the `geopmd` daemon + all primary CLI tools |
| [geopmdgo](geopmdgo) | Go bindings to `libgeopmd` (pio, topo, hash) |
| [geopmdrs](geopmdrs) | Rust gRPC-over-UDS proxy used for container support |
| [docs](docs) | Sphinx sources for man pages and the website; JSON schemas; shell completions |
| [libgeopm](libgeopm) | C/C++ Runtime Service (agents, MPI profiling) — see transition note below |
| [geopmpy](geopmpy) | Python bindings for the Runtime Service (`geopmlaunch`) — see transition note |
| [integration](integration) | Integration tests, application benchmarks, experiment automation |
| [release](release) | Distro packaging specs (deb/rpm) |
| [.github](.github) | CI workflows, issue/PR templates, `include_guards.sh` |

## Source code organization

### libgeopmd — C/C++ Access Service

```
libgeopmd/
  include/            installed public headers
    geopm_pio.h, geopm_topo.h, geopm_access.h, geopm_error.h, ...
    geopm/            installed C++ headers: IOGroup.hpp, PlatformIO.hpp,
                      PlatformTopo.hpp, SaveControl.hpp, Agg.hpp, Helper.hpp
  src/                implementation + non-installed headers (~120 files)
  json_data/          MSR and sysfs signal/control definitions (data, not code)
  test/               GoogleTest unit tests (~60 files) plus Mock*.hpp doubles
  fuzz_test/          fuzzing corpus and automation
  contrib/            vendored third-party code (json11)
```

Key groupings inside `src/`:

- **Aggregation layer**: `PlatformIO` / `PlatformIOImp` merges all IOGroups into
  a single signal/control namespace; `PlatformTopo` / `PlatformTopoImp`
  discovers the domain hierarchy.
- **IOGroup backends**: `MSRIOGroup` (with `MSRIO`, `MSRFieldSignal`,
  `MSRFieldControl`), `SysfsIOGroup` (with `CpufreqSysfsDriver`,
  `PowercapSysfsDriver`, `DrmSysfsDriver`), `SSTIOGroup`, `LevelZeroIOGroup`,
  `NVMLIOGroup`, `DCGMIOGroup`, `CNLIOGroup`, `CpuinfoIOGroup`, `TimeIOGroup`,
  `ConstConfigIOGroup`, `ServiceIOGroup`.
- **Derived signals**: `DerivativeSignal`, `DifferenceSignal`, `RatioSignal`,
  `MultiplicationSignal`, `CombinedSignal` / `CombinedControl`.
- **Batch path**: `BatchServer`, `BatchClient`, `BatchStatus`, `SharedMemory`,
  and `IOUring` (with `IOUringFallback`).
- **Service plumbing**: `SDBus` / `SDBusMessage`, `GRPCServiceProxy`,
  `SaveControl`, `SecurePath`, `StatsCollector`.

**Data-driven definitions**: most MSR and sysfs signals are *not* hardcoded.
They are declared in [libgeopmd/json_data](libgeopmd/json_data) —
`msr_data_*.json` per microarchitecture (arch, hsx, knl, skx, spr, ...) and
`sysfs_attributes_*.json` per driver. Adding a signal for existing hardware is
often a JSON edit plus a doc update, not a code change.

**Adding an IOGroup**: subclass `IOGroup`, implement `signal_names()`,
`control_names()`, `signal_domain_type()`, `push_signal()`, `read_batch()`,
`sample()`, `read_signal()`, and the control equivalents; register with the
plugin factory; add sources to [libgeopmd/Makefile.am](libgeopmd/Makefile.am);
add a `docs/source/geopm_pio_<name>.7.rst` page.

### geopmdpy — Python bindings, daemon, and CLI tools

```
geopmdpy/
  geopmdpy/
    pio.py, topo.py, hash.py, error.py, shmem.py   thin bindings over libgeopmd
    gffi.py                                        CFFI handle (_libgeopmd_py_cffi)
    read.py, write.py, session.py, access.py       CLI tools
    grid.py, optimizer.py, exporter.py             CLI tools
    monitor_agent.py, gpu_activity_agent.py,
      heatmap_agent.py                             geopmsession Agent plugins
    service.py, grpc_service.py, grpc_client.py,
      dbus_xml.py, system_files.py                 geopmd daemon internals
    stats.py, metrics.py, loop.py, schemas.py,
      restorable_file_writer.py                    supporting modules
  test/               unit tests, files named Test*.py
  integration_test/   tests requiring a live geopmd and real hardware
  debian/             packaging
```

The CFFI wrapper is generated at install time by
[geopmdpy/build_libgeopmd_wrapper.py](geopmdpy/build_libgeopmd_wrapper.py) from
the `libgeopmd` public headers. If a Python binding is missing, check whether
the corresponding C header is listed there.

Optional dependency groups are declared in
[geopmdpy/setup.cfg](geopmdpy/setup.cfg): `optimize` (scikit-optimize, needed by
`geopmopt`), `stats`, `heatmap`, `grpc`, `dbus_xml`.

### geopmsession agents

`geopmsession` supports pluggable Python agents that provide a signal
configuration, per-iteration logic, and derived output columns. The `Agent` base
class is defined in [geopmdpy/geopmdpy/session.py](geopmdpy/geopmdpy/session.py)
(`help`, `update_parser`, `update_args`, `signal_config_override`, `run_begin`,
`update_loop`, `run_end`, `header_names`, `trace_out`).

Existing agents, invoked as modules:

```bash
python3 -m geopmdpy.monitor_agent --hi-res
python3 -m geopmdpy.gpu_activity_agent
python3 -m geopmdpy.heatmap_agent -t 30 --heatmap-out heatmap.png
```

**This is the preferred extension point for new control/monitoring logic.**

### geopmdgo and geopmdrs

- [geopmdgo](geopmdgo): cgo bindings (`pio.go`, `topo.go`, `hash.go`,
  `error.go`) plus examples; built with `make`, tested with `./test.sh`.
- [geopmdrs](geopmdrs): Rust gRPC server that proxies between a
  restricted-permission UDS watched by `geopmd` and an open-permission UDS used
  by clients, enabling container access. Service contract lives in
  [geopm_service.proto](geopm_service.proto).

## Runtime Service transition

[libgeopm](libgeopm) and [geopmpy](geopmpy) implement the HPC Runtime Service:
C++ `Agent` plugins (`MonitorAgent`, `PowerBalancerAgent`,
`PowerGovernorAgent`, `FrequencyMapAgent`, `CPUActivityAgent`,
`GPUActivityAgent`, `FFNetAgent`), MPI/OpenMP profiling (`Profile`, `Tracer`,
`Reporter`, `TreeComm`), and the `geopmlaunch` job wrapper.

**These are not deprecated, but their functionality is being transitioned to
`geopmsession`-based Python agents** (the GPU activity agent already has a
`geopmdpy` implementation). Guidance for agents:

- **Prefer `geopmsession` (and its Python agents) over `geopmctl` /
  `geopmlaunch` / `geopmagent` whenever the required capability exists there.**
- Reach for `libgeopm` / `geopmpy` only when the task genuinely needs
  MPI-aware, application-instrumented, or hierarchical tree control that
  `geopmsession` does not yet provide.
- When adding new monitoring or control behavior, implement it as a
  `geopmdpy` `Agent` rather than a new C++ `Agent` plugin unless directed
  otherwise.

Runtime docs: [runtime.rst](docs/source/runtime.rst),
`docs/source/geopm_agent_*.7.rst`, [geopmlaunch.1.rst](docs/source/geopmlaunch.1.rst).

## Build, install, and test

### Installing released packages

Packages exist for Ubuntu, Fedora, CentOS, Rocky, openSUSE, and RHEL. Example:

```bash
sudo add-apt-repository ppa:geopm/release
sudo apt update
sudo apt install geopmd libgeopmd-dev
```

Full distro matrix, GPU build options, and Spack integration:
[install.rst](docs/source/install.rst) and [spack.rst](docs/source/spack.rst).
Confirm an existing installation with `geopmread --version`.

### Building the Access Service from source

```bash
INSTALL_PREFIX=$HOME/build/geopm
python3 -m pip install -r requirements.txt
cd libgeopmd && ./autogen.sh && ./configure --prefix=$INSTALL_PREFIX && make -j && make install
cd ../geopmdpy && python3 -m pip install .
export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH
export PATH=$INSTALL_PREFIX/bin:$PATH
```

`./autogen.sh` is only needed when building from a git clone. Notable
`libgeopmd` configure options: `--enable-debug`, `--enable-coverage`,
`--enable-nvml`, `--enable-dcgm`, `--enable-levelzero`, `--disable-systemd`,
`--disable-io-uring`. Full list: `./configure --help` and
[devel.rst](docs/source/devel.rst).

For a single-user build of both `libgeopmd` and `geopmdpy`, use
[geopmdpy/install_user.sh](geopmdpy/install_user.sh).

Build details are also in [libgeopmd/README.md](libgeopmd/README.md),
[geopmdpy/README.md](geopmdpy/README.md), and [build.rst](docs/source/build.rst).

### Running tests

```bash
# C++ unit tests (GoogleTest); results in test-suite.log
cd libgeopmd && make checkprogs -j && make check
GTEST_FILTER='HelperTest*' make check          # subset

# Python unit tests (must see the built library)
cd geopmdpy
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" \
    python3 -m unittest discover -p 'Test*.py'
```

`geopmdpy/integration_test/` and [integration](integration) require a running
`geopmd` and real hardware; do not run them in environments without it.

### Documentation

```bash
make -C docs man     # -> docs/build/man
make -C docs html    # -> docs/build/html
```

A Sphinx linter at [docs/source/\_ext/geopmlint.py](docs/source/_ext/geopmlint.py)
enforces that documented signals list aggregation, domain, format, and unit in
all-lowercase, and that `geopm_pio_*.7` pages use the section order:
Description, Requirements, Signals, Controls, Aliases, Signal Aliases, Control
Aliases, See Also.

Adding a man page requires three edits: add the target to `rst_file` in
[docs/source/conf.py](docs/source/conf.py), list the gzipped page in the
`%files` section of the doc spec file, and link it from the SEE ALSO section of
[geopm.7.rst](docs/source/geopm.7.rst) and related pages.

## Conventions

- **Python**: PEP 8. Use `python3` in all examples and documentation.
- **C++**: `astyle --style=linux --indent=spaces=4 -y -S -C -N`. Lower-snake
  variable names, `UpperCamelCase` class names, `m_` prefix for members, `g_`
  for globals. Avoid globals and preprocessor macros (prefer `enum`). Wrap
  source lines around 70–80 columns.
- **License headers**: every new file needs the BSD-3-Clause header comment.
  New installed files must also be added to the relevant `.spec` file and a
  `debian/*.install` file.
- **Pre-commit**: `python3 -m pip install pre-commit && pre-commit install`.
  Config in [.pre-commit-config.yaml](.pre-commit-config.yaml); codespell
  settings in [.codespellrc](.codespellrc).
- **Include guards**: enforced by [.github/include_guards.sh](.github/include_guards.sh).
- **Contribution flow**: open an issue first (Bug report / Feature request /
  Story), then a "Change - " issue, then a branch named `issue-NNNN`, then a PR
  that says `Fixes #NNNN`. Requirements belong in the Change issue, not the PR.
  Address review feedback with new commits rather than force-pushed rewrites.
  `Signed-off-by:` is appreciated but not required — generate it with
  `git commit -s`. Details in [CONTRIBUTING.rst](CONTRIBUTING.rst) and
  [devel.rst](docs/source/devel.rst).
- **Generative AI**: contributions produced with AI assistance are allowed under
  the policy in [CONTRIBUTING.rst](CONTRIBUTING.rst); review it before
  committing generated code.
- **CI**: [.github/workflows/build.yml](.github/workflows/build.yml) runs a
  codespell pre-build check, then a gcc/clang matrix (debug/release, ASan) that
  also runs the C/C++ include-guard check and builds and tests every component.
  Other workflows: `codeql-analysis.yml`, `coverity.yml`, `launchpad.yml` (PPA
  publishing), `website.yml` (docs deploy). Note there is no automated
  copyright-header check — headers are enforced by review.

## Documentation index

Start here when you need authoritative detail:

- [overview.rst](docs/source/overview.rst) — getting started
- [concepts.rst](docs/source/concepts.rst) — signal, control, domain, session,
  and access list explained for a first-time user
- [service.rst](docs/source/service.rst) — Access Service concepts
- [client.rst](docs/source/client.rst) / [admin.rst](docs/source/admin.rst) — user and admin guides
- [tutorial.rst](docs/source/tutorial.rst) — Bash, C, C++, Python, and Go examples
- [geopm_pio.7.rst](docs/source/geopm_pio.7.rst) — signal/control namespace
- [geopm_topo.3.rst](docs/source/geopm_topo.3.rst) — domain types
- [geopmdpy.7.rst](docs/source/geopmdpy.7.rst) — Python API overview
- [devel.rst](docs/source/devel.rst) — build options, coverage, coding style
- [ChangeLog.md](ChangeLog.md) — release history

Published site: <https://geopm.github.io>

## Agent tooling

This repository ships agent skills under `.github/skills/` and matching agent
personas under `.github/agents/`. Each skill is a `SKILL.md` plus `references/`
and `scripts/`; the format is portable across agent runtimes that read the same
convention.

### Available

| Skill | Agent | Use it for |
|---|---|---|
| [geopm-install](.github/skills/geopm-install/SKILL.md) | [geopm-install](.github/agents/geopm-install.agent.md) | Installing, configuring, and verifying GEOPM on a system under test. Trigger phrases: "install GEOPM", "geopmread command not found", "geopmd not running", "permission denied writing a control", "signal name not found", "no controls granted" |
| [geopm-optimize](.github/skills/geopm-optimize/SKILL.md) | [geopm-optimize](.github/agents/geopm-optimize.agent.md) | Tuning a workload with `geopmopt`. Trigger phrases: "optimize my benchmark", "best CPU frequency for my workload", "reduce energy consumption", "performance per watt", "power cap sweep" |

The two are chained by a **readiness gate**, worded identically in both skills
and enforced executably by
[geopm-verify-install.sh](.github/skills/geopm-install/scripts/geopm-verify-install.sh),
whose exit status *is* the gate:

- `geopmopt --list-controls` lists at least one dimension with real (non-`n/a`)
  min, max, and step values.
- `geopmread` returns a plausible power reading.
- At least one sweepable control is writable by the invoking user.

`geopm-optimize` refuses to start when the gate fails and directs the user to
`geopm-install`; `geopm-install` finishes by reporting the gate and offering
`geopm-optimize`. Both directions are also wired as `handoffs` buttons in the
agent frontmatter.

### Bundled scripts

All are read-only unless stated, and all are safe to run on a system that has
no GEOPM at all.

| Script | Purpose |
|---|---|
| [geopm-probe-system.sh](.github/skills/geopm-install/scripts/geopm-probe-system.sh) | Distro, hardware, privilege, and GEOPM state as `KEY=VALUE` plus a summary |
| [geopm-verify-install.sh](.github/skills/geopm-install/scripts/geopm-verify-install.sh) | The readiness gate. Opt-in `--write-probe` proves writability |
| [geopm-gen-access.sh](.github/skills/geopm-install/scripts/geopm-gen-access.sh) | Generates reviewable `geopmaccess` grant and revoke commands; executes nothing |
| [geopm-probe-controls.sh](.github/skills/geopm-optimize/scripts/geopm-probe-controls.sh) | Which sweep dimensions this platform can actually use, and why the others cannot |
| [geopm-check-workload.sh](.github/skills/geopm-optimize/scripts/geopm-check-workload.sh) | Baseline runtime, timeout recommendation, and run-to-run noise floor |

### One thing to know before using them

`geopmopt` is **not part of any tagged GEOPM release**. The published
`geopmdpy` 3.2.2 declares six console scripts and `geopmopt` is not among them,
nor is there an `optimize` extra to request. Until the next release it requires
a virtual environment built from the `dev` branch:

```bash
python3 -m venv ~/geopm-venv
source ~/geopm-venv/bin/activate
python3 -m pip install \
    'geopmdpy[optimize] @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy'
```

Revisit the skills when `geopmopt` ships in a release.

### Still planned

- **Signal/control discovery assistant** — mapping a user's intent to the right
  signal, domain, and IOGroup.
- **IOGroup development assistant** — adding hardware backends and `json_data`
  definitions, with the required doc and test updates.

When adding a skill, keep deep procedural detail in the skill files and keep
this document as the map of the repository. Working notes and remaining tasks
live in [plan-for-agents.md](plan-for-agents.md).
