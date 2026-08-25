# Troubleshooting

Every symptom below was reproduced on real systems, and the error text is
quoted verbatim. Match on the message, then apply the fix.

## Diagnosing "not found" — read this first

GEOPM reports the same error whether a name is unsupported, not granted to you,
or simply misspelled:

```
Error: geopm_pio_read_signal() failed: <geopm> Invalid argument:
PlatformIOImp::read_signal(): signal name "CPU_POWER" not found:
at src/PlatformIO.cpp:581
```

There is **no distinct permission-denied message**. Confirmed by reading
`MSR::CPU_POWER` on a host where it is supported by the service but absent from
the user's access list: the failure is byte-for-byte the same as for a name that
does not exist at all.

Disambiguate with `geopmaccess`:

```bash
NAME=CPU_POWER
geopmaccess --all | grep -qx "$NAME" && echo "supported" || echo "NOT supported here"
geopmaccess       | grep -qx "$NAME" && echo "granted"   || echo "NOT granted to me"
```

| `--all` | granted | Meaning | Fix |
|---|---|---|---|
| yes | yes | Name is fine; the domain or index is wrong | See the domain error below |
| yes | no | Access problem | [access-lists.md](access-lists.md) |
| no | no | Platform does not implement it, or the name is misspelled | Check spelling against `geopmread --info-all`; otherwise the hardware or driver does not support it |

One wrinkle: a few signals backed by world-readable sysfs read successfully even
when they are **not** in your granted list, because the client library serves
them locally without involving the service. `CPU_MAX_ENERGY_RANGE` behaves this
way. So "absent from the granted list" does not always mean "unreadable", but
"present in the granted list" does mean "readable".

## Command not found

### `geopmread: command not found`

GEOPM is not installed, or a virtual environment that provides it is not
active.

```bash
command -v geopmread || echo "not on PATH"
ls /usr/bin/geopm*                       # installed system-wide?
```

Install per [distro-packages.md](distro-packages.md), or activate the client
environment (`source ~/geopm-venv/bin/activate`).

### `geopmopt: command not found`

Expected on any system that only has a tagged release. `geopmopt` is not part
of any GEOPM release to date — the published `geopmdpy 3.2.2` declares six
console scripts and `geopmopt` is not among them.

Build a virtual environment from the `dev` branch:
[client-venv.md](client-venv.md). Do not try to `pip install geopmdpy[optimize]`
from a release; that extra does not exist in the release either.

## Import errors

### `ModuleNotFoundError: No module named 'skopt'`

Seen as:

```
ImportError: scikit-optimize is required for Bayesian optimization.
Install the optimize extra with: python3 -m pip install 'geopmdpy[optimize]'
```

`geopmopt` is present but its optimizer dependency is missing. This is common
when `geopmopt` came from a distribution package, because the packaged
`geopmdpy` deliberately omits client-side optional dependencies.

**Do not install `scikit-optimize` into the system Python to fix this.** The
system `geopmdpy` belongs to the root-owned daemon. Create a virtual
environment instead: [client-venv.md](client-venv.md).

Note the suggestion in GEOPM's own error message installs from a release, which
will not provide `geopmopt`. Use the `git+https://` form.

### `ModuleNotFoundError: No module named 'gi'`

```
File ".../dasbus/typing.py", line 27, in <module>
    import gi
ModuleNotFoundError: No module named 'gi'
```

Only affects `geopmaccess`, and only inside a plain virtual environment.
`dasbus` needs PyGObject, an OS package that pip cannot supply.

Use the system copy for access-list queries:

```bash
/usr/bin/geopmaccess --controls
```

This is the expected arrangement, not a broken install — `geopmaccess` only
queries the daemon and need not match the client tool version. Every other
client tool works normally in the environment. Upstream: `geopm/geopm#4057`.

### `libgeopmd.so.2: cannot open shared object file`

The library is not on the loader path, which happens when `libgeopmd` was built
with a `--prefix` outside the system directories.

```bash
export LD_LIBRARY_PATH=$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
ldconfig -p | grep libgeopmd      # confirm the system knows about it
```

## Service problems

### Signal reads fail and `geopmd` is not active

```bash
systemctl is-active geopm      # inactive / failed / unknown
sudo systemctl start geopm
sudo systemctl enable geopm    # survive reboot
journalctl -u geopm -n 50      # if it will not start
```

`is-enabled` reporting `disabled` while `is-active` reports `active` means the
service is running now but will not come back after a reboot.

### `geopmd` is active but almost nothing is readable

The access lists are empty or minimal. Check what you actually have:

```bash
geopmaccess | wc -l              # signals granted to me
geopmaccess --controls | wc -l   # controls granted to me
geopmaccess --all | wc -l        # what the service supports here
```

If the supported count is also tiny, the platform itself is the limit — a
virtual machine or container typically exposes no RAPL and no MSR, so GEOPM may
legitimately offer only `TIME`. Nothing can be tuned there; use a bare-metal
host.

If the supported count is large but the granted count is small, it is an access
problem: [access-lists.md](access-lists.md).

## Domain errors

```
Error: geopm_pio_read_signal() failed: <geopm> Invalid argument:
PlatformIOImp::read_signal(): domain 3 is not valid for signal "CPU_POWER":
at src/PlatformIO.cpp:634
```

The requested domain is finer than the signal's native domain. `CPU_POWER` is
measured per package, so `geopmread CPU_POWER cpu 0` is invalid while
`geopmread CPU_POWER package 0` and `geopmread CPU_POWER board 0` both work.
Coarser is fine and aggregates; finer is an error.

```bash
geopmread --domain                       # what this machine has
geopmread --signal-domain CPU_POWER      # native domain for a signal
geopmwrite --control-domain CPU_POWER_LIMIT_CONTROL
```

## Access list rejected

```
Error: Requested access to signals that are not available: NOT_A_REAL_SIGNAL
Error: Requested access to controls that are not available: CPU_POWER
```

`geopmaccess --write` rejects the **entire** list if any single name is
unsupported. The second message is the common case: a valid *signal* name was
put in a *controls* list, and not every signal has a matching control.

Validate before involving an administrator — `--dry-run` needs no privileges
and changes nothing:

```bash
geopmaccess --write --dry-run            < proposed-signals.txt
geopmaccess --write --dry-run --controls < proposed-controls.txt
```

Or let [geopm-gen-access.sh](../scripts/geopm-gen-access.sh) filter the list
against platform support for you.

## geopmopt runs but nothing is sweepable

```
CONTROL       DOMAIN    UNITS   MIN           MAX           STEP
gpu-freq      n/a       Hz      n/a           n/a           n/a
board-power   n/a       W       200           6000          1
```

A dimension is usable only when its **domain** resolved. Bounds alone are not
enough: unavailable power dimensions still print hardcoded defaults (`200`,
`6000`) beside an `n/a` domain. Treat any row whose domain is `n/a` as
unusable.

Causes, in order of likelihood:

1. The platform has no such hardware. `gpu-freq` shows `n/a` on a CPU-only host,
   and on hosts whose only DRM device is a management or VGA controller.
2. The bounds signals are not granted. `cpu-freq` needs
   `CPU_FREQUENCY_MIN_AVAIL`, `CPU_FREQUENCY_MAX_AVAIL`, and
   `CPU_FREQUENCY_STEP`; without them the dimension collapses to `n/a` even
   though the control itself is writable.
3. GEOPM was built without the relevant support, for example no LevelZero or
   NVML for GPU dimensions.

Check which controls the platform really implements:

```bash
geopmaccess --all --controls | grep -E 'GPU|BOARD'
```

## geopmopt has the wrong command line

An older development snapshot exposes a different interface: it requires
`--metric-regex` and uses per-control flags such as `--cpu-frequency`, rather
than `--sweep DIM` with `--metric`, `--maximize`, and `--constraint`.

```bash
geopmopt --help | grep -c -- --sweep    # 0 means the snapshot is too old
geopmread --version                     # record this with any campaign
```

Reinstall from current `dev`: [client-venv.md](client-venv.md).

## Network hangs during install

`pip`, `git`, and `curl` all produce no output and eventually exit `124`, while
the same commands work when the user runs them by hand.

The cause is almost always a proxy configured in `~/.bashrc`, which bash reads
only for **interactive** shells. A non-interactive `ssh host 'command'` inherits
no proxy, so every connection blocks until it times out.

```bash
export http_proxy=http://proxy.example.com:912
export https_proxy=http://proxy.example.com:912
export no_proxy=localhost,127.0.0.1,.example.com
```

The probe script reports this as
`PROXY_ENV=defined_in_bashrc_but_not_exported`.

When checking whether the network works, do not pipe the command into `head` or
`tail` — you will read the pager's exit status instead of the command's, and a
timeout will look like success:

```bash
git ls-remote --heads https://github.com/geopm/geopm.git dev > /tmp/out 2>&1
echo "exit=$?"
```

## Virtual environment surprises

### The wrong tool runs

A system `geopmopt` may exist and shadow the environment's, or vice versa.

```bash
command -v geopmopt geopmread
geopmread --version
```

Activate the environment, or invoke tools by absolute path
(`~/geopm-venv/bin/geopmopt`). The verification script takes `--venv DIR` for
exactly this reason.

### Packages you did not install are visible

The environment was created with `--system-site-packages`. It will work, but it
is no longer isolated: system packages satisfy dependencies that pip would
otherwise install, so the optimizer may run against a different `numpy` than
you expect, and a system upgrade can change its behavior.

```bash
python3 -m pip list | wc -l                      # ~20 plain, ~200 with system packages
python3 -c "import numpy; print(numpy.__file__)"  # dist-packages means system
```

Rebuild without the flag unless you specifically need `geopmaccess` inside the
environment. See [client-venv.md](client-venv.md).

## Still stuck

Collect the facts before asking for help:

```bash
./scripts/geopm-probe-system.sh > probe.txt
./scripts/geopm-verify-install.sh --venv ~/geopm-venv > verify.txt 2>&1
geopmread --version
```

Report bugs at https://github.com/geopm/geopm/issues/new/choose, including the
GEOPM version and the exact error text. Building with `--enable-debug` produces
more detailed messages if the problem is reproducible from source.
