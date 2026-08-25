# Plan for GEOPM AI Assistants

Working plan for two specialized AI assistants built on top of
[AGENTS.md](AGENTS.md):

1. **GEOPM Install Assistant** — gets a working, access-controlled GEOPM
   installation on a system under test.
2. **GEOPM Optimize Assistant** — guides a user who knows their workload but
   not GEOPM through a productive `geopmopt` optimization campaign.

Status legend: `[ ]` not started, `[~]` in progress, `[x]` done.

---

## 1. Motivation and target user

The target user:

- Owns a workload or benchmark they run regularly and can reconfigure.
- Can interpret their workload's own figure of merit (GFLOPS, images/s,
  tokens/s, wall time, ...).
- Has **never used GEOPM**, does not know what a signal, control, or domain is,
  and does not know whether GEOPM is installed.
- Wants an answer to: *"what hardware settings make my workload fastest, or most
  energy efficient, on this machine?"*

The end-to-end journey the two assistants must cover:

```
"optimize my benchmark"
   │
   ├─ Install Assistant ────────────────────────────────────────────┐
   │    probe system → choose install path → install → configure    │
   │    access lists → verify writable controls                     │
   │                                                                ▼
   │                                            HANDOFF GATE: geopmread works,
   │                                            geopmd running, target controls
   │                                            writable by this user
   │                                                                │
   └─ Optimize Assistant ◄──────────────────────────────────────────┘
        probe available sweep dims → capture workload launch command →
        define metric extraction → choose objective + constraints →
        smoke test (2 trials) → full campaign → interpret results →
        emit geopmwrite config
```

Installation is factored into its own assistant because it is a prerequisite
for every other GEOPM use case (telemetry collection, `geopmsession` tracing,
Prometheus export), not just optimization.

---

## 2. Design decisions

| Decision | Choice |
|---|---|
| Repository | `geopm/geopm`, branch `geopmopt-assistant`, intended for upstream contribution |
| Primitive shape | Thin `.agent.md` personas in `.github/agents/` + detailed `SKILL.md` bundles in `.github/skills/` |
| Rationale | Agents provide tool restriction and persona; skills carry the procedure and remain portable to Claude Code / Codex, which also read `.github/skills/` conventions |
| Execution authority | Mixed with confirmation gates — auto-run read-only probes, venv creation, and dry runs; **always confirm before** `sudo`, before writing controls, and before launching a multi-trial campaign |
| Target systems | Local host, remote host over SSH, and containerized targets |
| Bundled scripts | Yes — verification and platform-probe shell scripts under each skill's `scripts/` |
| Install paths in scope | Secure default (tagged-release system packages + `geopmdpy` venv from `dev`), client-only venv, access-list configuration, source build, rolling dev packages, container/Docker |
| Campaign scope (v1) | **Single node.** Multi-node is deferred pending upstream work — see [2.2](#22-deferred-multi-node-campaigns) |
| Access-list handling | The Install Assistant **generates** ready-to-review `geopmaccess` command blocks for an administrator; it does not merely describe them, and it does not execute them without confirmation |
| Result persistence | Campaign results are written to a standard per-user location so a later session can compare or resume — see OPT-15 |
| Out of scope (v1) | Spack, `libgeopm`/`geopmpy` runtime install, multi-node campaigns, aarch64 |

### 2.1 The recommended secure install path

This is the default the Install Assistant should recommend and must document
first:

1. **System-wide, from a tagged release.** The root-owned `geopmd` daemon runs
   vetted, stable code and deliberately omits optional client-side Python
   dependencies.
   ```bash
   # Example: Ubuntu
   sudo add-apt-repository ppa:geopm/release
   sudo apt update
   sudo apt install geopmd libgeopmd-dev
   ```
2. **Client tools in a per-user virtual environment, from the `dev` branch.**
   For `geopmopt` this step is **mandatory, not a preference** — see
   [2.3](#23-geopmopt-is-not-in-any-tagged-release).
   ```bash
   python3 -m venv ~/geopm-venv
   source ~/geopm-venv/bin/activate
   python3 -m pip install \
       'geopmdpy[optimize] @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy'
   ```
3. **Access lists configured by an administrator** with `geopmaccess`, granting
   the target user or group exactly the signals and controls the campaign
   needs.

Reference: the "Installing client tools with pip" section of
[install.rst](docs/source/install.rst).

### 2.2 Deferred: multi-node campaigns

v1 of the Optimize Assistant is explicitly **single node**. The assistant must
say so rather than silently producing a command that only tunes the local host.

Multi-node support is deferred because it depends on upstream work that has not
landed yet. The intended direction:

1. Replace `geopmsession --enable-mpi` with a `--hostfile` option that
   coordinates hosts over REST (or a similar transport) instead of MPI.
2. `geopmopt` already wraps `geopmsession` to gather energy and power for its
   objectives; that wrapper then produces a report spanning all hosts in the
   hostfile.
3. The resulting best configuration is deployed across nodes through
   `geopmsession --control-config`, which applies the settings for the duration
   of a run and reverts them at session end.

Until that exists, the assistant should state the single-node limitation, and
— if the user's workload is distributed — explain that only the node running
`geopmopt` is being tuned and measured.

### 2.3 geopmopt is not in any tagged release

Verified by downloading the published sdist: `geopmdpy 3.2.2` declares only six
console scripts (`geopmd`, `geopmaccess`, `geopmexporter`, `geopmread`,
`geopmsession`, `geopmwrite`). `geopmopt` and `geopmgrid` are absent, as are
`optimizer.py`, `grid.py`, and `metrics.py`. The release declares no `optimize`
extra either — only `stats` and `dbus_xml`.

Consequences the assistants must encode:

- `pip install 'geopmdpy[optimize]'` warns about an unknown extra and yields a
  `geopmdpy` with **no optimizer at all**. The assistant must never suggest it
  for a `geopmopt` workflow.
- A `dev`-branch virtual environment is the only supported source of `geopmopt`
  until v3.3.0 or v4.0.0.
- The `geopmopt` command line is still changing. Older snapshots required
  `--metric-regex` and used per-control flags such as `--cpu-frequency`; current
  ones use `--sweep DIM` with `--metric`/`--maximize`/`--constraint`. The
  assistant must verify the interface on the target rather than assume it.
- Revisit this section, `references/client-venv.md`, and the readiness gate when
  `geopmopt` is released.

### 2.4 Virtual environment isolation trade-off

`geopmaccess` cannot run inside a plain virtual environment: it reaches the
daemon through `dasbus`, which imports PyGObject (`gi`), an OS package that pip
cannot supply. Filed upstream as
[#4057](https://github.com/geopm/geopm/issues/4057).

`--system-site-packages` fixes it but measurably weakens the environment — on
the test host it raised visible packages from 23 to 203, and caused the
optimizer to import the system `numpy` 1.26.4 instead of the 2.5.2 pip had
installed, because pip skips dependencies "already satisfied" system-wide.

**Decision:** a plain virtual environment is the default, and access-list
queries use the system `/usr/bin/geopmaccess`. That tool only queries the
daemon, so it need not match the client tool version.

---

## 3. Deliverables

```
.github/
  agents/
    geopm-install.agent.md          # Install Assistant persona
    geopm-optimize.agent.md         # Optimize Assistant persona
  skills/
    geopm-install/
      SKILL.md
      references/
        probe-and-decide.md         # Decision tree: which install path
        distro-packages.md          # Per-distro tagged-release commands
        client-venv.md              # geopmdpy venv, dev-branch snapshot
        rolling-dev-packages.md     # install_rolling.rst CI packages
        source-build.md             # autogen/configure/make, install_user.sh
        container.md                # Docker/k8s targets, geopmd visibility
        access-lists.md             # geopmaccess admin + user verification
        troubleshooting.md          # Symptom → cause → fix table
      scripts/
        geopm-probe-system.sh       # distro, kernel, CPU, GPU, msr, geopmd
        geopm-verify-install.sh     # version, service, signals, writability
        geopm-gen-access.sh         # generate reviewable geopmaccess commands
    geopm-optimize/
      SKILL.md
      references/
        sweep-dimensions.md         # --sweep grammar, dims, units, bounds
        metrics-and-constraints.md  # --metric / --constraint grammar
        flags.md                    # every flag with its real default
        metric-regex.md             # building a figure-of-merit pattern
        objective-recipes.md        # Named recipes for common goals
        campaign-design.md          # Trials budget, seeds, run-to-run noise
        interpreting-results.md     # Output config, applying results
        campaign-results.md         # Persisted artifact layout and schema
        troubleshooting.md          # Symptom → cause → fix table
      scripts/
        geopm-probe-controls.sh     # geopmopt --list-controls + availability
        geopm-check-workload.sh     # Validate launch cmd + metric regex
docs/source/
  concepts.rst                      # Shared vocabulary, linked by both skills
  (updates to geopmopt.1.rst / install.rst if gaps are found)
plan-for-agents.md                  # this file
```

---

## 4. Phase 0 — Foundations

**Status: complete.** Outcomes are recorded in
[4.1](#41-conventions-for-customization-files) and
[4.2](#42-tracking-issues).

- [x] **F-1. Confirm skill/agent discovery works in this repo.**
  Create a stub `.github/skills/geopm-install/SKILL.md` with valid frontmatter
  and confirm it appears as a `/geopm-install` slash command in VS Code chat.
  *Done when:* the stub is discoverable and loads without a frontmatter warning.
  → Scaffold created at
  [.github/skills/geopm-install/SKILL.md](.github/skills/geopm-install/SKILL.md)
  with `name`, `description`, and `argument-hint` frontmatter. It carries the
  readiness gate and safety rules so it is useful even before Phase 1 fills in
  the references. Validated mechanically: frontmatter parses as YAML, `name`
  matches the folder, the description is within the 1024-character limit, and
  all four relative links resolve. Interactive confirmation that
  `/geopm-install` appears in the chat picker may need a window reload.

- [x] **F-2. Establish the license and style conventions for these files.**
  Decide whether `.agent.md` / `SKILL.md` files carry the BSD-3-Clause header
  comment, and whether `scripts/*.sh` must carry it (they must — they are
  source files). Record the decision in this plan.
  *Done when:* a written convention exists and the F-1 stub complies.
  → See [4.1](#41-conventions-for-customization-files).

- [x] **F-3. Decide packaging/installation of the customization files.**
  Determine whether these files ship in any `.deb`/`.rpm` (likely not — they are
  repo-only developer assets) and whether `.github/skills/**` needs entries in
  `MANIFEST.in` or spec files.
  *Done when:* the answer is documented and any needed packaging edits are
  listed as tasks.
  → **No packaging change is required.** `.github/` appears in no `EXTRA_DIST`
  (checked `libgeopmd/Makefile.am` and `libgeopm/Makefile.am`), is absent from
  `geopmdpy/MANIFEST.in`, and is not referenced by `setup.cfg`,
  `pyproject.toml`, or any spec file under `release/`. Automake only distributes
  explicitly listed files, so these assets stay repository-only, which is the
  intent.

- [x] **F-4. Create the tracking issue set upstream.**
  Per [CONTRIBUTING.rst](CONTRIBUTING.rst): one "Feature request" issue
  ("As a user of the geopmopt command line tool I would like AI assistant
  guidance..."), plus "Change - " issues for the install assistant and the
  optimize assistant.
  *Done when:* issue numbers are recorded here and the working branch is named
  after the change issue.
  → See [4.2](#42-tracking-issues). Branch naming was deliberately left as
  `geopmopt-assistant`; the issues are referenced from the PR body instead.

- [x] **F-5. Write a shared vocabulary reference.**
  A single page defining signal, control, domain, domain index, session,
  save/restore, access list — written for someone who has never seen GEOPM.
  Both skills link to it.
  *Done when:* `concepts.md` exists, is under 150 lines, and explains every term
  used by the two SKILL.md bodies.
  → Promoted from a skill-local reference to a first-class documentation page at
  [docs/source/concepts.rst](docs/source/concepts.rst), registered in the
  `index.rst` toctree between `overview` and `tutorial`. Both skills link to it
  rather than duplicating it, and GEOPM users who never touch the assistants
  benefit from it too. Verified with `make -C docs html` and `make -C docs man`;
  both succeed and the page carries no Sphinx warnings.

### 4.1 Conventions for customization files

Determined by inspecting existing files rather than by preference:

| File type | License header | Basis |
|---|---|---|
| `AGENTS.md`, `SKILL.md`, `*.agent.md`, `plan-for-agents.md` | **None** | No `.md` file in the repository carries one — checked `README.md`, `ChangeLog.md`, `CODE_OF_CONDUCT.md`, `geopmdpy/README.md`, `libgeopmd/README.md`, `docs/README.md` |
| `docs/source/*.rst` | **None** | No `.rst` page under `docs/source/` carries one |
| `scripts/*.sh` | **Required** | Every `.sh` in the repository carries one |

The shell-script header, matching `.github/include_guards.sh` and
`geopmdpy/install_user.sh` but with the current year:

```bash
#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
```

The rest of the tree currently says `2015 - 2025`; new files use `2026` and the
existing files will catch up at the next bulk year update.

Additional findings that affect how these files must be written:

- **There is no copyright-header check in CI.** The pre-build job runs only
  codespell, and the separate `.github/include_guards.sh` step scans just
  `libgeopm/src`, `libgeopmd/src`, `libgeopm/test`, and `libgeopmd/test` for
  C/C++ include guards. Nothing under `.github/skills/` is scanned by it.
- **codespell does cover `.github/skills/**`.** It is not in the `.codespellrc`
  skip list. Keep these files spell-clean rather than adding a skip entry, since
  a skip would also mask genuine typos in user-facing assistant text.
- Wrap markdown prose at 80 columns to match the surrounding documentation, and
  keep each `SKILL.md` body under 500 lines with detail pushed into
  `references/`.

### 4.2 Tracking issues

Filed on `geopm/geopm`, with the two Change issues linked as sub-issues of the
feature:

| Issue | Type | Covers |
|---|---|---|
| [#4054](https://github.com/geopm/geopm/issues/4054) | Feature request | The overall assistant capability and its motivation |
| [#4055](https://github.com/geopm/geopm/issues/4055) | Change | Install assistant skill and agent (tasks INST-1 … INST-15) |
| [#4056](https://github.com/geopm/geopm/issues/4056) | Change | Optimize assistant skill and agent (tasks OPT-1 … OPT-16) |

DOC-5 (`geopmopt --dry-run`) still needs its own Feature request plus Change
pair; file those when that work starts so it can be reviewed independently.

---


## 5. Phase 1 — GEOPM Install Assistant

### 5.1 Probing and decision logic

- [x] **INST-1. Write `scripts/geopm-probe-system.sh`.**
  Read-only. Must report: distro ID and version (`/etc/os-release`), kernel
  version, architecture, CPU vendor/model, presence of `msr` module and
  `/dev/cpu/*/msr`, presence of Intel/NVIDIA GPUs and their drivers, whether
  `geopmread` is on `PATH` and its version, whether `geopmd` is active
  (`systemctl is-active geopm`), whether the user has `sudo`, whether the
  environment is a container, and the active Python version.
  *Done when:* the script runs non-destructively as an unprivileged user on at
  least Ubuntu and RHEL/Rocky, exits 0 in all cases, and emits machine-parseable
  `KEY=VALUE` output plus a human summary.
  → Written and exercised on two hosts: a Skylake Xeon with a full install
  (299 signals / 142 controls) and a WSL host serving only `TIME` with zero
  controls. Both exit 0 and the summary distinguishes them correctly. Also
  detects the proxy-in-`.bashrc` trap described in INST-15. RHEL/Rocky coverage
  is still outstanding.

- [x] **INST-2. Write `references/probe-and-decide.md`.**
  A decision tree that maps probe output to exactly one recommended install
  path: secure default, client-only venv, rolling dev packages, source build, or
  container. Must state the disqualifying condition for each path (e.g. no root
  → client-only; unsupported distro → source build or container).
  *Done when:* every leaf of the tree names a specific reference file and a
  specific first command.
  → Written as an ASCII tree keyed on the probe's `SIGNAL_READ`,
  `GEOPMOPT_STATUS`, `PRIVILEGE`, and `OS_ID` outputs, with a table of
  disqualifying conditions per path. Also lists the traps that make an agent
  pick wrongly: treating `missing_optimize_extra` as a reason to touch the
  system Python, installing a daemon to obtain `geopmopt`, and reading a
  "not found" error as an access problem.

### 5.2 Install path references

- [x] **INST-3. Write `references/distro-packages.md`.**
  Tagged-release install commands for Ubuntu (PPA), Fedora, RHEL/Rocky/CentOS
  (with the EPEL prerequisite), and openSUSE. Include the package split:
  `geopmd`, `python3-geopmdpy`, `libgeopmd-dev`/`libgeopmd-devel`, `geopmd-doc`.
  Note the "Intel GPU support" vs "Expanded Intel GPU support" repository
  distinction.
  *Done when:* commands are transcribed from [install.rst](docs/source/install.rst)
  without paraphrase drift, and each block states which repo it enables.
  → Written for Ubuntu, Fedora, Rocky/CentOS (with the EPEL prerequisite), and
  openSUSE, plus the Intel GPU vs Expanded Intel GPU repository distinction. Ends
  by pointing at the client virtual environment, since a release alone cannot
  satisfy the readiness gate.

- [x] **INST-4. Write `references/client-venv.md`.**
  The venv path: `python3 -m venv`, activation, `pip install 'geopmdpy[optimize]'`
  for the release and the `git+https://...#subdirectory=geopmdpy` form for the
  `dev` snapshot. Explain why this is safe (the venv does not affect the
  root-owned daemon) and explain the `optimize` extra's role in enabling
  `geopmopt`. Cover the `LD_LIBRARY_PATH` case when `libgeopmd` is not
  system-installed.
  *Done when:* a user with an already-deployed `geopmd` can follow it start to
  finish and end with a working `geopmopt --help`.
  → Written and validated end to end on the test host: a plain venv built from
  `dev` yields a working `geopmopt --list-controls` against the system daemon.
  The page had to be reframed because the release command does **not** provide
  `geopmopt` at all (see [2.3](#23-geopmopt-is-not-in-any-tagged-release)); it
  now documents the release command only as a trap to avoid. Also covers the
  `geopmaccess`/`gi` limitation, the `--system-site-packages` trade-off, the
  proxy pitfall, and how to detect a too-old snapshot.

- [x] **INST-5. Write `references/rolling-dev-packages.md`.**
  Summarize [install_rolling.rst](docs/source/install_rolling.rst) and state
  clearly when a dev-snapshot *daemon* is appropriate (integration testing,
  feature feedback) versus when the secure default is preferred.
  *Done when:* the file states the trust tradeoff explicitly.
  → Written, including the repository naming difference (`ppa:geopm/dev` and
  `home:/geopm/` rather than the `:release` variants) and version pinning. Makes
  the key point that dev packages are still not a route to a working `geopmopt`,
  because the packaged `geopmdpy` omits `scikit-optimize` — observed directly on
  the test host, where `/usr/bin/geopmopt` exists and always fails.

- [x] **INST-6. Write `references/source-build.md`.**
  `libgeopmd` autotools flow (`./autogen.sh`, `./configure --prefix`, `make -j`,
  `make install`), the notable configure options (`--enable-nvml`,
  `--enable-dcgm`, `--enable-levelzero`, `--enable-debug`, `--disable-systemd`),
  the `geopmdpy` pip install against a non-system `libgeopmd`
  (`LIBRARY_PATH`/`C_INCLUDE_PATH`/`LD_LIBRARY_PATH`), and
  [geopmdpy/install_user.sh](geopmdpy/install_user.sh) as the one-shot helper.
  Must state that a source build without root does **not** give you the Access
  Service, and therefore does not give you safe control writes.
  *Done when:* both the manual and `install_user.sh` flows are documented and
  the no-root limitation is called out.
  → Written. Leads with the `libgeopmd`/`geopmdpy` ABI constraint, which is the
  actual reason `install_user.sh` exists, and states up front that a user-only
  build yields no Access Service and therefore no safe control writes.

- [x] **INST-7. Write `references/container.md`.**
  How a containerized client reaches a host `geopmd`: the
  [geopmdrs](geopmdrs) gRPC-over-UDS proxy, socket mounting, and what
  [geopm_service.proto](geopm_service.proto) exposes. Include the k8s pod case
  and its limitations.
  *Done when:* a minimal working example exists (host daemon + container
  client running `geopmread`), or the file explicitly documents that this path
  is unvalidated and why.
  → Written under the second option, and labelled unvalidated at the top. The
  architecture is documented accurately from the `geopmdrs` README and the
  proto, but the test host runs `geopmd` in D-Bus mode with no gRPC socket —
  `geopmd-proxy` exits with a transport error naming the missing file, and the
  packages install no proxy systemd unit. Enabling it would mean reconfiguring a
  shared daemon, so no end-to-end recipe is claimed. The page also argues
  against containerizing the client at all when the host would do, and notes
  that node-wide hardware changes make per-pod tuning misleading on a shared
  node.

### 5.3 Access control

- [x] **INST-8. Write `references/access-lists.md`.**
  The `geopmaccess` model: default (empty) allow lists, `geopmaccess --all` to
  see everything the platform offers, writing signal and control allow lists for
  a user or group, and the difference between the *default* access list and
  *per-user/per-group* lists. Include the exact recommended control set for an
  optimization campaign:
  `CPU_FREQUENCY_MAX_CONTROL`, `CPU_UNCORE_FREQUENCY_MAX_CONTROL`,
  `CPU_POWER_LIMIT_CONTROL`, `GPU_CORE_FREQUENCY_MAX_CONTROL`,
  `GPU_POWER_LIMIT_CONTROL`, `BOARD_POWER_LIMIT_CONTROL`, plus the signals
  `TIME`, `CPU_ENERGY`, `CPU_POWER`, `GPU_ENERGY`, `GPU_POWER`,
  `BOARD_ENERGY`, `BOARD_POWER`.
  The assistant must **generate** a concrete, copy-ready command block scoped to
  the named user or group and to the controls this campaign actually needs — not
  a prose description — so an administrator can review and run it verbatim. The
  generated block must be filtered against what the platform really offers
  (`geopmaccess --all`), must never request more than the campaign needs, and
  must be accompanied by the matching command to revoke it afterwards.
  *Done when:* an admin can copy a generated block that grants exactly this set
  to a named group, a user can verify their own grants, and a revoke block is
  produced alongside it.
  → Written, and backed by a third script,
  [geopm-gen-access.sh](.github/skills/geopm-install/scripts/geopm-gen-access.sh),
  since "generate" needed to be executable rather than prose. It filters the
  requested names against `geopmaccess --all`, validates the result, and prints
  grant and revoke blocks without executing anything. Verified on the test host:
  it dropped the three GPU/board controls and four GPU/board signals that the
  platform does not support, kept 3 controls and 13 signals, and both generated
  files independently re-validate.
  Two findings shaped the page. `geopmaccess --write --dry-run` **validates
  without privileges**, so a proposed list can be checked before an
  administrator is involved. And `--write` replaces a list while `--delete`
  removes an entire list — there is no way to drop individual names — so the
  guidance is to grant a dedicated Unix group that started empty, which can be
  revoked cleanly.

- [x] **INST-9. Document the session save/restore guarantee.**
  A short section in `SKILL.md` explaining that control writes are reverted when
  the session ends, so a failed or interrupted campaign does not leave the
  machine misconfigured — and the corollary that a *crashed* client is safe but
  a *killed daemon* may not be. Link to [security.rst](docs/source/security.rst).
  *Done when:* the guarantee and its boundary conditions are both stated.
  → Covered in `references/access-lists.md` alongside the access model, since
  the two together answer "is it safe to grant this?". States the guarantee, the
  killed-daemon boundary, and the practical corollary that verifying a
  recommended configuration requires holding a session open. `SKILL.md` links to
  it rather than repeating it.

### 5.4 Verification and the handoff gate

- [x] **INST-10. Write `scripts/geopm-verify-install.sh`.**
  The authoritative "is GEOPM ready?" check. Must verify, in order:
  1. `geopmread --version` succeeds and print the version.
  2. `geopmd` is active (or explain that only `--info` style reads will work).
  3. `geopmread TIME board 0` succeeds (basic signal read).
  4. `geopmread CPU_POWER board 0` succeeds (real telemetry).
  5. For each candidate control, report readable/writable/denied.
  6. `geopmopt --list-controls` succeeds (confirms the `optimize` extra).
  Must **not** perform a real control write by default; add an opt-in
  `--write-probe` flag that writes a control to its current value and reports
  success or the permission error.
  *Done when:* the script exits non-zero with an actionable message for each
  distinct failure mode, and exits 0 only when the handoff gate below is met.
  → Written, with a `--venv DIR` option since `geopmopt` normally lives in a
  virtual environment rather than on the default `PATH`. Validated against four
  real configurations: WSL with no controls (exit 1, three actionable issues),
  the test host's system tools with a broken `geopmopt` (exit 1, one issue), a
  plain dev venv (exit 0), and a `--system-site-packages` venv (exit 0).
  `--write-probe` was exercised on real hardware: it rewrote
  `CPU_FREQUENCY_MAX_CONTROL` to its existing 3.7 GHz and left no residue.
  Two defects found and fixed during testing — a `pipefail` interaction that
  misclassified the missing `scikit-optimize` error, and a check that called a
  dimension sweepable on hardcoded default bounds even when its native domain
  was `n/a`.

- [x] **INST-11. Define the handoff gate.**
  Write the explicit criteria that the Install Assistant must confirm before
  handing off to the Optimize Assistant:
  - `geopmopt --list-controls` runs and lists at least one dimension with real
    (non-`n/a`) min/max/step.
  - `geopmread` returns a plausible power reading.
  - At least one target control is writable by the invoking user.
  *Done when:* the criteria appear in both `SKILL.md` files, worded identically.
  → Defined in `geopm-install/SKILL.md` and enforced executably by
  `geopm-verify-install.sh`, whose exit status *is* the gate. Strengthened with
  the rule that a dimension counts only when its domain resolved, since bounds
  alone are satisfied by hardcoded defaults. Still to do: repeat the identical
  wording in `geopm-optimize/SKILL.md` when that file is written in OPT-13.

- [x] **INST-12. Write `references/troubleshooting.md`.**
  Symptom → cause → fix table covering at minimum: `geopmread: command not
  found`; `geopmopt: command not found` (missing `optimize` extra);
  `ImportError: skopt`; `libgeopmd.so.*: cannot open shared object file`;
  permission denied on a control (missing access-list entry); `geopmd` inactive;
  MSR module not loaded; signal unavailable on this platform; venv shadowing the
  system `geopmdpy`; `n/a` bounds in `--list-controls`.
  *Done when:* every symptom has a concrete diagnostic command and a concrete
  fix.
  → Written with error text quoted verbatim from reproductions on two hosts.
  The most important finding leads the page: **there is no permission-denied
  message**. A signal that is supported but not granted fails with exactly the
  same `signal name "X" not found` text as a nonexistent name, confirmed by
  reading `MSR::CPU_POWER` on a host where it is supported but ungranted. The
  page gives a `geopmaccess --all` versus `geopmaccess` comparison to
  disambiguate unsupported, ungranted, and misspelled. Also records that a few
  sysfs-backed signals read successfully even when absent from the granted list.

### 5.5 Assembly

- [x] **INST-13. Write `.github/skills/geopm-install/SKILL.md`.**
  Frontmatter `name: geopm-install`; a keyword-rich `description` including
  "install GEOPM", "geopmd not running", "geopmread command not found",
  "geopmaccess permissions", "set up geopmopt". Body: when to use, the probe →
  decide → install → configure → verify procedure, the confirmation gates
  (never run `sudo` without asking), and links one level deep into
  `references/` and `scripts/`.
  *Done when:* under 500 lines, all links resolve, and the body never inlines
  content that belongs in a reference file.
  → 192 lines, frontmatter validated, all links resolve. The three-step probe /
  decide / verify workflow was executed against the test host exactly as
  written, ending in a READY gate.

- [x] **INST-14. Write `.github/agents/geopm-install.agent.md`.**
  Thin persona. `tools: [read, search, execute, edit]`. Constraints: never run
  `sudo` without explicit confirmation; never modify access lists without
  confirmation; never install into the system Python; prefer the secure default
  path; always end by running `geopm-verify-install.sh` and reporting the
  handoff gate status.
  *Done when:* the agent appears in the agent picker and correctly delegates to
  the `geopm-install` skill.
  → Written as a 60-line persona with `tools: [read, search, execute, edit,
  todo]`, explicit prohibitions, and a required output format that separates
  what the agent ran from what the user must still run. Picker behaviour is
  still to be confirmed interactively.

- [x] **INST-15. Add remote/SSH and container handling.**
  Document how the assistant targets a non-local system: require the user to
  name the host, run probes via `ssh <host> bash -s < script`, and never assume
  the local machine is the system under test. State that campaign trials must
  run on the target, not the control host.
  *Done when:* `SKILL.md` has a "Choosing the target system" section that is
  consulted before any probe.
  → It is step 1 of the procedure, before the probe. Documents the
  `ssh HOST 'bash -s' < script` pattern used throughout this work, and the trap
  that cost real time here: a non-interactive ssh does not read `~/.bashrc`, so
  proxy settings defined there are absent and every network call hangs to a 124
  timeout while working fine interactively.

---

## 6. Phase 2 — GEOPM Optimize Assistant

### 6.1 Ground truth capture

- [x] **OPT-1. Write `references/sweep-dimensions.md` from source.**
  Transcribe from [geopmdpy/geopmdpy/grid.py](geopmdpy/geopmdpy/grid.py):
  - Grammar: `--sweep CONTROL[@DOMAIN][=MIN:MAX:STEP]`, repeatable.
  - Dimension aliases: `cpu-freq`/`cpu-frequency`, `uncore-freq`/
    `cpu-uncore-frequency`, `cpu-power`, `gpu-freq`/`gpu-frequency`,
    `gpu-power`, `board-power`, `prefetch`/`prefetch-disable`.
  - Unit suffixes: frequency `Hz|kHz|MHz|GHz`, power `W|kW`.
  - Domain defaults to the control's native domain.
  - Which signals supply auto-detected bounds for each dimension (e.g.
    `cpu-freq` uses `CPU_FREQUENCY_MIN_AVAIL` / `CPU_FREQUENCY_MAX_AVAIL` /
    `CPU_FREQUENCY_STEP`), and that unavailable bounds render as `n/a`.
  - That `prefetch` sweeps a level 0..4 over the MSR prefetcher-disable bits.
  *Done when:* every alias and unit in `grid.py` appears, and the page is
  verified against `geopmopt --list-controls` output on a real machine.
  → Written and verified. The rule that matters most is that a dimension is
  usable only when its **domain** resolved: `board-power` prints a plausible
  `200 … 6000 W` range on a host that cannot sweep it at all, because those are
  hardcoded defaults printed beside an `n/a` domain. Bounds alone are a false
  positive. Also records that `cpu-power`'s maximum is the default power limit
  rather than a hardware ceiling, and that `uncore-freq`'s maximum is read from
  the control's current value, so a previously lowered limit narrows the search.

- [x] **OPT-2. Write `references/metrics-and-constraints.md` from source.**
  Transcribe from [geopmdpy/geopmdpy/metrics.py](geopmdpy/geopmdpy/metrics.py):
  - `--metric NAME=SOURCE` where SOURCE is `regex:'PATTERN'`,
    `signal:SIGNAL@DOMAIN[:AGG]`, or `expr:'EXPRESSION'`.
  - Aggregations: `delta`, `mean`, `max`, `min`; default aggregation is derived
    from the signal's behavior (constant/monotone/variable).
  - Signal sampling domains: `board`, `gpu`, `cpu`.
  - Reserved metric names and units: `time` (s, immutable/always present),
    `energy` (J), `power` (W), `fom` (arb).
  - `--constraint 'NAME OP VALUE'` with OP in `<=`, `<`, `>=`, `>`, `==`, and
    unit-suffixed values (`'power <= 250W'`, `'energy <= 5000J'`).
  - How violations are scored rather than hard-rejected.
  *Done when:* the grammar matches `parse_metric_spec` / `parse_constraint_spec`
  exactly, including the identifier regex `[A-Za-z_][A-Za-z0-9_]*`.
  → Written and each error path reproduced against the real binary. The
  consequential finding is that **constraints are soft**: a violation adds a
  penalty scaled by `1/|bound|` rather than discarding the trial, and when no
  trial is feasible the least-infeasible one is returned. A campaign therefore
  always reports a winner even for an impossible constraint, so the assistant
  must verify satisfaction rather than assume it.

- [x] **OPT-3. Write the authoritative flag reference.**
  A table of every `geopmopt` flag with its default, transcribed from
  `get_parser()` in [geopmdpy/geopmdpy/optimizer.py](geopmdpy/geopmdpy/optimizer.py):
  `--trials` (50), `--n-initial-points` (10), `--metric-regex` (None),
  `--minimize [NAME]`, `--metric` (repeatable), `--maximize NAME`,
  `--constraint` (repeatable), `--energy-domain`, `--list-metrics`,
  `--random-seed` (42), `--application-timeout` (300), `--output-file` (`-`),
  `--verbosity` (1, choices 0–3), `--print-stdout`, `--defer-write`,
  `--efficiency DOMAIN`, `--metric-bound`, `--sample-period` (0.01),
  `--penalty` (`auto`), and the trailing `-- LAUNCH ...`.
  Must record the dependency rules: `--metric-bound` requires `--metric-regex`
  **and** `--efficiency`; `--defer-write` requires `--output-file`;
  `--maximize NAME` and `--minimize NAME` are mutually exclusive.
  *Done when:* every `add_argument` call in `get_parser()` is represented with
  its real default.
  → Written to `references/flags.md`. Testing established that `geopmopt`
  validates the **entire** configuration — dimensions, bounds, units, metric
  sources, expression references, constraint names and operators, and flag
  dependencies — before launching the workload, so a malformed command costs no
  trials. All thirteen invalid cases tried exited 1 at parse time. This
  substantially reduces what DOC-5's `--dry-run` would need to add. Two warts
  recorded: `--list-metrics` returns before `--sweep` is parsed so it cannot
  validate sweeps, and `--defer-write` without `--output-file` surfaces a raw
  `ValueError` traceback rather than a clean error.

### 6.2 Workload onboarding

- [x] **OPT-4. Write the workload interview procedure.**
  The ordered questions the assistant asks a GEOPM-naive user:
  1. What single command runs your workload end to end?
  2. Roughly how long does one run take?
  3. Does it print a figure of merit? Paste one run's output.
  4. Is the figure of merit better when higher or lower?
  5. Is run-to-run variation small, or does it fluctuate?
  6. CPU-only, or does it use GPUs?
  7. What is your goal — fastest, lowest energy, best performance-per-watt,
     or lowest energy subject to a performance constraint, or fastest subject
     to a power cap?
  8. Is this a single machine, or a distributed run across several nodes?
     (v1 tunes only the node running `geopmopt` — see [2.2](#22-deferred-multi-node-campaigns).)
  *Done when:* each answer maps deterministically to a flag or a recipe in
  `objective-recipes.md`.

- [x] **OPT-5. Write `scripts/geopm-check-workload.sh`.**
  Given a launch command and a candidate regex, run the workload **once** at
  default settings and report: exit code, wall time, whether the regex matched,
  the captured value, and a recommended `--application-timeout` (measured time
  with generous headroom). Must warn if wall time is under ~10 s (too short to
  optimize meaningfully) or over ~30 min (campaign would be impractically long).
  *Done when:* the script correctly reports a regex miss and suggests a fix
  rather than failing silently.
  → Written with `--runs N` so it also measures the **noise floor**, which
  turned out to be the single most important number in a campaign. Validated on
  four cases: matching regex over three runs, a non-matching regex, a failing
  workload, and an invalid two-group regex, with distinct exit codes 0, 1, 1,
  and 2. On the synthetic benchmark it correctly flagged both a too-short
  runtime and a metric varying 7.7% between identical runs.

- [x] **OPT-6. Write the metric-regex construction guidance.**
  How to turn a line of workload output into a capturing regex, including: the
  single capture group requirement, escaping, matching the *last* occurrence
  when a workload prints per-iteration values, and validating with `python3 -c`
  before spending trials.
  *Done when:* at least four worked examples exist (GFLOPS, images/s, elapsed
  seconds, and a value embedded in JSON-ish output).
  → Written to `references/metric-regex.md` with six worked examples, adding
  scientific notation (where an incomplete character class silently truncates
  the value rather than failing to match) and per-iteration output with a
  summary line.

### 6.3 Objective design

- [x] **OPT-7. Write `references/objective-recipes.md`.**
  Named, copy-pasteable recipes, each with the goal, the exact command, and what
  the result means:
  - **Fastest run** (no metric): default objective is wall-clock runtime.
  - **Maximize a figure of merit**: `--metric-regex`.
  - **Minimize energy**: `--efficiency <domain>` with no `--metric-regex`.
  - **Best performance per watt**: `--metric-regex` + `--efficiency`.
  - **Minimize energy subject to a performance floor**: `--metric-regex` +
    `--efficiency` + `--metric-bound`.
  - **General composed objective**: `--metric fom=regex:'...'`,
    `--metric power=signal:CPU_POWER@board:mean`, `--maximize fom`,
    `--constraint 'power <= 250W'`.
  *Done when:* each recipe has been executed at least once and its output
  captured in the reference.

- [x] **OPT-8. Write `references/campaign-design.md`.**
  Guidance on: choosing `--trials` relative to the number of `--sweep`
  dimensions and single-run duration; the `--n-initial-points` relationship
  (random exploration before the Gaussian Process takes over); estimating total
  campaign wall time as `trials × run_time` and presenting that estimate to the
  user **before** starting; using `--random-seed` for reproducibility; using
  `--sample-period` and its overhead tradeoff; and choosing `--penalty`
  (`auto` vs `none` vs a fixed value) based on whether trial failures are
  expected.
  *Done when:* the page contains a concrete "estimate before you run" formula
  and a recommended starting configuration for 1-, 2-, and 3-dimensional sweeps.

- [x] **OPT-9. Define the mandatory smoke-test gate.**
  Before any full campaign, the assistant must run a reduced campaign
  (`--trials 2 --n-initial-points 2`) and confirm: the workload launches, the
  metric is extracted on both trials, controls are actually written, and a
  result is produced. Only then present the full-campaign time estimate and ask
  for confirmation. Once DOC-5 lands, prefer `geopmopt --dry-run` for the
  configuration-validation half of this gate and keep the two-trial run only to
  prove the workload and metric extraction work.
  *Done when:* the gate is stated in `SKILL.md` as a hard requirement with the
  exact reduced command, and the `--dry-run` fallback path is described.
  → Stated in the skill and in `campaign-design.md`, and executed on real
  hardware to confirm it behaves as written: two trials, distinct coordinates,
  distinct scores, configuration written, settings restored afterwards. The
  gate specifies `--penalty none` so failures abort rather than being absorbed
  into a meaningless result.

### 6.4 Results

- [x] **OPT-10. Write `references/interpreting-results.md`.**
  What `--output-file` produces (a `geopmwrite` configuration file), how to
  apply it (`geopmwrite` batch input, or `geopmsession --control-config` to hold
  the settings for the duration of a run), what `--defer-write` changes, how to
  read the trial history at `--verbosity 2`/`3`, and how to recognize a campaign
  that did not converge (best result at a search-space boundary, best result
  equal to the default, or all trials within run-to-run noise).
  *Done when:* the page shows a real output file and the exact command to apply
  it.
  → Written around a real campaign run on the test host, which produced an
  unusually instructive result: the same grid coordinate was evaluated twice and
  scored `-6.074` and `-3.157`, a factor of two at an identical setting, while
  3.7 GHz scored *worse* than 3.5 GHz for a CPU-bound workload. The campaign
  reported a confident best configuration that in fact established nothing. The
  page teaches recognizing exactly that. Also documents that `score` is negated
  for maximized objectives, that `coordinate` is a grid index rather than a
  setting, that applying a result needs `geopmsession -i sig.conf
  --control-config` (omitting `-i` makes it consume the surrounding script as
  stdin), and that hardware snaps the applied value — 3.5 GHz requested read
  back as 3.515 GHz.

- [x] **OPT-11. Write `references/troubleshooting.md` for the optimizer.**
  Symptom → cause → fix, covering at minimum: regex never matches; every trial
  hits the timeout; `--list-controls` shows `n/a` bounds; permission denied
  writing a swept control; `scikit-optimize` missing; results indistinguishable
  from noise; workload has an internal timer that a frequency cap invalidates;
  campaign appears to hang (long single-run time); constraint never satisfiable.
  *Done when:* each entry names the flag or access-list change that resolves it.

- [x] **OPT-12. Write `scripts/geopm-probe-controls.sh`.**
  Wrap `geopmopt --list-controls` and `geopmopt --list-metrics`, flag
  dimensions with `n/a` bounds as unusable on this platform, and recommend a
  starting `--sweep` set based on what is actually available and writable.
  *Done when:* on a CPU-only machine it recommends CPU dimensions only and
  explicitly excludes GPU dimensions with a stated reason.
  → Verified on the CPU-only test host: reports 4 usable dimensions, excludes
  `gpu-freq`, `gpu-power`, and `board-power` with a per-dimension reason, and
  suggests a workload-appropriate starting command with a trial budget. It also
  cross-checks `geopmaccess` to separate "the platform does not implement this"
  from "the service supports it but you were not granted it", which are
  indistinguishable in `--list-controls` alone.

- [x] **OPT-15. Persist campaign results in a standard location.**
  Define where a campaign's artifacts are stored so a later chat session can
  compare runs or resume work: a per-user directory (respect
  `XDG_DATA_HOME`, defaulting to `~/.local/share/geopm/campaigns/`), one
  timestamped subdirectory per campaign, containing the exact `geopmopt`
  command line, the resolved sweep dimensions and their detected bounds, the
  `--output-file` configuration, the trial history, the hostname and platform
  identity, and the GEOPM version. Specify the file format (YAML, to match
  `geopmsession --report-out`) and a stable schema.
  The assistant must, at the start of a session, check this directory and offer
  to compare against or extend a prior campaign on the same host and workload.
  *Done when:* the layout and schema are documented, two campaigns on the same
  host are distinguishable, and the assistant can summarize a prior campaign
  from the stored artifacts alone. Track whether `geopmopt` should write these
  artifacts itself — if so, raise it as an upstream change alongside DOC-5.

### 6.5 Assembly

- [x] **OPT-13. Write `.github/skills/geopm-optimize/SKILL.md`.**
  Frontmatter `name: geopm-optimize`; description with trigger keywords:
  "geopmopt", "optimize benchmark power", "best CPU frequency for my workload",
  "energy efficiency tuning", "performance per watt", "power cap sweep".
  Body: prerequisites (the handoff gate), the interview → probe → smoke test →
  campaign → interpret procedure, and links one level deep.
  *Done when:* under 500 lines and a GEOPM-naive transcript can be completed
  using only the skill and its references.
  → 211 lines, frontmatter validated, all links resolve. Carries the readiness
  gate worded identically to the install skill, completing INST-11.

- [x] **OPT-14. Write `.github/agents/geopm-optimize.agent.md`.**
  Thin persona. `tools: [read, search, execute, edit, todo]`. Constraints:
  never launch a campaign without presenting a wall-time estimate and getting
  confirmation; never skip the smoke test; never write controls outside a
  `geopmopt`/`geopmsession` session; delegate to the install assistant when the
  handoff gate fails; never invent signal, control, or flag names — verify with
  `--list-controls` / `--list-metrics` / `geopmread --info-all`.
  *Done when:* the agent correctly refuses to proceed on a machine without
  GEOPM and routes to `geopm-install`.

- [x] **OPT-16. State the single-node scope and the multi-node path.**
  `SKILL.md` must declare that v1 tunes and measures only the node running
  `geopmopt`. When the workload interview reveals a distributed run, the
  assistant explains the limitation, offers to tune a single representative
  node, and points at the deferred design in
  [2.2](#22-deferred-multi-node-campaigns) (`geopmsession --hostfile` over REST
  replacing `--enable-mpi`, then `--control-config` to deploy the result across
  nodes).
  *Done when:* a distributed-workload transcript shows the limitation stated
  before any campaign is proposed, and the skill contains no guidance that only
  makes sense for multi-node runs.

---

## 7. Phase 3 — Integration

- [x] **INT-1. Wire the handoff in both directions.**
  `geopm-install.agent.md` ends by naming `geopm-optimize` as the next step when
  the gate passes; `geopm-optimize.agent.md` invokes or names `geopm-install`
  when the gate fails.
  *Done when:* a single prompt — "help me optimize my benchmark" — on a machine
  without GEOPM results in the install flow, then the optimize flow, without
  the user naming either assistant.
  → Wired at two levels. Both agents declare a `handoffs` entry targeting the
  other, with `send: false` so the user presses the button rather than the
  workflow auto-advancing, which matches the confirmation-gate philosophy. Both
  skills also name the other in prose, so the chain works when the default agent
  loads a skill with no persona involved.
  One schema detail worth recording: `handoffs.agent` takes an agent
  *identifier*, and a `name:` field overrides the filename-derived one. The
  `name:` fields were removed from both agents so the identifier is
  unambiguously the filename stem (`geopm-install`, `geopm-optimize`). Both
  handoff targets were verified to resolve to real files.
  Interactive confirmation that a single naive prompt traverses both flows is
  still outstanding — it needs a human in the chat UI, and is covered by VAL-1.

- [x] **INT-2. Update [AGENTS.md](AGENTS.md).**
  Replace the speculative "Agent tooling roadmap" entries with the delivered
  skills and their real paths and trigger phrases.
  *Done when:* the roadmap section describes what exists, and remaining ideas
  are clearly marked as planned.
  → Replaced with an "Agent tooling" section listing both delivered skills and
  agents with their trigger phrases, the readiness gate that chains them, and a
  table of the five bundled scripts. Also states up front that `geopmopt` is
  absent from tagged releases and gives the virtual-environment command, since
  that is the first thing any reader of this repository needs to know before
  using the optimize skill. The two unbuilt ideas are kept under "Still
  planned".

- [x] **INT-3. Verify multi-tool portability.**
  Confirm the `.github/skills/` bundles are usable by at least one non-VS Code
  agent runtime that reads the same convention; note any VS Code-specific
  frontmatter that other runtimes ignore.
  *Done when:* portability findings are recorded here.
  → Verified structurally, not by executing another runtime — no second agent
  runtime is available in this environment, so this is a format-compatibility
  review rather than an interoperability test.

  **Portable.** Every `SKILL.md` is plain Markdown with a YAML block whose only
  required keys are `name` and `description`, both of which are common to the
  VS Code and Claude skill formats. The `references/` and `scripts/` layout and
  the relative links between them carry no runtime dependency. All content is
  readable as documentation even with no runtime at all.

  **Not portable, and deliberately so.**
  - **Skill discovery path.** VS Code reads `.github/skills/`, `.agents/skills/`,
    and `.claude/skills/`; Claude Code reads only `.claude/skills/`. So these
    bundles are discovered by VS Code but not by Claude Code without a symlink
    or a copy. Adding `.claude/` directories to an upstream repository is a
    packaging decision for the maintainers, so it was not done unilaterally.
  - **Agent format.** `.github/agents/*.agent.md` is VS Code's. Claude uses
    `.claude/agents/*.md` with a comma-separated `tools` string rather than a
    YAML array, plus `disallowedTools`.
  - **Frontmatter other runtimes ignore.** `argument-hint`, `handoffs`,
    `user-invocable`, `disable-model-invocation`, `agents`, `model`, `target`,
    and `hooks`. Ignoring them degrades gracefully: the skill body and
    references still load, only the handoff buttons and tool restrictions are
    lost.

  **Consequence for authoring.** Keep anything essential in the body rather
  than the frontmatter. The bidirectional handoff is therefore stated in prose
  in both skills as well as declared in `handoffs`, so the chain survives on a
  runtime that drops the frontmatter.

---

## 8. Phase 4 — Validation

- [ ] **VAL-1. Dry-run transcript: fresh Ubuntu, root available.**
  Full journey from "optimize my benchmark" to an applied `geopmwrite` config
  using a trivial synthetic workload. Capture the transcript.
  *Done when:* the transcript shows zero points where the assistant invented a
  flag, signal, or package name.

- [ ] **VAL-2. Dry-run transcript: no root, `geopmd` already deployed.**
  Client-only venv path. Confirm the assistant never proposes `sudo` and
  correctly identifies missing access-list entries as an admin action.
  *Done when:* the transcript ends with a concrete request the user can send to
  their administrator.

- [ ] **VAL-3. Negative test: platform with no writable controls.**
  Confirm the assistants fail loudly and usefully rather than producing a
  command that will error mid-campaign.
  *Done when:* the failure message names the exact missing access-list entries.

- [ ] **VAL-4. Negative test: workload with no figure of merit.**
  Confirm the assistant falls back to the default runtime objective and explains
  that choice.
  *Done when:* the transcript shows the fallback being explained, not silently
  applied.

- [ ] **VAL-5. Real-hardware campaign.**
  Run one full multi-dimension campaign on a real Intel platform using a real
  benchmark. Record the wall time, the trial count, and whether the recommended
  configuration reproduced on a verification run.
  *Done when:* results and the reproduction check are recorded here.

- [ ] **VAL-6. Script portability check.**
  Run both `scripts/*.sh` under `bash -n`, `shellcheck`, and on at least two
  distros. Confirm they carry the license header and are `set -euo pipefail`
  clean (mind the known `head -c` / SIGPIPE interaction).
  *Done when:* `shellcheck` is clean and both scripts pass on both distros.

---

## 9. Phase 5 — Documentation and upstreaming

- [ ] **DOC-1. Reconcile [geopmdpy/OPTIMIZER_README.md](geopmdpy/OPTIMIZER_README.md)
  with the current CLI.**
  It documents `--cpu-frequency DOMAIN`, `--cpu-power DOMAIN`, etc., but the
  current parser exposes `--sweep DIM` with `DIM = CONTROL[@DOMAIN][=MIN:MAX:STEP]`.
  It also omits `--metric`, `--maximize`, `--constraint`, `--energy-domain`,
  `--metric-bound`, `--sample-period`, `--penalty`, `--defer-write`, and
  `--list-metrics`. Decide whether to update it or fold it into
  [geopmopt.1.rst](docs/source/geopmopt.1.rst) and delete it.
  *Done when:* no reader can follow the README and produce an invalid command.

- [ ] **DOC-2. Fill documentation gaps discovered while writing the skills.**
  Track any behavior the skills had to learn from source because the man pages
  did not state it (candidates so far: the `prefetch` sweep dimension, `n/a`
  bound semantics, `--penalty` scoring, default-aggregation-by-signal-behavior).
  Fix the man pages rather than only documenting it in the skill.
  *Done when:* each gap is either fixed in `docs/source/` or recorded here with
  a reason for deferring.

- [ ] **DOC-3. Add a short "AI assistant" note to the docs.**
  A brief section pointing users at the repository's agent tooling, likely in
  [devel.rst](docs/source/devel.rst) or [contrib.rst](docs/source/contrib.rst).
  *Done when:* the note builds cleanly with `make -C docs html` and passes the
  `geopmlint` checks.

- [ ] **DOC-4. Open the pull request.**
  Follow [CONTRIBUTING.rst](CONTRIBUTING.rst): branch named for the change
  issue, `Fixes #NNNN` in the PR body, `git commit -s`, requirements documented
  in the issue rather than the PR. Note the generative-AI contribution policy
  applies to any AI-generated content in these files.
  *Done when:* CI is green and the PR is open.

- [ ] **DOC-5. Propose and implement `geopmopt --dry-run` upstream.**
  A flag that validates the full configuration without running the workload:
  resolve every `--sweep` dimension against the platform, confirm the bounds are
  real (not `n/a`), confirm every swept control is writable by the invoking
  user, parse and validate all `--metric` and `--constraint` specifications,
  check the flag dependency rules, and print the resolved search space and
  objective — then exit without launching `-- LAUNCH`.
  This makes the OPT-9 smoke-test gate nearly free and turns the most common
  campaign failures (permission denied on a control, unsatisfiable constraint,
  malformed metric spec) into instant errors instead of wasted trials.
  File it as a separate "Feature request" plus "Change - " issue pair so it can
  be reviewed independently of the assistant work.
  *Done when:* the flag is implemented in
  [geopmdpy/geopmdpy/optimizer.py](geopmdpy/geopmdpy/optimizer.py), documented
  in [geopmopt.1.rst](docs/source/geopmopt.1.rst), covered by a unit test, and
  OPT-9 is updated to use it.

---

## 10. Cross-cutting requirements

### 10.1 Safety rules for both assistants

- Never run `sudo` without explicit, immediately preceding confirmation.
- Never modify `geopmaccess` allow lists without confirmation; treat them as
  shared system configuration. Generating the command block for review is
  encouraged; running it is not, until the user confirms.
- Request the minimum access needed for the campaign, and always supply the
  matching revoke command.
- Never write a hardware control outside a GEOPM session — session save/restore
  is the safety mechanism, and bypassing it can leave the machine misconfigured.
- Always present an estimated wall time before starting a campaign.
- Never claim a signal, control, sweep dimension, or flag exists without
  verifying it with `--list-controls`, `--list-metrics`, or
  `geopmread --info-all` on the actual target.
- Never imply a campaign covers more than the node running `geopmopt`.
- On a shared or production system, state that a campaign will repeatedly change
  power and frequency limits, and confirm the user is authorized to do that.

### 10.2 Anti-patterns to avoid

- Restating the man pages instead of adding decision support — the value is in
  *choosing* the right flags for this user's situation.
- Monolithic `SKILL.md` files; keep bodies under 500 lines and push detail into
  `references/`.
- Vague `description` frontmatter; the description is the discovery surface and
  must contain the phrases a naive user would actually type.
- Hardcoding platform specifics (frequency ranges, package counts) that must be
  probed at runtime.

### 10.3 Resolved scope decisions

- [x] **Multi-node campaigns?** No — v1 declares single-node scope. The
      multi-node path depends on replacing `geopmsession --enable-mpi` with a
      REST-based `--hostfile` option, after which `geopmopt`'s existing
      `geopmsession` wrapper can produce a cross-node report and deploy the
      result with `--control-config`. Captured in
      [2.2](#22-deferred-multi-node-campaigns); enforced by OPT-16.
- [x] **Persist campaign results?** Yes — a standard per-user location with a
      stable schema, so later sessions can compare or resume. Specified by
      OPT-15.
- [x] **`geopmopt --dry-run` upstream?** Yes — worth doing, and it makes the
      smoke-test gate substantially cheaper. Specified by DOC-5, consumed by
      OPT-9.
- [x] **Generate or describe `geopmaccess` commands?** Generate — the Install
      Assistant emits a reviewable, least-privilege command block plus its
      revoke counterpart, and never runs it without confirmation. Specified by
      INST-8.

---

## 11. Progress summary

| Phase | Tasks | Done |
|---|---|---|
| 0 — Foundations | 5 | 5 |
| 1 — Install Assistant | 15 | 15 |
| 2 — Optimize Assistant | 16 | 16 |
| 3 — Integration | 3 | 3 |
| 4 — Validation | 6 | 0 |
| 5 — Docs and upstreaming | 5 | 0 |
| **Total** | **50** | **39** |

Upstream issues filed so far: [#4054](https://github.com/geopm/geopm/issues/4054)
(feature), [#4055](https://github.com/geopm/geopm/issues/4055) (install
assistant), [#4056](https://github.com/geopm/geopm/issues/4056) (optimize
assistant), [#4057](https://github.com/geopm/geopm/issues/4057) (geopmaccess
fails in a virtual environment).
