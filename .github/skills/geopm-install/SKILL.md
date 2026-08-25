---
name: geopm-install
description: 'Install, configure, and verify GEOPM on a system under test. Use when GEOPM is missing or broken: "install GEOPM", "geopmread command not found", "geopmopt command not found", "geopmd not running", "geopmaccess permissions", "permission denied writing a control", "signal name not found", "set up geopmopt", "no controls granted". Covers distro packages from a tagged release, the geopmdpy virtual environment that geopmopt requires, source builds, container clients, access-list generation, and readiness verification. Hands off to geopm-optimize once the machine is ready.'
argument-hint: 'Name the system under test, or say "local"'
---

# GEOPM Install Assistant

Bring a system to the point where GEOPM signals can be read and the hardware
controls a tuning campaign needs can be written by the invoking user.

## When to use

- GEOPM is not installed, or `geopmread` / `geopmopt` are not on `PATH`.
- `geopmd` is not running, or signal reads fail.
- A control write fails, or `geopmaccess` shows no controls.
- The `geopm-optimize` assistant reported that the readiness gate is not met.

If the user is new to GEOPM, read
[Concepts](../../../docs/source/concepts.rst) before explaining anything.
Signal, control, domain, session, and access list are used throughout.

## The one fact that shapes most of this work

**`geopmopt` is not in any tagged GEOPM release.** The published `geopmdpy`
3.2.2 declares six console scripts and `geopmopt` is not among them, nor is
there an `optimize` extra to request. So:

- A virtual environment built from the `dev` branch is **required**, not
  preferred, for any `geopmopt` work.
- `pip install 'geopmdpy[optimize]'` does not produce a working `geopmopt`.
  Never suggest it.
- Installing or upgrading system packages does not fix a missing `geopmopt`.

Revisit this when `geopmopt` ships in a release; see
[client-venv.md](references/client-venv.md).

## Procedure

### 1. Choose the target system

Establish whether the system under test is the local machine, a remote host, or
a container. **Never assume it is local.** Ask if it is not stated.

For a remote host, send scripts over stdin:

```bash
ssh HOST 'bash -s' < ./scripts/geopm-probe-system.sh
ssh HOST 'bash -s -- --venv ~/geopm-venv' < ./scripts/geopm-verify-install.sh
```

A non-interactive `ssh HOST 'command'` does **not** read `~/.bashrc`, so proxy
variables and `PATH` additions defined there are missing. The symptom is that
`pip`, `git`, and `curl` produce no output and exit 124, while the same commands
work when the user runs them by hand. Export the proxy explicitly in the script
you send. The probe reports this as
`PROXY_ENV=defined_in_bashrc_but_not_exported`.

Campaign trials must run on the target, never on the machine driving the
install.

For containers see [container.md](references/container.md) — and prefer running
the client on the host.

### 2. Probe

```bash
./scripts/geopm-probe-system.sh
```

Read-only, exits 0 even when GEOPM is absent, and emits `KEY=VALUE` lines plus a
human summary.

### 3. Decide

Map the probe output to exactly one path using
[probe-and-decide.md](references/probe-and-decide.md). The common answer is:
administrator installs the release daemon, user creates a `dev` virtual
environment.

| Path | Reference |
|---|---|
| Release packages | [distro-packages.md](references/distro-packages.md) |
| Client virtual environment | [client-venv.md](references/client-venv.md) |
| Development snapshot daemon | [rolling-dev-packages.md](references/rolling-dev-packages.md) |
| Source build | [source-build.md](references/source-build.md) |
| Container client | [container.md](references/container.md) |

### 4. Install

Confirm before anything privileged. Present the exact commands and let the user
approve them.

### 5. Configure access

Generate a least-privilege grant rather than describing one:

```bash
./scripts/geopm-gen-access.sh --group geopm-tuning --out-dir /tmp/geopm-access
```

It filters the requested names against what the platform supports, validates
them, and prints grant *and* revoke commands. It executes nothing. See
[access-lists.md](references/access-lists.md).

Validation needs no privileges, so check a proposed list before involving an
administrator:

```bash
geopmaccess --write --dry-run --controls < proposed-controls.txt
```

### 6. Verify

```bash
./scripts/geopm-verify-install.sh --venv ~/geopm-venv
```

Report the gate status explicitly, whether it passed or failed.

## Readiness gate

Hand off to the `geopm-optimize` assistant only when all three hold:

- `geopmopt --list-controls` runs and lists at least one dimension with real
  (non-`n/a`) min, max, and step values.
- `geopmread` returns a plausible power reading.
- At least one sweepable control is writable by the invoking user.

`geopm-verify-install.sh` exits 0 exactly when these are met, so its exit status
is the gate. Do not declare a system ready on the strength of a successful
install command.

A dimension counts only when its **domain** resolved. Bounds alone are not
enough: unavailable power dimensions still print hardcoded defaults beside an
`n/a` domain.

## Safety

- Never run `sudo` without explicit, immediately preceding confirmation.
- Never modify access lists without confirmation. They are shared system
  configuration, and `--write` replaces a list while `--delete` removes an
  entire list — there is no way to drop individual names. Prefer granting a
  dedicated Unix group that started empty, which can be revoked cleanly.
- Request the minimum access the campaign needs, and always supply the matching
  revoke commands.
- Never install client dependencies into the system Python. That interpreter
  belongs to the root-owned daemon.
- Never claim a signal or control exists without checking it on the target.

### Why granting controls is reasonable

The first time a session writes a control, the Access Service records the
previous value and restores it when the session ends — including when the client
is killed. A user can change a power cap but cannot leave it changed after they
walk away, and an interrupted campaign needs no cleanup. The boundary: if
`geopmd` itself is killed while a session holds modified controls, restoration
may not happen. See [access-lists.md](references/access-lists.md).

## Diagnosing failures

Start from [troubleshooting.md](references/troubleshooting.md). The single most
important point:

**GEOPM has no permission-denied message.** A signal that is supported but not
granted fails with exactly the same text as one that does not exist:

```
PlatformIOImp::read_signal(): signal name "CPU_POWER" not found
```

Disambiguate before concluding anything:

```bash
geopmaccess --all | grep -qx CPU_POWER && echo supported || echo "NOT supported"
geopmaccess       | grep -qx CPU_POWER && echo granted   || echo "NOT granted"
```

Supported but not granted is an access problem. Not supported is a platform
limitation, and no amount of installing will fix it.

## When to stop

Say so plainly rather than continuing:

- **No administrative access and no daemon running.** The Access Service cannot
  be installed, and without it there is no safe control writing.
- **The platform exposes almost nothing.** If `geopmaccess --all` lists only a
  handful of signals, there is no RAPL or MSR access — typical of a virtual
  machine or WSL. Nothing can be tuned there. Recommend bare metal.
- **Every sweep dimension reports an `n/a` domain.** The hardware or build does
  not support the controls `geopmopt` would sweep.
