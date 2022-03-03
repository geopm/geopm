
Guide for HPC Runtime Users
===========================

The GEOPM HPC Runtime package provides many built-in features.  An
advanced use case is dynamically coordinating hardware settings across
all compute nodes used by an distributed HPC application in response
to the application's behavior and resource manager requests.  The
dynamic coordination is implemented as a hierarchical control system
for scalable communication and decentralized control. The hierarchical
control system can optimize for various objective functions including
maximizing global application performance within a power bound or
minimizing energy consumption with marginal degradation of application
performance.  The root of the control hierarchy tree can communicate
with the system resource manager to extend the hierarchy above the
individual MPI application and enable the management of system power
resources for multiple MPI jobs and multiple users by the system
resource manager.

The GEOPM HPC Runtime package provides two libraries: libgeopm for use
with MPI applications, and libgeopmpolicy for use with applications
that do not link to MPI.  There are several command line tools
included in GEOPM which have dedicated manual pages.  The
geopmlaunch(1) command line tool is used to launch an MPI application
while enabling the GEOPM runtime to create a GEOPM Controller thread
on each compute node.  The Controller loads plugins and executes the
Agent algorithm to control the compute application.  The
geopmlaunch(1) command is part of the geopmpy python package that is
included in the GEOPM installation.  See the GEOPM overview man page
for further documentation and links:
`geopm(7) <https://geopm.github.io/man/geopm.7.html>`_.

The GEOPM HPC Runtime provides some built-in algorithms, each as an
"Agent" that implements the geopm::Agent(3) class interface.
A developer may extend these algorithm features by writing an Agent
plugin.  A new implementation of this class can be dynamically loaded
at runtime by the GEOPM Controller.  The Agent class defines which
data are collected, how control decisions are made, and what messages
are communicated between Agents in the tree hierarchy of compute
nodes.  The reading of data and writing of controls from within a
compute node is abstracted from the Agent through the PlatformIO
interface.  The PlatformIO interface is provided by the GEOPM Service
package which is contained in the service directory of the GEOPM
repository.  The PlatformIO abstraction enables Agent implementations
to be ported to different hardware platforms without modification.

The libgeopm library can be called directly or indirectly within MPI
applications to enable application feedback for informing the control
decisions.  The indirect calls are facilitated by GEOPM's integration
with MPI and OpenMP through their profiling decorators, and the direct
calls are made through the geopm_prof_c(3) or geopm_fortran(3)
interfaces.  Marking up a compute application with profiling
information through these interfaces can enable better integration of
the GEOPM runtime with the compute application and more precise
control.

The GEOPM HPC Runtime tutorial shows how to profile unmodified
applications, select and evaluate different GEOPM Agent algorithms,
and how to add markup to an application.  The tutorial provides a good
starting point for someone trying to get familiar witht the GEOPM HPC
Runtime.  The tutorial can be found
`here <https://github.com/geopm/geopm/tree/dev/tutorial>`__.



Build Requirements
------------------

When using the build system in the base of the GEOPM source repository
to build the GEOPM HPC Runtime some additional requirements must be
met.  If the user is not interested in building the GEOPM HPC Runtime,
these extra build requirements may be ignored.  The user may also opt
out of the specific GEOPM HPC Runtime features enabled by any of these
requirements by providing the appropriate disable flag to the base
build configure command line.

The GEOPM HPC runtime requires support for MPI, the Message Passing
Interface, standard 2.2 or higher.  In many cases meeting this
requirement will depend on the specific HPC resource being targeted
based on documentation that is site specific.  The Intel MPI
implementation may be used to meet this requirement.  The MPI
requirement may also be met through HPC packaging systems like OpenHPC
or Spack.  Additionally, the OpenMPI binaries are distributed with
most major Linux distributions, and may also be used to satisfy this
requirement.  This requirement can be met by installing the
``openmpi-devel`` package version 1.7 or greater on RHEL and SLES
Linux, and ``libopenmpi-dev`` on Ubuntu.

* Install all requirements on RHEL or CentOS

  .. code-block:: bash

      yum install openmpi-devel elfutils-libelf-devel


* Install all requirements on SUSE based distributions

  .. code-block:: bash

      zypper install openmpi-devel elfutils-libelf-devel


* Install all requirements on UBUNTU (as of 18.04.3 LTS)

  .. code-block:: bash

      apt install libtool automake libopenmpi-dev build-essential gfortran \
          libelf-dev python libsqlite3-dev


Requirements that can be avoided by removing features with configure
option:

* Remove MPI compiler requirement
  ``--disable-mpi``

* Remove Fortran compiler requirement
  ``--disable-fortran``

* Remove elfutils library requirement
  ``--disable-ompt``


For details on how to use non-standard install locations for build
requirements see

  .. code-block:: bash

    ./configure --help


which describes some options of the form ``--with-<feature>`` that can
be used for this purpose, e.g. ``--with-mpi-bin``.


Building the GEOPM HPC Runtime
------------------------------

The best recommendation for building the GEOPM HPC Runtime is to follow
the developer documentation posted
`here <https://geopm.github.io/geopmpy/devel.html>`__.  This will
enable the use of the GEOPM Service and will also provide the latest
development in the GEOPM repository.


Run Requirements
----------------

The GEOPM HPC Runtime has several requirements at time-of-use beyond
what is required for the GEOPM Service.  These requirements are
outlined here.  A user that is not interested in running the GEOPM HPC
Runtime may ignore these requirements.


MPI Requirements
^^^^^^^^^^^^^^^^

Using the GEOPM HPC Runtime requires that the package was built
against the same MPI implementation that is used at runtime to launch
the user's application.


BIOS Configuration
^^^^^^^^^^^^^^^^^^

If power governing or power balancing is the intended use case
for GEOPM deployment, then there is an additional dependency on
the BIOS being configured to support RAPL control. To check for
BIOS support, execute the following on a compute node:

.. code-block:: bash

    ./tutorial/admin/00_test_prereqs.sh


If the script output contains:

.. code-block:: none

    WARNING: The lock bit for the PKG_POWER_LIMIT MSR is set.  The power_balancer
             and power_governor agents will not function properly until this is cleared.


Please enable RAPL in your BIOS and if such an option doesn't exist please
contact your BIOS vendor to obtain a RAPL supported BIOS.

For additional information, please contact the GEOPM team.


Linux Power Management
^^^^^^^^^^^^^^^^^^^^^^

Note that other Linux mechanisms for power management can interfere
with GEOPM, and these must be disabled.  We suggest disabling the
intel_pstate kernel driver by modifying the kernel command line
through grub2 or the boot loader on your system by adding:

.. code-block:: bash

   "intel_pstate=disable"


The cpufreq driver will be enabled when the intel_pstate driver is
disabled.  The cpufreq driver has several modes controlled by the
scaling_governor sysfs entry.  When the performance mode is selected,
the driver will not interfere with GEOPM.  For SLURM based systems the
`GEOPM launch wrappers <#geopm-mpi-launch-wrapper>`_ will attempt to set
the scaling governor to "performance".  This alleviates the need to
manually set the governor.  Older versions of SLURM require the
desired governors to be explicitly listed in /etc/slurm.conf.  In
particular, SLURM 15.x requires the following option:

.. code-block:: bash

   CpuFreqGovernors=OnDemand,Performance


More information on the slurm.conf file can be found
`here <https://slurm.schedmd.com/slurm.conf.html>`__.
Non-SLURM systems must still set the scaling governor through some
other mechanism to ensure proper GEOPM behavior.  The following
command will set the governor to performance:

.. code-block:: bash

   echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor


See kernel documentation
`here <https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt>`__
for more information.


GEOPM Application Launch Wrapper
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM HPC Runtime package installs the command, "geopmlaunch".
This is a wrapper for the MPI launch commands like "srun", "aprun",
and "mpiexec" where the wrapper script enables the GEOPM runtime.  The
"geopmlaunch" command supports exactly the same command line interface
as the underlying launch command, but the wrapper extends the
interface with GEOPM specific options.  The "geopmlaunch" application
launches the primary compute application and the GEOPM control thread
on each compute node and manages the CPU affinity requirements for all
processes.  The wrapper is documented in the geopmlaunch(1) man page.

There are several underlying MPI application launchers that
"geopmlaunch" wrapper supports.  See the geopmlaunch(1) man page for
information on available launchers and how to select them.  If the
launch mechanism for your system is not supported, then affinity
requirements must be enforced by the user and all options to the GEOPM
runtime must be passed through environment variables.  Please consult
the geopm(7) man page for documentation of the environment variables
used by the GEOPM runtime that are otherwise controlled by the wrapper
script.

CPU Affinity Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM runtime requires that each MPI process of the application
under control is affinitized to distinct CPUs.  This is a strict
requirement for the runtime and must be enforced by the MPI launch
command.  When using the geopmlaunch wrapper described in the previous
section, these affinity requirements are handled by geopmlaunch unless
the --geopm-affinity-disable command line option is provided (see
geopmlaunch(1)).

While the GEOPM control thread connects to the application it will
automatically affinitize itself to the highest indexed core not used
by the application if the application is not affinitized to a CPU on
every core.  In the case where the application is utilizing all cores
of the system, the GEOPM control thread will be pinned to the highest
logical CPU.

There are many ways to launch an MPI application, and there is no
single uniform way of enforcing MPI rank CPU affinities across
different job launch mechanisms.  Additionally, OpenMP runtimes, which
are associated with the compiler choice, have different mechanisms for
affinitizing OpenMP threads within CPUs available to each MPI process.
To complicate things further the GEOPM control thread can be launched
as an application thread or a process that may be part of the primary
MPI application or a completely separate MPI application.  For these
reasons it is difficult to document how to correctly affinitize
processes in all configurations.  Please refer to your site
documentation about CPU affinity for the best solution on the system
you are using and consider extending the geopmlaunch wrapper to
support your system configuration (please see the CONTRIBUTING.rst file
for information about how to share these implementations with the
community).

Resource Manager Integration
----------------------------

The GEOPM HPC Runtime package can be integrated with a compute cluster
resource manager by modifying the resource manager daemon running on
the cluster compute nodes.  An example of integration with the SLURM
resource manager via a SPANK plugin can be found
`here <https://github.com/geopm/geopm-slurm>`__ and the implementation
reflects what is documented below.

Integration is achieved by modifying the daemon to make two
libgeopmd.so function calls prior to releasing resources to the
user (prologue), and one call after the resources have been reclaimed
from the user (epilogue).  In the prologue, the resource manager
compute node daemon calls:

.. code-block:: C

   geopm_pio_save_control()


which records into memory the value of all controls that can be
written through GEOPM (see geopm_pio_c(3)).  The second call made in
the prologue is:

.. code-block:: C

   geopm_agent_enforce_policy()


and this call (see geopm_agent_c(3)) enforces the configured policy
such as a power cap or a limit on CPU frequency by a one-time
adjustment of hardware settings.  In the epilogue, the resource
manager calls:

.. code-block:: C

   geopm_pio_restore_control()


which will set all GEOPM platform controls back to the values read in
the prologue.

The configuration of the policy enforced in the prologue is controlled
by the two files:

.. code-block:: bash

   /etc/geopm/environment-default.json
   /etc/geopm/environment-override.json


which are JSON objects mapping GEOPM environment variable strings to
string values.  The default configuration file controls values used
when a GEOPM variable is not set in the calling environment.  The
override configuration file enforces values for GEOPM variables
regardless of what is specified in the calling environment.  The list
of all GEOPM environment variables can be found in the geopm(7) man
page.  The two GEOPM environment variables used by
geopm_agent_enforce_policy() are "GEOPM_AGENT" and "GEOPM_POLICY".
Note that it is expected that /etc is mounted on a node-local file
system, so the geopm configuration files are typically part of the
compute node boot image.  Also note that the "GEOPM_POLICY" value
specifies a path to another JSON file which may be located on a
shared file system, and this second file controls the values enforced
(e.g. power cap value in Watts, or CPU frequency value in Hz).

When configuring a cluster to use GEOPM as the site-wide power
management solution, it is expected that one agent algorithm with one
policy will be applied to all compute nodes within a queue partition.
The system administrator selects the agent based on the site
requirements.  If the site requires that the average CPU power draw
per compute node remains under a cap across the system, then they
would choose the power_balancer agent (see
geopm_agent_power_balancer(7)).  If saving as much energy as possible
with a limited impact on performance is the site requirement, then the
energy_efficient agent would be selected (see
geopm_agent_energy_efficient(7)).  If the site would like to restrict
applications to run below a particular CPU frequency unless they are
executing a high priority optimized subroutine that has been granted
permission by the site administration to run at an elevated CPU
frequency, they would choose the frequency_map agent (see
geopm_agent_frequency_map(7)).  There is also the option for a site
specific custom agent plugin to be deployed.  In all of these use
cases, calling geopm_agent_enforce_policy() prior to releasing compute
node resources to the end user will enforce static limits to power or
CPU frequency, and these will impact all user applications.  In order
to leverage the dynamic runtime features of GEOPM, the user must
opt-in by launching their MPI application with the geopmlaunch(1)
command line tool.

The following example shows how a system administrator would configure
a system to use the power_balancer agent.  This use case will enforce
a static power limit for applications which do not use geopmlaunch(),
and will optimize power limits to balance performance when
geopmlaunch() is used.  First, the system administrator creates the
following JSON object in the boot image of the compute node in the
path "/etc/geopm/environment-override.json":

.. code-block:: json

   {"GEOPM_AGENT": "power_balancer",
    "GEOPM_POLICY": "/shared_fs/config/geopm_power_balancer.json"}


Note that the "POWER_PACKAGE_LIMIT_TOTAL" value controlling the limit
is specified in a secondary JSON file "geopm_power_balancer.json" that
may be located on a shared file system and can be created with the
geopmagent(1) command line tool.  Locating the policy file on the
shared file system enables the limit to be modified without changing
the compute node boot image.  Changing the policy value will impact
all subsequently launched GEOPM processes, but it will not change the
behavior of already running GEOPM control processes.
