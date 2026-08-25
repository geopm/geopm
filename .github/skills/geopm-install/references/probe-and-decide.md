# Choosing an install path

Run [geopm-probe-system.sh](../scripts/geopm-probe-system.sh) first, then use
its `KEY=VALUE` output with the tree below. Recommend exactly one path.

```bash
./scripts/geopm-probe-system.sh
```

## The one thing to establish first

**Is a GEOPM Access Service already running on this machine?**

```
GEOPMD_ACTIVE=active  and  SIGNAL_READ=ok
```

If yes, do not install a daemon. The remaining work is client-side, and almost
always just a virtual environment. Installing a second daemon or upgrading the
existing one is a change to shared system state that needs a reason.

## Decision tree

```
SIGNAL_READ=ok ?
├─ yes ──> A daemon is running and you can read from it.
│          GEOPMOPT_STATUS?
│          ├─ working ............... Nothing to install.  Go to the gate.
│          ├─ absent ................ client-venv.md          [most common]
│          ├─ missing_optimize_extra  client-venv.md          [most common]
│          └─ broken ................ troubleshooting.md
│
└─ no ───> GEOPM_VERSION=absent ?
           ├─ yes ──> Nothing installed.
           │          PRIVILEGE?
           │          ├─ root / sudo_nopasswd / sudo_password
           │          │      └─ OS_ID supported?
           │          │         ├─ yes ..... distro-packages.md
           │          │         └─ no ...... source-build.md
           │          └─ none
           │                 └─ Cannot install a daemon.  STOP and ask an
           │                    administrator.  Without the Access Service
           │                    there is no safe control writing.
           │
           └─ no ───> Installed but not serving.
                      GEOPMD_ACTIVE?
                      ├─ inactive/failed ... start it (distro-packages.md)
                      └─ active ............ access lists are empty
                                             (access-lists.md)
```

`OS_ID` is supported when it is `ubuntu`, `fedora`, `rocky`, `centos`, or
`opensuse*`. Anything else means no published package.

## Paths and their disqualifying conditions

| Path | Choose when | Do not choose when |
|---|---|---|
| [client-venv.md](client-venv.md) | A daemon is running and you need `geopmopt` | No daemon is running — a venv alone cannot write controls |
| [distro-packages.md](distro-packages.md) | You have root and the distribution is supported | No root, or no published package for the distribution |
| [rolling-dev-packages.md](rolling-dev-packages.md) | You specifically need an unreleased *daemon* feature | You only need `geopmopt` — a venv is enough and safer |
| [source-build.md](source-build.md) | Unsupported distribution, or a build option packages omit (NVML on openSUSE, non-x86_64) | Packages exist and would do — building is strictly more work |
| [container.md](container.md) | The client must run in a container | The client could simply run on the host |

## The common case

Most requests resolve to a two-step answer, because `geopmopt` is not in any
tagged release:

1. Administrator installs the Access Service from a release
   ([distro-packages.md](distro-packages.md)) and grants access
   ([access-lists.md](access-lists.md)). Skip if a daemon is already running.
2. User creates a client virtual environment from the `dev` branch
   ([client-venv.md](client-venv.md)).

Step 2 is required even on a machine where everything else is already perfect.
A probe reporting `GEOPMD_ACTIVE=active`, hundreds of granted signals, and
`GEOPMOPT_STATUS=missing_optimize_extra` still needs a virtual environment.

## Traps

**Do not fix `missing_optimize_extra` by installing into the system Python.**
That interpreter belongs to the root-owned daemon. The status means the packaged
`geopmdpy` omitted client-side optional dependencies, which is deliberate. The
fix is a virtual environment.

**Do not install a daemon just because `geopmopt` is missing.** A newer daemon
does not supply a working `geopmopt` — the dev packages ship it without
`scikit-optimize`, so it is present and broken.

**Do not conclude "no controls" from a failed read.** GEOPM reports the same
"not found" error for unsupported, ungranted, and misspelled names. Check
`geopmaccess --all` before deciding which problem you have.

**A container or virtual machine may be a dead end.** If `geopmaccess --all`
lists only a handful of signals, the platform exposes no RAPL or MSR and nothing
can be tuned there regardless of what you install. Move to bare metal.

## Not the local machine

If the system under test is remote, run the probe there:

```bash
ssh HOST 'bash -s' < ./scripts/geopm-probe-system.sh
```

Never assume the machine running the assistant is the machine to be tuned. Note
that a non-interactive `ssh HOST 'command'` does not read `~/.bashrc`, so proxy
settings and `PATH` additions defined there are absent — the probe reports this
as `PROXY_ENV=defined_in_bashrc_but_not_exported`, and network installs will
hang without it.

## After installing

Always finish by running the gate, pointing at the client environment:

```bash
./scripts/geopm-verify-install.sh --venv ~/geopm-venv
```

It exits 0 only when the machine is genuinely ready.
