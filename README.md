![GEOPM logo](https://geopm.github.io/images/geopm-banner.png)

# GEOPM - Global Extensible Open Power Manager
[Home Page](https://geopm.github.io)
| [GEOPM Access Service Documentation](https://geopm.github.io/service.html)
| [GEOPM Runtime Service Documentation](https://geopm.github.io/runtime.html)
| [Reference Manual](https://geopm.github.io/reference.html)
| [Slack Workspace](https://geopm.slack.com)

Fine-grained batchable access to power metrics and control knobs on Linux

[![Build Status](https://github.com/geopm/geopm/actions/workflows/build.yml/badge.svg)](https://github.com/geopm/geopm/actions)
[![version](https://img.shields.io/badge/version-3.0.1-blue)](https://github.com/geopm/geopm/releases)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

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
admin interfaces to control user access. **C and C++ bindings** to this
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

Apply a 3.0 GHz maximum-allowed CPU core frequency to each CPU on the board.
This setting will be automatically reverted when the user's session ends (e.g.,
when exiting the current shell).
```
geopmwrite CPU_FREQUENCY_MAX_CONTROL board 0 3.0e9
```

Generate a CSV (comma-separated variable) trace of CPU core frequency versus
time, sampling once every second for a total of 10 seconds:
```
echo -e 'TIME board 0\nCPU_FREQUENCY_STATUS package 0' | geopmsession -p 1.0 -t 10.0
```

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
**TBD**: Show how to install to `$HOME` (useful for geopmd-client-only changes
or runtime changes) or install to `/usr/local` (useful for end-to-end setup)?

**TODO**: Validate below after all the build changes are done.

```
cd libgeopmd
./autogen.sh
./configure --prefix=$HOME/build/geopm
make -j            # Build libgeopmd
make -j checkprogs # Build the tests
make check         # Run the tests
make install       # Install to the --prefix location
cd ../libgeopm
./autogen.sh
./configure --prefix=$HOME/build/geopm
make -j            # Build libgeopm
make -j checkprogs # Build the tests
make check         # Run the tests
make install       # Install to the --prefix location
cd ..
pip install -r geopmdpy/requirements.txt
pip install ./geopmdpy
pip install -r geopmpy/requirements.txt
pip install ./geopmpy
make -C docs DESTDIR=$HOME/build/geopm
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

Please refer to the [ChangeLog](ChangeLog) for a more detailed history of
changes in each release. 

## Repository Directories

* [.github](.github) contains definitions of this repository's GitHub actions
* [docs](docs) contains web and man-page documentation for GEOPM
* [geopmdpy](geopmdpy) provides Python bindings for libgeopmd
* [geopmpy](geopmpy) provides Python bindings for libgeopm
* [integration](integration) contains integration test automation for GEOPM
* [libgeopm](libgeopm) provides the C/C++ implementation of the GEOPM Runtime Service
* [libgeopmd](libgeopmd) provides the C/C++ implementation of the GEOPM Access Service

## Guide for Contributors
We appreciate all feedback on our project. See our [contributing
guide](https://geopm.github.io/contrib.html) for guidelines on
how to report bugs, request new features, or contribute new code.

Refer to the [GEOPM Developer Guide](https://geopm.github.io/devel.html) for
information about how to interact with our build and test tools.

## License
The GEOPM source code is distributed under the 3-clause BSD license.

SEE [COPYING](COPYING) FILE FOR LICENSE INFORMATION.

## Last Update
2024 April 10

Christopher Cantalupo <christopher.m.cantalupo@intel.com> <br>
Brad Geltz <brad.geltz@intel.com> <br>

## Acknowledgments
Development of the GEOPM software package has been partially funded
through contract B609815 with Argonne National Laboratory.
