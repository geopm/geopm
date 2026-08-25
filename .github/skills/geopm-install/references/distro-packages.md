# Distribution packages

Installing the GEOPM Access Service system-wide from a tagged release. This is
the right first step on any machine where you have administrative access: the
root-owned `geopmd` daemon should run vetted, released code.

It is only the first step for a tuning campaign. **`geopmopt` is not in any
release**, so after this you still need a client virtual environment built from
the `dev` branch — see [client-venv.md](client-venv.md).

Full per-distribution matrix, including the GEOPM Runtime Service which is not
covered here: [install.rst](../../../../docs/source/install.rst).

## What the packages provide

| Package | Contents |
|---|---|
| `geopmd` | The systemd service unit and the `geopmd` daemon |
| `python3-geopmdpy` | Python module and the client tools `geopmread`, `geopmwrite`, `geopmsession`, `geopmaccess` |
| `libgeopmd-dev` / `libgeopmd-devel` | C and C++ headers, needed to build `geopmdpy` in a virtual environment |
| `geopmd-doc` | Man pages for the Access Service |
| `libgeopmd-doc` | Man pages for C and C++ development |

Install `libgeopmd-dev` even if you do not plan to write C: the client virtual
environment compiles a CFFI wrapper against those headers.

All commands below need `sudo` or a root shell.

## Ubuntu

Packages are published through the Launchpad PPA. The 24.04 Noble packages
carry NVIDIA GPU support and expanded Intel GPU support.

```bash
add-apt-repository ppa:geopm/release
apt update
apt install python3-geopmdpy
apt install geopmd
apt install libgeopmd-dev
apt install geopmd-doc
apt install libgeopmd-doc
```

## Fedora

GEOPM is native to Fedora 42 and Rawhide, so no extra repository is needed.
Man pages are not packaged; they are published at
https://geopm.github.io instead.

```bash
dnf install python3-geopmdpy
dnf install geopmd
dnf install libgeopmd-devel
```

## Rocky Linux and CentOS Stream

**Enable EPEL first.** These distributions do not ship some of the
dependencies, and the install fails without it. Follow the Fedora project's
instructions: https://docs.fedoraproject.org/en-US/epel/getting-started/

Then add the openSUSE Build Service repository for your distribution and
install:

```bash
# Rocky 9
pushd /etc/yum.repos.d/
wget https://download.opensuse.org/repositories/home:/geopm:/release/RockyLinux_9_standard/home:geopm:release.repo
popd

# CentOS 9 Stream
pushd /etc/yum.repos.d/
wget https://download.opensuse.org/repositories/home:/geopm:/release/CentOS_CentOS-9_Stream/home:geopm:release.repo
popd

dnf install python3-geopmdpy
dnf install geopmd
dnf install libgeopmd-devel
dnf install geopmd-doc
dnf install libgeopmd-doc
```

## openSUSE

Version 15.4, 15.6, and Tumbleweed take GEOPM from the distribution's own
hardware repository, which is preferred where available. Versions 15.3, 15.5,
and 15.7 use the GEOPM team's release repository.

```bash
# 15.6 and 15.4 -- distribution hardware repository
zypper addrepo https://download.opensuse.org/repositories/hardware/15.6/hardware.repo
# Tumbleweed
zypper addrepo https://download.opensuse.org/repositories/hardware/openSUSE_Tumbleweed/hardware.repo
# 15.3, 15.5, 15.7 -- GEOPM release repository, substituting the version
zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/release/15.7/home:geopm:release.repo

zypper refresh
zypper install python3-geopmdpy
zypper install geopmd
zypper install libgeopmd-devel
zypper install geopmd-doc
zypper install libgeopmd-doc
```

## Choosing the GPU variant

openSUSE, Rocky, and CentOS publish two repository flavours:

- **Intel GPU support** — the Linux Direct Rendering Manager (DRM) sysfs
  interfaces only.
- **Expanded Intel GPU support** — adds the LevelZero interfaces, giving
  considerably more GPU signals and controls.

The expanded variant lives under `home:geopm:release:supplementary` rather than
`home:/geopm:/release`, for example:

```bash
zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.6/home:geopm:release:supplementary.repo
```

Ubuntu's PPA already includes both NVIDIA and expanded Intel GPU support.
Fedora provides the DRM interfaces only.

If the machine has no GPU, or has only a management or VGA controller, the
choice does not matter. A DRM device alone does not make GPU dimensions
sweepable — a server whose only `/sys/class/drm/card0` is a BMC VGA device will
still report `n/a` for `gpu-freq`.

## Enable the service

Packages do not start the daemon.

```bash
sudo systemctl enable geopm
sudo systemctl start geopm
```

`enable` makes it survive reboot; `start` brings it up now. Check both:

```bash
systemctl is-active geopm    # active
systemctl is-enabled geopm   # enabled
```

A host reporting `active` but `disabled` works today and silently stops working
after the next reboot.

## Grant access

Users can do nothing until an administrator populates the access lists. A
freshly installed service grants nothing at all, so `geopmread CPU_POWER board 0`
will fail even though the daemon is running.

See [access-lists.md](access-lists.md), which generates a reviewed,
least-privilege grant for a dedicated group.

## Verify

```bash
geopmread --version
systemctl is-active geopm
geopmread TIME board 0
./scripts/geopm-verify-install.sh
```

At this stage the verification script is expected to report `NOT READY` for
`geopmopt`, since a release does not include it. That is the cue to move on to
[client-venv.md](client-venv.md).

## When packages are not an option

- The distribution or version has no published package — see
  [source-build.md](source-build.md).
- You need a build option the packages do not enable, such as NVML on
  openSUSE — see [source-build.md](source-build.md).
- You want the very latest daemon rather than a release — see
  [rolling-dev-packages.md](rolling-dev-packages.md).
- You have no administrative access at all. You cannot install the Access
  Service, and without it there is no safe control writing. If a service is
  already deployed, go to [client-venv.md](client-venv.md); otherwise the
  machine cannot be tuned by GEOPM.
