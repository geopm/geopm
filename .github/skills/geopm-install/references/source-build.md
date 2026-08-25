# Building from source

Use this when packages will not do: an unsupported distribution, a build option
the packages do not enable, or a non-x86_64 architecture.

**Read this first:** building from source without root gives you the client
library and tools, but **not** the Access Service. Without a running `geopmd`,
an unprivileged user cannot write controls safely, and most of what makes GEOPM
useful is unavailable. A user-only build is for development and for reading
signals that happen to be world-readable — it is not a path to a tuning
campaign.

Reference: [build.rst](../../../../docs/source/build.rst) and
[devel.rst](../../../../docs/source/devel.rst).

## The ABI constraint

`geopmdpy` links `libgeopmd.so` through a CFFI wrapper. If the library at
runtime is older than the Python package expects, imports fail with missing
symbols. Any mixed installation must keep the two in step.

The two safe combinations:

- Released `libgeopmd` package plus released `geopmdpy`.
- One tree built and installed together, which is what `install_user.sh` below
  automates.

A development `geopmdpy` from pip on top of an older system `libgeopmd` is the
combination that breaks. In practice a recent release library often works with a
slightly newer `geopmdpy`, but do not rely on it; if imports fail after
upgrading the client, this is the first thing to check.

## The helper script

[geopmdpy/install_user.sh](../../../../geopmdpy/install_user.sh) builds and
installs `libgeopmd` and `geopmdpy` together for one user, which resolves the
ABI problem by construction. Arguments are forwarded to `libgeopmd`'s
`configure`.

```bash
wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
chmod a+x install_user.sh
export GEOPM_GIT_PATH=$(mktemp -d)/geopm
./install_user.sh --prefix=$HOME/geopm-build --enable-levelzero
export LD_LIBRARY_PATH=$HOME/geopm-build/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
rm -rf $(dirname $GEOPM_GIT_PATH)
```

It clones the repository when `GEOPM_GIT_PATH` does not exist. Useful
environment variables:

| Variable | Effect |
|---|---|
| `GEOPM_GIT_PATH` | Existing checkout to build from, or where to clone |
| `GEOPM_GIT_URL` | Clone source, default `https://github.com/geopm/geopm.git` |
| `GEOPM_GIT_CHECKOUT` | Branch, tag, or commit to check out |
| `GEOPM_GIT_REMOTE` | Remote used for rebase; empty disables rebase |

Run `./install_user.sh --help` for the current list.

## Building by hand

```bash
INSTALL_PREFIX=$HOME/build/geopm

git clone https://github.com/geopm/geopm.git
cd geopm/libgeopmd
./autogen.sh                     # only needed from a git clone
./configure --prefix=$INSTALL_PREFIX
make -j
make install

cd ../geopmdpy
LIBRARY_PATH=$INSTALL_PREFIX/lib \
C_INCLUDE_PATH=$INSTALL_PREFIX/include \
    python3 -m pip install .

export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH
export PATH=$INSTALL_PREFIX/bin:$PATH
```

Add `[optimize]` to the pip install to get `geopmopt`, and prefer doing it
inside a virtual environment: [client-venv.md](client-venv.md).

### Configure options worth knowing

| Option | Effect |
|---|---|
| `--prefix=DIR` | Install location |
| `--enable-levelzero` | Intel GPU support through LevelZero |
| `--enable-nvml` | NVIDIA GPU support |
| `--enable-dcgm` | NVIDIA data centre GPU support |
| `--enable-debug` | Verbose errors, no optimization |
| `--enable-coverage` | Coverage instrumentation |
| `--disable-systemd` | Build without Access Service integration |
| `--disable-io-uring` | Disable `liburing` batch I/O |

`./configure --help` lists everything. Enabling GPU support requires the
corresponding vendor libraries at build time; without them the GPU signals
simply do not appear, and GPU sweep dimensions report `n/a`.

### Build dependencies

On Debian and Ubuntu:

```bash
sudo apt install build-essential autoconf automake libtool \
                 libsystemd-dev libcap-dev liburing-dev zlib1g-dev \
                 python3-dev python3-venv git
```

`libsystemd-dev` is what enables the Access Service integration; omitting it and
then wondering why control writes are unavailable is a common trap.

## Installing the service from source

To get the daemon rather than just the client, install system-wide and enable
the unit. This requires root and is more work than the packaged path — prefer
[distro-packages.md](distro-packages.md) unless packages genuinely do not exist
for your distribution.

```bash
cd libgeopmd
./configure --prefix=/usr/local
make -j
sudo make install
sudo ldconfig

cd ../geopmdpy
sudo python3 -m pip install .

sudo systemctl enable geopm
sudo systemctl start geopm
```

The systemd unit and D-Bus configuration ship in `geopmdpy` as `geopm.service`
and `io.github.geopm.conf`; a pip install into a non-standard prefix may not
place them where systemd and D-Bus look. If `systemctl start geopm` reports the
unit does not exist, that is why. Building distribution packages instead is
usually less painful — see [build.rst](../../../../docs/source/build.rst).

## Verifying

```bash
geopmread --version
ldd $(command -v geopmread) | grep geopmd    # confirm which library is used
./scripts/geopm-verify-install.sh
```

If `geopmread` runs but `python3 -c 'import geopmdpy'` fails with a symbol
error, the ABI constraint above has been violated: rebuild both halves from the
same tree.
