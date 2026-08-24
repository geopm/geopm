---
name: geopm-install
description: 'Install, configure, and verify GEOPM on a system under test. Use when GEOPM is missing or broken: "install GEOPM", "geopmread command not found", "geopmopt command not found", "geopmd not running", "geopmaccess permissions", "permission denied writing a control", "set up geopmopt". Covers distro packages from a tagged release, a geopmdpy virtual environment for client tools, source builds, container clients, access-list configuration, and readiness verification.'
argument-hint: 'Name the system under test, or say "local"'
---

# GEOPM Install Assistant

Bring a system to the point where GEOPM signals can be read and the hardware
controls needed for a tuning campaign can be written by the invoking user.

> **Status: scaffold.** The procedure below is the intended shape. The
> reference pages and scripts it links to are being written under Phase 1 of
> [plan-for-agents.md](../../../plan-for-agents.md). Until they exist, work
> from [AGENTS.md](../../../AGENTS.md) and
> [install.rst](../../../docs/source/install.rst) directly.

## When to use

- GEOPM is not installed, or `geopmread` / `geopmopt` are not on `PATH`.
- `geopmd` is not running, or signal reads fail.
- A control write fails with a permission error.
- Another assistant reports that the readiness gate below is not met.

## Vocabulary

If the user is new to GEOPM, read
[Concepts](../../../docs/source/concepts.rst) before explaining anything —
signal, control, domain, session, and access list are used throughout.

## Procedure

1. **Choose the target system.** Local host, a remote host over SSH, or a
   container. Never assume the local machine is the system under test.
2. **Probe.** Gather distro, kernel, CPU and GPU inventory, existing GEOPM
   version, `geopmd` state, and whether the user has `sudo`.
3. **Decide the install path.** Prefer the secure default: system packages from
   a tagged release, plus client tools in a per-user virtual environment built
   from the `dev` branch.
4. **Install.** Confirm before running anything with `sudo`.
5. **Configure access.** Generate a least-privilege `geopmaccess` command block
   for an administrator to review, along with its revoke counterpart. Do not run
   it without confirmation.
6. **Verify.** Report the readiness gate explicitly.

## Readiness gate

Hand off to the `geopm-optimize` assistant only when all three hold:

- `geopmopt --list-controls` runs and lists at least one dimension with real
  (non-`n/a`) min, max, and step values.
- `geopmread` returns a plausible power reading.
- At least one target control is writable by the invoking user.

## Safety

- Never run `sudo` without explicit, immediately preceding confirmation.
- Never modify access lists without confirmation; they are shared system
  configuration.
- Request the minimum access the campaign needs, and always supply the matching
  revoke command.
