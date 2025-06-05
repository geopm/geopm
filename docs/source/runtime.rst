User Guide for GEOPM Runtime
============================

The GEOPM Runtime is software designed to enhance energy efficiency of
applications through active hardware configuration. See
:ref:`Getting Started Guide <tutorial:|:straight_ruler:| Measure Performance>`
for information on how to begin using the GEOPM Runtime.

User Model
----------

The architecture is designed to provide a secure infrastructure to
support a wide range of tuning algorithms while solving three related
challenges:

1. Measuring Performance
^^^^^^^^^^^^^^^^^^^^^^^^

For hardware tuning algorithms, the first challenge involves generating a
durable estimate of application performance. In energy efficiency terms,
performance measurements are performance-to-power ratios, for example,
"perf per watt". Therefore, any dynamic hardware power control tuning
that aims for energy efficiency must formulate an estimate of application
performance. Without specific application feedback on the critical path,
these performance estimate might prove inaccurate, causing hardware
tuning algorithms to potentially disrupt application performance and lead
to elongated run times, resulting in higher energy costs per unit of work
than without the adaptive algorithm.

2. Hardware Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^

The second challenge arises from allowing hardware control algorithms to
be influenced by unprivileged user input (like application feedback), which
presents a security risk. Not having adequate checks can result in escalated
privileges, service denial, and impacts on other system users' quality of
service. GEOPM Service is created specifically to mitigate these issues.

3. Advanced Data Analysis
^^^^^^^^^^^^^^^^^^^^^^^^^

The third challenge is to provide a software development platform suitable for
control algorithms that can be securely deployed and use high-level languages
for data analysis. To effectively enhance energy efficiency for certain
applications, substantial software dependencies may be required. Control
algorithms might lean on optimization software like machine learning
packages or other numerical packages. The application being optimized could
include millions of lines of code, and there may be significant coupling
between the application and the control algorithm. Limiting the privileges
of the process running the control algorithm significantly reduces software
security audit requirements.

Introduction
------------

The GEOPM Runtime creates a bridge between GEOPM’s :doc:`application
instrumentation interfaces <geopm_prof.3>` and :doc:`platform
monitoring/control interfaces <geopm_pio.7>`.

By default, the GEOPM Runtime presents relationships between application
instrumentation and platform-monitoring interfaces in a *report*. For more
complex interactions, such as dynamic control of platform settings, different
GEOPM *agents* can be utilized.  For more information on user-facing GEOPM
Runtime launch options, please refer to :doc:`geopmlaunch(1)<geopmlaunch.1>`
documentation.

.. figure:: https://geopm.github.io/images/geopm-runtime-usage.svg
   :alt: An illustration of geopmlaunch running on 2 servers, generating a trace file per host, and one report across all hosts.
   :align: center

GEOPM *agents* can exploit this hierarchical control system to optimize
various objective functions. Examples include maximizing application
performance within a power limit (such as GEOPM :doc:`power_balancer
agent<geopm_agent_power_balancer.7>`) or decreasing energy consumption while
minimally affecting application performance. The control hierarchy root
can communicate with the system resource manager to extend the hierarchy
beyond the individual MPI application, thus facilitating multiple MPI jobs
and multiple-user system resource management.

The GEOPM Runtime package includes the libgeopm shared object library. GEOPM
comes with numerous command-line tools, each with dedicated manual pages. The
:doc:`geopmlaunch(1) <geopmlaunch.1>` command-line tool launches an MPI
application, enabling the GEOPM runtime to create a GEOPM Controller thread on
each compute node. The Controller loads plugins and runs the Agent algorithm
to manage the compute application. The :doc:`geopmlaunch(1)<geopmlaunch.1>`
command is featured in the geopmpy python package that is part of the GEOPM
installation. For more documentation and links, please visit the :doc:`GEOPM
overview man page <geopm.7>`.

GEOPM Runtime offers several built-in algorithms, each incorporated within an
"Agent" implementing the :doc:`geopm::Agent(3) <geopm::Agent.3>` class
interface. Developers can expand these algorithm features by creating an Agent
plugin. An implementation of this class can be dynamically loaded at runtime
by the GEOPM Controller. The Agent class determines what data is collected,
how control decisions are made, and how messages are exchanged between
Agents in the compute nodes' tree hierarchy. The GEOPM Service package,
which resides in the service directory of the GEOPM repository, provides
the PlatformIO interface which abstracts reading signals and writing controls
from the Agent within a compute node. This allows Agent implementations to
be ported to various hardware platforms without modification.

The libgeopm library can be called indirectly or directly within
MPI applications, enabling application feedback to aid control
decisions. Indirect calls are facilitated through GEOPM's integration with
MPI and OpenMP via their profiling decorators. Direct calls are made through
:doc:`geopm_prof(3)<geopm_prof.3>` or :doc:`geopm_fortran(3)<geopm_fortran.3>`
interfaces. The application can be better integrated with the GEOPM runtime
and controlled more accurately by marking up the compute application with
profiling information obtained through these interfaces.

Build Requirements
------------------

When building the GEOPM Runtime from source, additional requirements must
be met. Those uninterested in building the GEOPM Runtime can ignore these
requirements, or by providing the disable flag to the configure command
line, users may skip particular GEOPM Runtime features enabled by these
requirements.

The GEOPM Runtime provides optional support for MPI standards, Message Passing
Interface, version 2.2 or later.  Building the Runtime with MPI support will
add MPI related region information to the reports as well as enable Agents that
leverage the hierarchical communications tree (just the ``power_balancer`` at
the time of this writing).  If building for an HPC system, target the desired
site-specific MPI implementation.  Otherwise the Intel MPI implementation,
OpenHPC or Spack packaging systems, or OpenMPI binaries distributed with most
major Linux distributions satisfy this requirement. For RHEL and SLES Linux,
the requirement can be met by installing the ``openmpi-devel`` package version
1.7 or later, and ``libopenmpi-dev`` on Ubuntu.

* Install all requirements on **RHEL** or **CentOS**

  .. code-block:: bash

      yum install openmpi-devel elfutils libelf-devel


* Install all requirements on **SUSE**-based distributions

  .. code-block:: bash

      zypper install openmpi-devel elfutils libelf-devel


* Install all requirements on **Ubuntu** (as of 18.04.3 LTS)

  .. code-block:: bash

      apt install libtool automake libopenmpi-dev build-essential gfortran \
          libelf-dev python libsqlite3-dev


Requirements that can be avoided by removing features with configure option:

* Remove MPI compiler requirement
  ``--disable-mpi``

* Remove Fortran compiler requirement
  ``--disable-fortran``

* Remove elfutils library requirement
  ``--disable-ompt``

For details on how to use non-standard install locations for build
requirements see:

  .. code-block:: bash

    ./configure --help

This provides options, for example ``--with-<feature>``, to be used for
this purpose, such as ``--with-mpi-bin``.

Building the GEOPM Runtime
---------------------------

The best recommendation for constructing the GEOPM Runtime is to
follow the "developer build process" referenced in the :doc:`developer
guide<devel>`. This will enable GEOPM Service use and also provide the
latest developments in the GEOPM repository.

Run Requirements
----------------

Beyond the GEOPM Service, the GEOPM Runtime requires several additional
features at the time of use. Users uninterested in running the GEOPM Runtime
can ignore these requirements.

.. contents:: Categories of run requirements:
   :local:

BIOS Configuration
^^^^^^^^^^^^^^^^^^

If power governing or power balancing is the intended usage for GEOPM
deployment, an additional requirement involves configuring the BIOS to
support RAPL control. To make this check for BIOS support, execute the
following on a compute node:

.. code-block:: bash

    ./integration/tutorial/admin/00_test_prereqs.sh

If the script output includes:

.. code-block:: none

    WARNING: The lock bit for the PKG_POWER_LIMIT MSR is set.  The power_balancer
             and power_governor agents will not function properly until this is cleared.

Please enable RAPL in your BIOS, and if such an option doesn't exist please
contact your BIOS vendor to obtain a BIOS that supports RAPL.

For additional information, please contact the GEOPM team.

Linux Power Management
^^^^^^^^^^^^^^^^^^^^^^

It's important to note that other Linux mechanisms for CPU power management may
interfere with performance optimization objectives of GEOPM Agents. To achieve
optimal performance when deploying a GEOPM Agent that controls CPU frequency or
power limits it's recommended that the generic scaling governor ``userspace``
is selected while the GEOPM Agent is active.  If ``userspace`` is not available
on your system, it may be preferred to select ``performance`` mode while the
GEOPM Agent is active.

For more information, see the Linux Kernel documentation on
`generic scaling governors <https://docs.kernel.org/admin-guide/pm/cpufreq.html#generic-scaling-governors>`_.


Using Slurm to control the Linux CPU governor
"""""""""""""""""""""""""""""""""""""""""""""

When the ``userspace`` or ``performance`` mode is selected, the driver will not
interfere with GEOPM. On SLURM-based systems, the :ref:`GEOPM launch wrapper
<runtime:geopm application launch wrapper>` will attempt to set the scaling
governor to ``performance`` automatically, eliminating the need to manually set
the governor. On older versions of SLURM, the desired governors must be listed
explicitly in ``/etc/slurm.conf``. Specifically, SLURM 15.x requires the
following option:

.. code-block:: bash

   CpuFreqGovernors=OnDemand,Performance

For more on SLURM configuration, please see the `slurm.conf manual
<https://slurm.schedmd.com/slurm.conf.html>`_. On non-SLURM systems, the
scaling governor should still be manually set through some other mechanism
to ensure proper GEOPM behavior. The following command will set the governor
to ``userspace``:

.. code-block:: bash

   echo userspace | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

Launching the GEOPM Runtime
^^^^^^^^^^^^^^^^^^^^^^^^^^^



GEOPM Application Launch Wrapper
""""""""""""""""""""""""""""""""

The GEOPM Runtime package installs the ``geopmlaunch`` command. This command is
a wrapper for MPI launch commands such as ``srun``, ``aprun``, and
``mpiexec.hydra``, where the wrapper script enables the GEOPM runtime. The
``geopmlaunch`` command supports the same command-line interface as the
underlying launch command, while extending the interface with GEOPM-specific
options. The ``geopmlaunch`` application launches the primary compute
application and the GEOPM control thread on each compute node.  This wrapper is
documented in the :doc:`geopmlaunch(1)<geopmlaunch.1>` man page.

If your system's launch mechanism is not supported then options to the GEOPM
runtime must be passed through environment variables and some features of the
``geopmlaunch`` command (such as process CPU affinity management) will not be
available.  Please consult the :doc:`geopm(7)<geopm.7>` man page for
documentation of the environment variables used by the GEOPM runtime that would
otherwise be controlled by the wrapper script and see :ref:`tutorial:Profiling Applications without ``geopmlaunch```
for details.

CPU Affinity Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^

When using the ``geopmlaunch`` wrapper, the user may optionally provide the
``--geopm-affinity-enable`` command-line option (see
:doc:`geopmlaunch(1)<geopmlaunch.1>`). This will enable hardware metrics to be
more accurately measured on a per-application-region basis by restricting
process migration.

While the GEOPM control thread connects to the application it will
automatically affinitize itself to the highest indexed core not used by the
application if the application is not affinitized to a CPU on every core. If
the application is using all cores of the system, the GEOPM control thread
will be pinned to the highest logical CPU.

Configuring System-wide Runtime Policy
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two files may be used to control system-wide default and override value for the
GEOPM Runtime parameters.

.. code-block:: bash

   /etc/geopm/environment-default.json
   /etc/geopm/environment-override.json

These files contain JSON objects that map GEOPM environment variables to
their respective values. The default configuration holds values for any
unset GEOPM variable in the calling environment. Meanwhile, the override
configuration enforces values, overriding the calling environment's
specifications. A comprehensive list of GEOPM environment variables is
available in the geopm(7) man page. The two primary environment variables
that ``geopm_agent_enforce_policy()`` utilizes are ``GEOPM_AGENT`` and
``GEOPM_POLICY``. It's important to note that ``/etc`` should be mounted on a
local node file system, meaning the GEOPM configuration files typically become
part of the compute node's boot image. The ``GEOPM_POLICY`` value directs
to another JSON file, possibly located on a shared file system, dictating
the enforced values (like the power cap in Watts or CPU frequency in Hz).

For GEOPM's integration as the universal power management solution for
a cluster, it's usual for a single agent algorithm with one policy to be
applied across all compute nodes within a partition. The choice of agent
rests upon the site's needs. For instance, if the aim is to keep the average
CPU power draw for each node below a specific cap, the :doc:`power_balancer
agent <geopm_agent_power_balancer.7>` is ideal. However, if the goal is to
limit application CPU frequencies with exceptions for specific high-priority
processes, the :doc:`frequency_map agent <geopm_agent_frequency_map.7>`
is the best fit. Sites can also deploy a custom agent plugin. In every
scenario, invoking ``geopm_agent_enforce_policy()`` before releasing
compute resources ensures the enforcement of static limits impacting all
user applications. For dynamic runtime features, users must initiate their
MPI application using the :doc:`geopmlaunch(1) <geopmlaunch.1>` tool.

To illustrate, if a system administrator wants to use the ``power_balancer``
agent, the process would involve setting a static power cap for
apps not utilizing ``geopmlaunch``, while optimizing power caps for
performance when ``geopmlaunch`` is in use. The administrator would
install the following JSON object in the compute node's boot image at
``/etc/geopm/environment-override.json``:

.. code-block:: json

   {"GEOPM_AGENT": "power_balancer",
    "GEOPM_POLICY": "/shared_fs/config/geopm_power_balancer.json"}

The controlling value, ``CPU_POWER_LIMIT``, is defined in a separate
"geopm_power_balancer.json" file that could reside on a shared file
system. This file can be generated using the :doc:`geopmagent(1)
<geopmagent.1>` tool. By placing the policy file on a shared file system,
you allow modifications to the limit without affecting the compute node
boot image. Changing the policy value affects all new GEOPM processes but
leaves running GEOPM processes untouched.
