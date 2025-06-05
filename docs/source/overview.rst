=================
 Getting Started
=================

GEOPM (Global Extensible Open Power Manager) is a software framework that
enables users and administrators to **safely and securely modify hardware
settings for the duration of a process session**. This unique capability allows
for dynamic, user-driven power and performance tuning—enabling energy efficiency
and workload optimization that is not possible with traditional system tools.

GEOPM is deployed in production on large-scale systems (e.g., Aurora) and is
used by HPC centers and cloud providers to help users and administrators meet
power, performance, and sustainability goals.

.. note::

    For a quick reference on GEOPM commands, see:

      - :doc:`geopmread.1`
      - :doc:`geopmwrite.1`
      - :doc:`geopmsession.1`

----

|:zap:| Why Use GEOPM?
======================

GEOPM provides a **secure userspace interface** for modifying hardware controls
(such as CPU power limits and frequencies) for the duration of a user session or
job. This means:

- **Users can safely adjust hardware settings** (e.g., power limits,
  frequencies) for their own jobs, without affecting other users or the system
  as a whole.

- **All changes are automatically reverted** at the end of the session, ensuring
  system integrity and preventing persistent misconfiguration.

- **Administrators retain fine-grained control** over which users and groups can
  access which hardware features, with full auditability.

This capability is especially valuable for:

- **HPC and cloud users** who want to optimize performance per watt for their
  workloads.

- **System administrators** who want to enable user-driven tuning while
  maintaining security and stability.

- **Sustainability initiatives** that require power capping or energy-aware
  scheduling.

----

|:rocket:| Example: Power Management for Memory-Bound HPC Workloads
===================================================================

GEOPM is particularly effective for memory-bound workloads, where reducing CPU
power limits can lower energy consumption with minimal impact on performance.


**Key Takeaways:**

- For memory-bound workloads, reducing CPU package power limit via GEOPM can
  significantly lower system power consumption with minimal impact on
  performance, up to a certain threshold.

- The optimal setting ("sweet spot") depends on the workload behavior and user's
  preference for balancing energy efficiency and performance.

- This demonstrates the practical value of exposing user-controllable power
  management through GEOPM, enabling end users to optimize for performance per
  watt based on workload characteristics.

----

|:wrench:| How to Set Power Limits with GEOPM
=============================================

GEOPM provides command-line tools to read and write hardware settings. The most
common use case is to set a power limit or frequency limit for your job or session.

**Example: Setting and Reading CPU Power Limit**

.. code-block:: bash

    # Read the default power limit for package 0
    geopmread CPU_POWER_LIMIT_DEFAULT package 0

    # Reduce the power limit by 100W and apply it
    NEW_LIMIT=$(( $(geopmread CPU_POWER_LIMIT_DEFAULT package 0) - 100 ))
    geopmwrite CPU_POWER_LIMIT_CONTROL package 0 $NEW_LIMIT

    # Verify the new power limit
    geopmread CPU_POWER_LIMIT_CONTROL package 0

**Example: Measuring Power Consumption During a Job**

.. code-block:: bash

    # Read initial energy
    ENERGY_BEGIN=$(geopmread CPU_ENERGY package 0)
    sleep 10
    ENERGY_END=$(geopmread CPU_ENERGY package 0)
    python3 -c "print(f'Package 0 energy: {float($ENERGY_END) - float($ENERGY_BEGIN)} joules')"

**Example: Detailed Power Monitoring with geopmsession**

.. code-block:: bash

    printf "TIME board 0\nCPU_ENERGY * *" | geopmsession -r- -p 0.01 -- sleep 1

This will output a time series of energy readings for each package during the
execution of the `sleep 1` command.

For more details, see the `geopmsession` documentation:
https://geopm.github.io/geopmsession.1.html#reading-signals-during-a-job-execution

----

|:shield:| Security and Safe Access
===================================

GEOPM is designed to be secure and robust for multi-user environments:

- **Session-based control:** All hardware changes are scoped to the user's
  session and reverted at the end.

- **Access control:** Administrators can grant or restrict access to specific
  hardware features per user or group.

- **Auditability:** All changes are logged and can be audited.

- **No persistent changes:** Hardware is always restored to a safe state after
  each session.

For more information, see the :doc:`security` guide.

----

|:computer:| Install GEOPM
==========================

To check if GEOPM is installed on your system you may try querying the command
line interfaces for the GEOPM version.  For example:

.. code-block:: bash

   geopmread --version

Follow the :doc:`install` for details about how to install GEOPM if
this command errors with ``command not found``.

----

|:card_file_box:| Platform Topology
===================================

GEOPM provides tools to discover the hardware topology of your system, including
domains such as board, package, core, cpu, memory, and more. While this is not
unique to GEOPM, it is essential for advanced tuning and control.

.. _topo-diagram:
.. figure:: https://geopm.github.io/images/platform-topo-diagram.svg
    :alt: Topology Encapsulation Diagram
    :align: center

.. note::

   For more information on the domain types and topology APIs, see:

   - :ref:`geopm_topo.3:Domain Types`
   - :ref:`Python API <geopmdpy.7:geopmdpy.topo>`
   - :doc:`C API <geopm_topo.3>`
   - :doc:`C++ API <geopm::PlatformTopo.3>`

----

|:microscope:| Reading Telemetry
================================

GEOPM enables users to read a wide variety of hardware telemetry signals (e.g.,
frequency, power, energy, temperature) at different domains. This is useful for
real-time monitoring and analysis, but the unique value of GEOPM is in its safe,
session-based control features.

.. note::

   For more information, see the documentation for

     - :doc:`geopmread <geopmread.1>`
     - :doc:`geopmsession <geopmsession.1>`
     - :ref:`Python API <geopmdpy.7:geopmdpy.pio>`
     - :doc:`C API <geopm_pio.3>`
     - :doc:`C++ API <geopm::PlatformIO.3>`

----

|:gear:| Enact Hardware-based Settings
======================================

GEOPM enables users to write hardware controls (e.g., set power limits,
frequencies) safely and securely. See the section below for examples and the
:doc:`geopmwrite <geopmwrite.1>` documentation for details.

.. note::

   For more information, see the documentation for

     - :doc:`geopmwrite <geopmwrite.1>`
     - :ref:`Python API <geopmdpy.7:geopmdpy.pio>`
     - :doc:`C API <geopm_pio.3>`
     - :doc:`C++ API <geopm::PlatformIO.3>`

----

|:straight_ruler:| Measure Performance
======================================

GEOPM provides runtime tools for collecting telemetry and profiling
applications, including integration with MPI and OpenMP.
