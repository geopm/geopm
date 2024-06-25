![GEOPM logo](https://geopm.github.io/images/geopm-banner.png)

# GEOPM - Global Extensible Open Power Manager
[Home Page](https://geopm.github.io)
| [GEOPM Access Service](https://geopm.github.io/service.html)
| [GEOPM Runtime Service](https://geopm.github.io/runtime.html)
| [Reference Manual](https://geopm.github.io/reference.html)
| [Slack Workspace](https://geopm.slack.com)

Fine-grained low-latency batch access to power metrics and control knobs on Linux

[![Version](https://img.shields.io/badge/Version-3.1.0-blue)](https://github.com/geopm/geopm/releases)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

[![CI Status](https://github.com/geopm/geopm/actions/workflows/build.yml/badge.svg)](https://github.com/geopm/geopm/actions)
[![Coverity Status](https://img.shields.io/coverity/scan/23217.svg)](https://scan.coverity.com/projects/geopm-geopm)
[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/geopm/geopm/badge)](https://scorecard.dev/viewer/?uri=github.com/geopm/geopm)
[![OBS Packaging: Service](https://build.opensuse.org/projects/home:geopm/packages/geopm-service/badge.svg?type=default)](https://build.opensuse.org/package/show/home:geopm/geopm-service)
[![OBS Packaging: Runtime](https://build.opensuse.org/projects/home:geopm/packages/geopm-runtime/badge.svg?type=default)](https://build.opensuse.org/package/show/home:geopm/geopm-runtime)

## Key Features
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes of
computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.

With GEOPM, a system administrator can:

  - Grant per-user or per-group access to individual metrics and controls, even
    when their underlying interfaces do not offer fine-grained access control
  - Ensure that user-driven changes to hardware settings are reverted when the
    user's process session terminates
  - Develop your own platform-specific monitor and control interfaces through
    the extensible plugin architecture

With GEOPM, an end user can:

  - Interact with hardware settings and sensors (e.g., set a CPU power limit or
    read a GPU's current power consumption) using a platform-agnostic interface
  - Generate summarized reports of power and energy behavior during execution
    of an application
  - Automatically detect MPI and OpenMP phases in an application, generating
    per-phase summaries within application reports
  - Optimize applications to improve energy efficiency or reduce the
    effects of work imbalance, system jitter, and manufacturing variation
    through built-in control algorithms
  - Develop your own runtime control algorithms through the extensible
    plugin architecture
  - Gather large groups of signal-reads or control-writes into batch
    operations, often reducing total latency to complete the operations.

GEOPM software is separated into two major components, the *GEOPM Access
Service* and the *GEOPM Runtime Service*.

[GEOPM Access Service](https://geopm.github.io/service.html): A privileged
process that provides user interfaces to hardware signals and controls and
admin interfaces to manage user access. **C and C++ bindings** to this
interface are provided through [libgeopmd](libgeopmd). **Python bindings** are
provided through the [geopmdpy](geopmdpy) package.

[GEOPM Runtime Service](https://geopm.github.io/runtime.html): An unprivileged
process that provides a framework to control platform settings based on
feedback from signals and monitored application state. This process delegates
platform interactions to the GEOPM Access Service. **C and C++ bindings** are
provided through [libgeopm](libgeopm). **Python bindings** are provided through
the [geopmpy](geopmpy) package.

## Usage
Some simple use cases are illustrated below. See the [Getting Started
Guide](https://geopm.github.io/overview.html) for additional use cases in Bash,
C, C++, and Python.

Read the current power consumption of all CPUs in the platform. The command
will print the total power consumption (in Watts) summed across all CPUs on the
board.
```
geopmread CPU_POWER board 0
```
https://github.com/geopm/geopm.github.io/assets/378319/795b297e-7a45-47a8-bf67-3fdf8be9448e

Apply a 3.0 GHz maximum-allowed CPU core frequency to each CPU on the board.
This setting will be automatically reverted when the user's session ends (e.g.,
when exiting the current shell).
```
geopmwrite CPU_FREQUENCY_MAX_CONTROL board 0 3.0e9
```
https://github.com/geopm/geopm.github.io/assets/378319/0a4f2cf8-cebf-4556-b710-6c664568790d

Generate a CSV (comma-separated variable) trace of CPU core frequency versus
time, sampling once every second for a total of 10 seconds:
```
echo -e 'TIME board 0\nCPU_FREQUENCY_STATUS package 0' | geopmsession -p 1.0 -t 10.0
```
https://github.com/geopm/geopm.github.io/assets/378319/382fbe44-5ab4-4c43-9173-982473ebccb8

Other use cases in the [Getting Started
Guide](https://geopm.github.io/overview.html) include:
* Setting admin policies for user access to signals and controls
* Exploring the platform's hardware topology
* Reading other types of signals and writing other types of controls at
  various scopes in the topology
* Repeatedly reading multiple signals in batches
* Using the GEOPM Runtime Service alongside applications

## How to Install GEOPM
We provide installable packages for Ubuntu, CentOS, openSUSE, and RHEL. Our
build systems can also be used to install GEOPM from source.

More details are available in our
[Installation](https://geopm.github.io/install.html) documentation page.

### On Ubuntu
```bash
sudo add-apt-repository ppa:geopm/release
sudo apt update
sudo apt install geopm-service libgeopmd-dev libgeopmd2 python3-geopmdpy
```

### On CentOS, openSUSE, and RHEL
Follow the installation wizards described in our [installation
guide](https://geopm.github.io/install.html#sles-opensuse-and-centos) to
install our latest release through `dnf` or `zypper`.

### From Source

Follow the documentation in our [developer build
guide](https://geopm.github.io/devel.html#developer-build-process) for details
about how to configure the build process.  Note that some dependency packages
required for the C++ builds may be missing from your system.  Refer to the
[requirements guide](https://geopm.github.io/requires.html) for details on the
packages required for build on your operating system.  In the bash script below
we show a simple way to build and install all of the GEOPM packages from the
source repository assuming that all build dependencies are installed system
wide.

```bash
# Choose install location
INSTALL_PREFIX=$HOME/build/geopm    # User install
# INSTALL_PREFIX=/usr/local         # Root install
pip install -r requirements.txt
cd libgeopmd
./autogen.sh
./configure --prefix=$INSTALL_PREFIX
make -j                             # Build libgeopmd
make install                        # Install to the --prefix location
cd ../libgeopm
./autogen.sh
./configure --prefix=$INSTALL_PREFIX
make -j                             # Build libgeopm
make install                        # Install to the --prefix location
cd ..
pip install -r geopmdpy/requirements.txt
pip install ./geopmdpy
pip install -r geopmpy/requirements.txt
pip install ./geopmpy
make -C docs man
make -C docs prefix=$INSTALL_PREFIX install_man
```

When building from source and configured with the `--prefix` option, the
libraries, and binaries will not install into the standard system paths. At this
point, you must modify your environment to specify the installed location.

```bash
    export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH
    export PATH=$INSTALL_PREFIX/bin:$PATH
```

### Major GEOPM Versions
At a high level, major GEOPM releases are summarized as follows:

* **Version 3.0**: The GEOPM runtime now also works with non-MPI applications.
* **Version 2.0**: GEOPM is split into two components: a service that manages
  platform I/O, and a runtime that writes platform power management controls
  based on feedback from MPI application state and platform state. GEOPM now
  has interfaces for Intel and NVIDIA GPUs and the `isst_interface` driver.
* **Version 1.0**: GEOPM is production-ready. It provides an abstraction layer
  for interaction with a platform's power metrics and control knobs, and
  offers the ability to interact with control knobs based on information from
  instrumented MPI applications.

Please refer to the [ChangeLog.md](ChangeLog.md) for a more detailed history of
changes in each release. 

## Repository Directories

* [.github](.github) contains definitions of this repository's GitHub actions
* [docs](docs) contains web and man-page documentation for GEOPM
* [geopmdpy](geopmdpy) provides Python bindings for libgeopmd
* [geopmpy](geopmpy) provides Python bindings for libgeopm
* [integration](integration) contains integration test automation for GEOPM
* [libgeopm](libgeopm) provides the C/C++ implementation of the GEOPM Runtime Service
* [libgeopmd](libgeopmd) provides the C/C++ implementation of the GEOPM Access Service
* [release](release) packaging files for latest release by distro
## Guide for Contributors
We appreciate all feedback on our project. See our [contributing
guide](https://geopm.github.io/contrib.html) for guidelines on
how to report bugs, request new features, or contribute new code.

Refer to the [GEOPM Developer Guide](https://geopm.github.io/devel.html) for
information about how to interact with our build and test tools.

## License
The GEOPM source code is distributed under the 3-clause BSD license.

SEE [LICENSE-BSD-3-Clause](LICENSE-BSD-3-Clause) FILE FOR LICENSE INFORMATION.

## Last Update
2024 April 10

Christopher Cantalupo <christopher.m.cantalupo@intel.com> <br>
Brad Geltz <brad.geltz@intel.com> <br>

## Acknowledgments
Development of the GEOPM software package has been partially funded
through contract B609815 with Argonne National Laboratory.
