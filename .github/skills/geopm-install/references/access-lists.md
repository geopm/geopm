# Access lists

An unprivileged user can read and write nothing through GEOPM until an
administrator grants it. This page covers what to grant for a tuning campaign,
how to generate the exact commands, and how to undo them.

Command reference: [geopmaccess(1)](../../../../docs/source/geopmaccess.1.rst).
Vocabulary: [Concepts](../../../../docs/source/concepts.rst).

## The model

Access lists live in `/etc/geopm`, which is not readable by ordinary users.
There are two layers:

- **The default list** applies to every user of the system. Read it with
  `geopmaccess --default` (`-u`).
- **Per-group lists** extend the default for members of a named Unix group.
  Read one with `geopmaccess --group GROUP` (`-g`).

A user's effective access is the default list plus the lists of every group
they belong to.

Signals and controls are managed separately. Every command applies to signals
unless `--controls` (`-c`) is given:

```bash
geopmaccess                    # signals I may read
geopmaccess --controls         # controls I may write
geopmaccess --all              # every signal the service supports here
geopmaccess --all --controls   # every control the service supports here
```

Granting a control implicitly grants read access to that control's setting.
It does **not** grant the separate measurement signals a campaign needs, so
both lists still have to be populated.

Writing a list requires `CAP_SYS_ADMIN`, in practice root or `sudo`.

## Grant a dedicated group, not the default list

Prefer creating a group for the campaign:

```bash
sudo groupadd geopm-tuning
sudo usermod -aG geopm-tuning alice     # alice must log out and back in
```

Two reasons. First, the default list applies to everyone on the machine, so
widening it to enable one user's experiment changes the security posture for
all users. Second, and more practically, **`geopmaccess --write` replaces a
list rather than adding to it, and `--delete` removes an entire list rather
than individual names.** There is no "remove just these entries" operation. If
the campaign's names are mixed into a list that other work depends on, undoing
the grant means reconstructing the original list by hand. A dedicated group
that started empty can be removed cleanly.

## Generate the commands

Use [geopm-gen-access.sh](../scripts/geopm-gen-access.sh) rather than composing
the list by hand. It filters the requested names against what the platform
actually supports, validates the result, and prints grant *and* revoke commands
without executing anything:

```bash
./scripts/geopm-gen-access.sh --group geopm-tuning --out-dir /tmp/geopm-access
```

Filtering matters because `geopmaccess` rejects an entire list if any single
name is unsupported. On a CPU-only host the GPU and board entries are dropped
automatically:

```
# Requested and supported:
#   3 control(s), 13 signal(s)
#
# Dropped, not supported on this platform:
#   GPU_CORE_FREQUENCY_MAX_CONTROL
#   GPU_POWER_LIMIT_CONTROL
#   BOARD_POWER_LIMIT_CONTROL
#   GPU_POWER
#   GPU_ENERGY
#   BOARD_POWER
#   BOARD_ENERGY
```

Add anything extra the campaign needs with `--signal NAME` and `--control NAME`.

### Validation needs no privileges

`geopmaccess --write --dry-run` checks names against the service without
touching any file, and an ordinary user may run it. Use it to confirm a
proposed list before asking an administrator to do anything:

```console
$ echo CPU_POWER_LIMIT_CONTROL | geopmaccess --write --dry-run --controls
$ echo NOT_A_REAL_SIGNAL | geopmaccess --write --dry-run
Error: Requested access to signals that are not available: NOT_A_REAL_SIGNAL
$ echo $?
255
```

Note the second failure mode: passing a signal name to a controls list is also
rejected, since not every signal has a matching control.

```console
$ echo CPU_POWER | geopmaccess --write --dry-run --controls
Error: Requested access to controls that are not available: CPU_POWER
```

## What a campaign needs

Controls, so `geopmopt` can sweep them:

| Control | Purpose |
|---|---|
| `CPU_FREQUENCY_MAX_CONTROL` | `cpu-freq` sweep dimension |
| `CPU_UNCORE_FREQUENCY_MAX_CONTROL` | `uncore-freq` sweep dimension |
| `CPU_POWER_LIMIT_CONTROL` | `cpu-power` sweep dimension |
| `GPU_CORE_FREQUENCY_MAX_CONTROL` | `gpu-freq`, GPU platforms only |
| `GPU_POWER_LIMIT_CONTROL` | `gpu-power`, GPU platforms only |
| `BOARD_POWER_LIMIT_CONTROL` | `board-power`, where supported |

Signals, so the result can be measured and the search space discovered:

| Signal | Purpose |
|---|---|
| `TIME` | always required; the default objective is wall-clock runtime |
| `CPU_POWER`, `CPU_ENERGY` | energy and efficiency objectives |
| `CPU_FREQUENCY_STATUS` | observed frequency |
| `CPU_FREQUENCY_MIN_AVAIL`, `CPU_FREQUENCY_MAX_AVAIL`, `CPU_FREQUENCY_STEP` | auto-detected `cpu-freq` bounds |
| `CPU_POWER_MIN_AVAIL`, `CPU_POWER_MAX_AVAIL`, `CPU_POWER_LIMIT_DEFAULT` | auto-detected `cpu-power` bounds |
| `GPU_POWER`, `GPU_ENERGY`, `BOARD_POWER`, `BOARD_ENERGY` | corresponding objectives where supported |

Omitting the `*_AVAIL` and `*_STEP` signals is a subtle failure: `geopmopt`
still runs, but the affected dimension reports `n/a` bounds and cannot be
swept.

## Apply and verify

```bash
sudo geopmaccess --write --group geopm-tuning < geopm-access-signals-geopm-tuning.txt
sudo geopmaccess --write --group geopm-tuning --controls < geopm-access-controls-geopm-tuning.txt
```

Then, as a member of the group:

```bash
geopmaccess --controls                              # should list the new names
./scripts/geopm-verify-install.sh --venv ~/geopm-venv
```

Group membership only takes effect in a new login session. If the new entries
do not appear, log out and back in before investigating anything else.

## Revoke

If the group's lists were empty beforehand, deleting them restores the original
state exactly:

```bash
sudo geopmaccess --delete --group geopm-tuning
sudo geopmaccess --delete --group geopm-tuning --controls
```

If the group already had entries, capture them **before** granting:

```bash
geopmaccess --group geopm-tuning            > backup-signals.txt
geopmaccess --group geopm-tuning --controls > backup-controls.txt
```

and restore by rewriting those files, since `--delete` would otherwise remove
access that predates the campaign.

## Session save and restore

Access lists decide *whether* a user may change a setting. What protects the
machine once they do is the **session**.

The first time a session writes a control, the Access Service records the
previous value. When the session ends the service restores every control that
session changed. A session ends when the client process exits — normally, on
error, or when killed — so an interrupted campaign does not leave the hardware
capped.

Practical consequences:

- A `geopmwrite` change lasts only for that command unless a session is held
  open, for example with `geopmsession --control-config`.
- A `geopmopt` campaign that is cancelled partway leaves nothing behind. There
  is no cleanup step to run and no need to record prior values manually.
- The guarantee is scoped to the client. If `geopmd` itself is killed while a
  session holds modified controls, restoration may not happen; restarting the
  service is the recovery path.
- Because changes are reverted, verifying a recommended configuration requires
  applying it inside a session that stays open for the measurement.

This is why granting a control is less alarming than it first appears: a user
can change a power cap, but cannot leave it changed after they walk away.
See [security.rst](../../../../docs/source/security.rst).

## Wider grants

Some sites open everything the platform supports to all users:

```bash
geopmaccess --all | sudo geopmaccess --write
geopmaccess --all --controls | sudo geopmaccess --write --controls
```

Appropriate for a single-user test machine, not for a shared one. A narrower
alternative restricts the list to what has actually been exercised since the
service last started:

```bash
geopmaccess --log | sudo geopmaccess --write
geopmaccess --log --controls | sudo geopmaccess --write --controls
```

Both write the **default** list, affecting every user, because no `--group` is
given. Prefer the generated per-group grant above.
