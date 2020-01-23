GEOPM TUTORIAL FOR SYSTEM ADMINISTRATORS
========================================
This README presents a tutorial for system administrators who
would like to configure their HPC systems to enable the use of GEOPM.
There are several different use cases that are broken out into
numbered sections below.  The tutorial starts out with the least
restrictive configuration of GEOPM.  The other sections show ways that
system administrators can use GEOPM to achieve specific goals for
energy, power, and performance.

The directory contains scripts that can be used to configure systems
for use with the GEOPM runtime.  The scripts associated with each
section are numbered accordingly.

0. Verify that the required pre-requisites for running GEOPM are met
--------------------------------------------------------------------
Run ```./test_prereqs.sh``` on a compute node in the context of a regular user
to ensure the GEOPM components are installed and functioning properly.
In the event that something is misconfigured, an ERROR or WARNING message
will be displayed reccomending a course of action to address the issue
or refer the user back to the main GEOPM README for guidance.

1. Set up a system to use the GEOPM static policy plugin for SLURM
-------------------------------------------------------------------
GEOPM provides two libraries: libgeopmpolicy contains tools for interacting
with hardware signals and controls, such as geopmread, and the supporting
library functions.  libgeopm contains all the same functions and additionally
provides tools for launching and interacting with MPI applications. The SLURM
plugin requires libgeopmpolicy only; it does not use libgeopm or MPI.
If the GEOPM runtime will be installed (libgeopm and MPI launch tools), it
should be installed in a different location, preferably using the module system.
The packages are available through OpenHPC (https://openhpc.community/downloads/).
See step 1 in this guide.

A.  To build libgeopmpolicy and a compatible libgeopm_slurm.so:
    - Obtain the source code for geopm from https://github.com/geopm/geopm
    - Make sure no modules are loaded in the environment that will affect
      compilation:
        ```
        module purge
        ```
    - Run configure with a local prefix that will be used to build the plugin:
        ```
        ./configure --disable-mpi --disable-fortran --prefix=$HOME/build/geopm-no-mpi
        ```
    - Build and install for use when building the SLURM plugin
        ```
        make -j && make install
        ```
    - Build an RPM to be used to install libgeopmpolicy on the compute nodes:
        ```
        make rpm
        ```
    - Obtain the source code for the plugin from https://github.com/geopm/geopm-slurm
    - Configure using --with-geopm targeting the geopm install above:
        ```
        ./autogen.sh
        ./configure --with-geopm=$HOME/build/geopm-no-mpi --prefix=$HOME/build/geopm-slurm-plugin
        ```
    - Build the plugin:
        ```
        make && make install
        ```
B.  Install the libgeopmpolicy package in the compute nodes using the RPM.
    The library must be accessible to the root user when the plugin runs,
    for example in /usr/lib64, and this version of the library should be built
    against a toolchain available in the system default paths.

C.  Install libspank_geopm.so* into /usr/lib64/slurm on the compute nodes.

D.  Create or update plugstack.conf in /etc/slurm on the compute nodes
    to contain the following:
    ```
    optional  libgeopm_spank.so
    ```
The head node should not need plugstack.conf; or if present, it should not
contain any reference to libgeopm_spank.so.

E.  Update the SLURM configuration (/etc/slurm/slurm.conf) as follows:
    - Update PluginDir to contain "/usr/lib64/slurm"
    - In a typical setup using Warewulf, slurm.conf will be synchronized
    between head and compute nodes (refer to the OpenHPC documentation at
    https://openhpc.community/downloads/).  If this is not the case, copy
    slurm.conf to the same location on the compute nodes.

2. Enable users to do research with the GEOPM runtime
-----------------------------------------------------

Follow the instructions in "RUNTIME REQUIREMENTS" section of the
top level GEOPM README.md.
Test scripts to check the system requirements have been met can be found in XXXX.

### Prerequisites for different scenarios

- power capping
    - description
    - test: see is_rapl_working.sh
    - test: check that msr-safe, etc. is set up
- frequency limit
    - description
    - test


3. Restrict CPU frequency for jobs that do not use the GEOPM runtime
----------------------------------------------------------------------
In this scenario, the CPU frequency of all nodes in the job will be set
to a fixed value at the beginning of every job.  This setup can be used
on system to save energy by reducing the processor frequency below the
base frequency (also known as "sticker frequency").  This frequency
can be obtained by running `geopmread FREQ_STICKER board 0` on a node
of the target architecture for the configuration.

See the script "setup_fixed_frequency.sh" in this folder for an example
of how to set up a system to run all jobs at 300 MHz below the sticker
frequency.  The test script test_integration/test_plugin_static_policy.py
can be used to check whether this setup is working.

4. Set the energy efficient agent as the default agent
------------------------------------------------------
When users run jobs with GEOPM, the built-in default agent is the monitor
(no controls, monitoring only).  To change this default to the energy
efficient agent so that jobs launched with GEOPM optimize energy efficiency,
see the script "setup_default_energy_efficient.sh".  In this example, jobs
not using GEOPM will run not have their frequency changed.  The test
script XXXX can be used to check whether this setup is working.

5. Restrict power limit for jobs that do not use the GEOPM runtime
------------------------------------------------------------------
In this scenario, the package power limit of all nodes in the job will be set
to a fixed value at the beginning of every job.  This setup can be used
on system to limit the total system power.

See the script "setup_fixed_power_cap.sh" in this folder for an example of
how to set up a system to run all jobs at 50 watts below TDP.  The
test script test_integration/test_plugin_static_policy.py can be used to
check whether this setup is working.

6. Set the power balancing agent as the default agent and power cap other jobs
------------------------------------------------------------------------------
When users run jobs with GEOPM, the built-in default agent is the monitor
(no controls, monitoring only).  To change this default to the power balancer
agent so that jobs launched with GEOPM optimize performance under a power cap,
see the script "setup_default_power_balancer.sh".  In this example, jobs
not using GEOPM will have their power cap set to a fixed cap across all nodes.
The test script XXXX can be used to check whether this setup is working.

7. Restrict CPU frequency or enforce energy efficient agent
-----------------------------------------------------------
In this scenario, the CPU frequency of all jobs will be capped at a specified
maximum, but users who request GEOPM using the `geopmlaunch` tool can be
allowed to run at higher maximum frequencies when their application is more
efficient.  This is achieved by setting the override environment values
such that all GEOPM users must use the energy efficient agent.

See the test script "setup_override_energy_efficient.sh".
The test script XXXX can be used to check whether this setup is working.

8. Restrict power cap or enforce power balancing agent
------------------------------------------------------
In this scenario, the average power of all jobs will be constrained to
a specified limit, but users who request GEOPM using the `geopmlaunch`
tool can achieve better performance using the power balancer agent while
staying under the same average power cap.

See the test script "setup_override_power_balancer.sh".
The test script XXXX can be used to check whether this setup is working.

9. Control location of report and trace file output
---------------------------------------------------
Unsupported future feature.

10. Restrict the maximum size of trace file generation
-----------------------------------------------------
Unsupported future feature.
