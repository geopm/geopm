GEOPM TUTORIAL FOR SYSTEM ADMINISTRATORS
========================================
This README is a tutorial for system administrators who would like to configure
their HPC systems to enable the use of GEOPM. Several different use cases are
broken out into numbered sections below, beginning with the least restrictive
configuration of GEOPM. Subsequent sections show how system administrators can
use GEOPM to achieve specific goals for energy, power, and performance.

This directory contains scripts that can be used to configure systems for use
with the GEOPM runtime. The scripts associated with each section are numbered
accordingly.

Table of Contents
-----------------
0. [Verify GEOPM pre-requisites](#0-verify-geopm-pre-requisites)
1. [GEOPM+SLURM static policy plugin](#1-geopmslurm-static-policy-plugin)
2. [GEOPM runtime capabilities](#2-geopm-runtime-capabilities)
3. [Limiting the frequency for non-GEOPM jobs](#3-limiting-the-frequency-for-non-geopm-jobs)
4. [Set the frequency map agent as the default agent](#4-set-the-frequency-map-agent-as-the-default-agent)
5. [Limiting power for non-GEOPM jobs](#5-limiting-power-for-non-geopm-jobs)
6. [Set the power balancing agent as the default](#6-set-the-power-balancing-agent-as-the-default)
7. [Restrict the power cap or enforce the power balancing agent](#7-restrict-power-cap-or-enforce-power-balancing-agent)
8. [Control location of GEOPM runtime output (TBD)](#9-control-location-of-report-and-trace-file-output)
9. [Restrict maximum trace file size (TBD)](#10-restrict-the-maximum-size-of-trace-files)

## 0. Verify GEOPM pre-requisites
Run [`./00_test_prereqs.sh`](./00_test_prereqs.sh) on a compute node as an
unprivileged user to ensure the GEOPM components are installed and functioning
properly. In the event that something is misconfigured, an ERROR or WARNING
message will be displayed recommending corrective action or referring the user
back to the main GEOPM README for guidance.

## 1. GEOPM+SLURM static policy plugin
GEOPM provides two libraries: `libgeopm` and `libgeopm`. `libgeopm`
contains tools for interacting with hardware signals and controls, such as
`geopmread`, and the supporting library functions. `libgeopm` contains all of
these functions and also provides tools for launching and interacting with MPI
applications.

The `libgeopm` library must be available in order to set up a system with
the GEOPM+SLURM static policy plugin. The SLURM plugin requires
`libgeopm` alone; it does not use `libgeopm` or MPI. If the GEOPM runtime
will be installed (including `libgeopm` and MPI launch tools), it should be
installed in a different location, preferably using the module system. The
packages are available through OpenHPC
(<https://openhpc.community/downloads/>). See the [GEOPM runtime
capabilities](#2-geopm-runtime-capabilities) section in this guide.

A.  To build `libgeopm` and a compatible `libgeopm_slurm.so`:
* Obtain the source code for geopm from <https://github.com/geopm/geopm>
* Make sure no modules that will affect compilation are loaded in the
  environment:
```
module purge
```
* Run configure with a local prefix that will be used to build the plugin:
```
./configure --disable-mpi --disable-fortran --prefix=$HOME/build/geopm-no-mpi
```
* Build and install for use when building the SLURM plugin
```
make -j && make install
```
* Build an RPM to be used to install `libgeopm` on the compute nodes.
  The output from this make command will indicate where the RPMs are located:
```
make rpm
```
* Obtain the source code for the plugin from <https://github.com/geopm/geopm-slurm>
* Configure using `--with-geopm` targeting the GEOPM install above:
```
./autogen.sh
./configure --with-geopm=$HOME/build/geopm-no-mpi --prefix=$HOME/build/geopm-slurm-plugin
```
* Build the plugin:
```
make && make install
```
B.  Install the `libgeopm` package in the compute nodes using the RPM.
The library must be in a directory that is in the root user's library search
path when the plugin runs (such as `/usr/lib64`), and this version of the
library should be built against a toolchain available in the system default
paths.

C.  Install `libspank_geopm.so*` into `/usr/lib64/slurm` on the compute nodes.

D.  Create or update `plugstack.conf` in `/etc/slurm` on the compute nodes
to contain the following:
```
optional  libgeopm_spank.so
```
The head node does not need `plugstack.conf`; if present, it should not
contain any reference to `libgeopm_spank.so`.

E.  Update the SLURM configuration (`/etc/slurm/slurm.conf`) by adding
`/usr/lib64/slurm` to `PluginDir`.

In a typical setup using Warewulf, `slurm.conf` will automatically be
synchronized between head and compute nodes (refer to the OpenHPC documentation
at <https://openhpc.community/downloads/>). If you are not using Warewulf, copy
`slurm.conf` to the same location on the compute nodes.

## 2. GEOPM runtime capabilities
To enable users to directly use the GEOPM runtime, follow the instructions in
the "INSTALL" section for utilizing the pre-built RPMs or the "BUILD
REQUIREMENTS" and "INSTRUCTIONS" sections if a source build is desired.
Finally, follow the "RUN REQUIREMENTS" sections of the top level GEOPM README
to ensure the runtime requirements are met for the users' context.

## 3. Limiting the frequency for non-GEOPM jobs
In this scenario, the CPU frequency of all nodes in the job will be set
to a fixed value at the beginning of every job. This setup can be used
to save energy by reducing the processor's frequency below the
base frequency (also known as "sticker frequency"), which
can be learned by running `geopmread FREQ_STICKER board 0` on a node
of the target architecture for the configuration.

See the script [`03_setup_fixed_frequency.sh`](03_setup_fixed_frequency.sh) in this
folder for an example of how to set up a system to run all jobs at 300 MHz
below the sticker frequency. The test script
[`integration/test/test_plugin_static_policy.py`](../../integration/test/test_plugin_static_policy.py)
can be used to verify that this setup is working.

## 4. Set the frequency map agent as the default agent
When users run jobs with GEOPM, the default agent is the monitor (no controls,
monitoring only). To change this default to the frequency map agent so that
jobs launched with GEOPM use a fixed frequency, see the script
[`04_setup_default_frequency_map.sh`](04_setup_default_frequency_map.sh). In
this example, jobs not using GEOPM will run without having their frequency
changed. The test script
[`integration/test/test_plugin_static_policy.py`](../../integration/test/test_plugin_static_policy.py)
can be used to verify that this setup is working.

## 5. Limiting power for non-GEOPM jobs
In this scenario, the package power limit of all nodes in the job will be set
to a fixed value at the beginning of every job. This setup can be used on
system to limit the total system power.

See the script [`05_setup_fixed_power_cap.sh`](05_setup_fixed_power_cap.sh) in this
folder for an example of how to set up a system to run all jobs at 50 watts
below TDP. The test script
[`integration/test/test_plugin_static_policy.py`](../../integration/test/test_plugin_static_policy.py)
can be used to verify that this setup is working.

## 6. Set the power balancing agent as the default
When users run jobs with GEOPM, the default agent is the monitor (no controls,
monitoring only). To change this default to the power balancer agent so that
jobs launched with GEOPM optimize performance under a power cap, see the script
[`06_setup_default_power_balancer.sh`](06_setup_default_power_balancer.sh). In
this example, jobs not using GEOPM will have their power cap set to a fixed cap
across all nodes. The test script
[`integration/test/test_plugin_static_policy.py`](../../integration/test/test_plugin_static_policy.py)
can be used to verify that this setup is working.

## 7. Restrict power cap or enforce power balancing agent
In this scenario, the average power of all jobs will be constrained to a
specified limit, but jobs executed with GEOPM using the `geopmlaunch` tool can
achieve better performance using the power balancer agent while staying under
the same average power cap.

To make this change, refer to the script
[`07_setup_override_power_balancer.sh`](07_setup_override_power_balancer.sh).  The
test script
[`integration/test/test_plugin_static_policy.py`](../../integration/test/test_plugin_static_policy.py)
can be used to verify that this setup is working.

## 8. Control location of report and trace file output
This is a planned feature of GEOPM that is not yet available. To be notified
when this is completed or to participate in its development, please refer to
the corresponding issue ([#912](https://github.com/geopm/geopm/issues/912)).

## 9. Restrict the maximum size of trace files
This is a planned feature of GEOPM that is not yet available. To be notified
when this is completed or to participate in its development, please refer to
the corresponding issue ([#913](https://github.com/geopm/geopm/issues/913)).
