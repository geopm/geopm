# Client tools in a virtual environment

**This is the required path for `geopmopt` today.** `geopmopt` is not part of
any tagged GEOPM release, so it cannot be installed from a distribution package
or from PyPI. Until the next release (v3.3.0 or v4.0.0), the only way to obtain
it is a development snapshot installed into a Python virtual environment.

When `geopmopt` ships in a tagged release, this page and the assistant should be
revisited.

## Why a release will not do

Verified against the published package:

```console
$ python3 -m pip download --no-deps --no-binary :all: 'geopmdpy==3.2.2'
$ grep -A8 console_scripts geopmdpy-3.2.2/setup.cfg
console_scripts =
        geopmd = geopmdpy.__main__:main
        geopmaccess = geopmdpy.access:main
        geopmexporter = geopmdpy.exporter:main
        geopmread = geopmdpy.read:main
        geopmsession = geopmdpy.session:main
        geopmwrite = geopmdpy.write:main
```

Six entry points, and neither `geopmopt` nor `geopmgrid` is among them. The
supporting modules `optimizer.py`, `grid.py`, and `metrics.py` are absent from
the released package as well.

The release also declares no `optimize` extra — only `stats` and `dbus_xml`.
So this command, which appears in the install guide, does **not** produce a
working `geopmopt`:

```bash
# Does NOT provide geopmopt.  pip warns that the extra does not exist and
# installs a release that has no optimizer in it.
python3 -m pip install 'geopmdpy[optimize]'
```

Use the `dev` branch instead.

## Recommended recipe

A plain, fully isolated virtual environment:

```bash
# 1. Create and activate
python3 -m venv ~/geopm-venv
source ~/geopm-venv/bin/activate

# 2. Install the client tools from the dev branch, with the optimizer extra
python3 -m pip install \
    'geopmdpy[optimize] @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy'

# 3. Confirm
geopmopt --list-controls
```

The virtual environment affects only user-space client tools. The root-owned
`geopmd` daemon keeps running whatever the administrator installed, so a
development snapshot on the client side does not change what executes as root.

### Prerequisites

The install compiles a CFFI wrapper against `libgeopmd`, so the target needs:

| Requirement | Debian/Ubuntu package | Check |
|---|---|---|
| `libgeopmd` runtime | `libgeopmd2` | `ldconfig -p \| grep libgeopmd` |
| `libgeopmd` headers | `libgeopmd-dev` | `ls /usr/include/geopm_pio.h` |
| C compiler | `build-essential` | `command -v gcc` |
| venv module | `python3-venv` | `python3 -c 'import venv'` |
| git | `git` | `command -v git` |

If `libgeopmd` is installed somewhere other than a system path, point the
compiler at it:

```bash
LIBRARY_PATH=$PREFIX/lib C_INCLUDE_PATH=$PREFIX/include \
    python3 -m pip install 'geopmdpy[optimize] @ git+...'
export LD_LIBRARY_PATH=$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
```

## geopmaccess does not work inside a plain virtual environment

Expected, and not a sign of a broken install. `geopmaccess` reaches the daemon
through `dasbus`, which imports PyGObject (`gi`). PyGObject is an OS package
(`python3-gi`), not a pip dependency, so a plain virtual environment cannot
import it:

```console
$ ~/geopm-venv/bin/geopmaccess --controls
ModuleNotFoundError: No module named 'gi'
```

**Use the system copy for access-list queries:**

```bash
/usr/bin/geopmaccess --controls
```

This is correct rather than a compromise. `geopmaccess` only queries and edits
the daemon's access lists; it does not need to match the version of the client
tools, and access-list administration is a separate concern from optimization.

Every other client tool works normally in the virtual environment, because they
reach the service through `libgeopmd` rather than through Python D-Bus:

```console
$ ~/geopm-venv/bin/geopmread CPU_POWER board 0
85.34676174580227
$ ~/geopm-venv/bin/geopmopt --list-controls    # exits 0
```

Tracked upstream as `geopm/geopm#4057`.

## The --system-site-packages fallback, and what it costs

If you specifically need `geopmaccess` or other D-Bus Python APIs inside the
environment, create it with `--system-site-packages`:

```bash
python3 -m venv --system-site-packages ~/geopm-venv
```

This works — the virtual environment's own `geopmdpy` still takes precedence,
and `geopmaccess` functions. But it is **not the default**, because it gives up
the isolation that makes results reproducible. Measured on one host:

| | plain venv | `--system-site-packages` |
|---|---|---|
| Packages visible to pip | 23 | 203 |
| `numpy` actually imported | 2.5.2, installed in the venv | 1.26.4, from `/usr/lib/python3/dist-packages` |
| Dependency resolution | controlled by the venv | pip skips anything "already satisfied" by a system package |

The consequence is concrete: the optimizer runs against whatever `numpy` and
`scipy` the operating system happens to ship, and a later system upgrade can
change optimization behavior without any change on your part. Prefer the plain
environment and the system `geopmaccess`.

## Behind a proxy

Corporate proxies are commonly configured in `~/.bashrc`, which bash reads only
for **interactive** shells. Any non-interactive invocation — notably
`ssh host 'command'` — therefore runs with no proxy, and `pip` and `git` hang
until they time out rather than reporting a clear error.

Export the proxy explicitly in the script you send to the target:

```bash
export http_proxy=http://proxy.example.com:912
export https_proxy=http://proxy.example.com:912
export no_proxy=localhost,127.0.0.1,.example.com
python3 -m pip install 'geopmdpy[optimize] @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy'
```

Symptom to recognize: `curl`, `git ls-remote`, and `pip` all exit `124`
(the `timeout` command's exit status) with no output, while the same commands
succeed when the user runs them interactively.

## Verifying

Point the verification script at the environment:

```bash
./scripts/geopm-verify-install.sh --venv ~/geopm-venv
```

It exits 0 only when the readiness gate is met. Expected output on a healthy
system:

```
  [ok]   geopmread: /home/user/geopm-venv/bin/geopmread
  [ok]   geopmd is active
  [ok]   signal read: TIME board 0
  [ok]   telemetry: CPU_POWER board 0 = 85.3 W
  [ok]   access list via /usr/bin/geopmaccess (expected inside a venv)
  [ok]   writable controls: 3 of 6 candidates
  [ok]   geopmopt: /home/user/geopm-venv/bin/geopmopt
  [ok]   sweepable dimensions: 4
READY.
```

## Keeping the environment current

The `dev` branch moves. To pick up newer client tools:

```bash
source ~/geopm-venv/bin/activate
python3 -m pip install --force-reinstall --no-deps \
    'geopmdpy[optimize] @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy'
geopmread --version    # records the commit, e.g. 3.2.1.dev380+g9854e3735
```

Record the version in any campaign you run. The `geopmopt` command line has
changed shape during development — an older snapshot required `--metric-regex`
and exposed per-control flags such as `--cpu-frequency`, whereas current
snapshots use `--sweep DIM` together with `--metric`, `--maximize`, and
`--constraint`. **Always confirm the interface on the machine you are using**
before constructing a command:

```bash
geopmopt --help | grep -c -- --sweep    # 0 means the snapshot is too old
```
