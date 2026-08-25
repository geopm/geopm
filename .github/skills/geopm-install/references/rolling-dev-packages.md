# Rolling development packages

Packages built by GEOPM's GitHub CI from the tip of the `dev` branch, published
as a rolling release. This installs a **development snapshot of the daemon
itself**, which is a different decision from running development *client tools*
against a released daemon.

Full per-distribution matrix:
[install_rolling.rst](../../../../docs/source/install_rolling.rst).

## When this is the right choice

- Testing an unreleased Access Service feature and giving feedback.
- Integration testing against the latest implementation.
- A signal, control, or IOGroup you need exists only on `dev`.

## When it is not

Reach for this only when the *daemon* must be newer. If you simply want
`geopmopt`, you do **not** need dev packages: install the release daemon from
[distro-packages.md](distro-packages.md) and put the development client tools in
a virtual environment ([client-venv.md](client-venv.md)). That combination keeps
the root-owned process on vetted code while giving you current client features.

The trade-off is explicit. `geopmd` runs as root and mediates every hardware
write on the machine. A rolling snapshot moves whenever `dev` moves, with no
release testing behind it, and an unattended package upgrade can change daemon
behavior under running workloads. Prefer a tagged release for the daemon on any
shared or production system.

## Repositories

These differ from the release repositories: drop the `:release` component.

| | Release | Development snapshot |
|---|---|---|
| Ubuntu PPA | `ppa:geopm/release` | `ppa:geopm/dev` |
| openSUSE / RPM | `home:/geopm:/release/...` | `home:/geopm/...` |
| Expanded Intel GPU | `home:geopm:release:supplementary` | `home:/geopm:/supplementary/...` |

Ubuntu also publishes `ppa:geopm/dev-next`, which tracks changes staged ahead of
`dev`. Treat it as less stable again.

### Ubuntu

```bash
add-apt-repository ppa:geopm/dev
apt update
apt install python3-geopmdpy
apt install geopmd
apt install libgeopmd-dev
apt install geopmd-doc
apt install libgeopmd-doc
```

### openSUSE

```bash
# Substitute your version for 15.6
zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.6/home:geopm.repo
zypper refresh
zypper install python3-geopmdpy geopmd libgeopmd-devel geopmd-doc libgeopmd-doc
```

Expanded Intel GPU support instead:

```bash
zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.6/home:geopm:supplementary.repo
```

### Rocky Linux and CentOS Stream

EPEL must be enabled first, as for the release packages.

```bash
pushd /etc/yum.repos.d/
# Rocky 9
wget https://download.opensuse.org/repositories/home:/geopm/RockyLinux_9_standard/home:geopm.repo
# CentOS 9 Stream
wget https://download.opensuse.org/repositories/home:/geopm/CentOS_CentOS-9_Stream/home:geopm.repo
popd

dnf install python3-geopmdpy geopmd libgeopmd-devel geopmd-doc libgeopmd-doc
```

## A snapshot daemon still does not give you geopmopt reliably

The dev *packages* do contain `geopmopt`, because it exists on `dev`. But the
packaged `python3-geopmdpy` deliberately omits client-side optional
dependencies, so `geopmopt` is installed and immediately fails:

```
ImportError: scikit-optimize is required for Bayesian optimization.
```

Observed on a host running dev packages: `/usr/bin/geopmopt` exists and is
unusable. Resist the temptation to `pip install scikit-optimize` into the
system Python — that is the daemon's interpreter. Use a virtual environment:
[client-venv.md](client-venv.md).

## Pinning

A rolling repository upgrades whenever the system updates. To hold a known
version:

```bash
# Debian and Ubuntu
apt-mark hold geopmd python3-geopmdpy libgeopmd2
# openSUSE
zypper addlock geopmd python3-geopmdpy
# Rocky, CentOS, Fedora
dnf versionlock add geopmd python3-geopmdpy
```

Record the exact version with any results you intend to compare later:

```bash
geopmread --version    # e.g. 3.2.1.dev380+gf1edf88d1f
```

The suffix after `dev` is the commit, which is what makes a measurement
reproducible.

## Reverting to a release

```bash
# Ubuntu
add-apt-repository --remove ppa:geopm/dev
add-apt-repository ppa:geopm/release
apt update
apt install --allow-downgrades python3-geopmdpy geopmd libgeopmd-dev
```

Downgrading the daemon can leave access lists referencing names the older
service does not support. Re-validate afterwards:

```bash
geopmaccess --controls | geopmaccess --write --dry-run --controls
```
