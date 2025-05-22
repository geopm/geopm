geopmctl(1) -- GEOPM runtime control application
================================================

Synopsis
--------

.. code-block::

   geopmctl [--help] [--version]

Description
-----------

The ``geopmctl`` application runs concurrently with a computational MPI
application to manage power settings on compute nodes allocated to the
computation MPI application.

CPU Affinity and cgroup Constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When ``geopmctl`` runs in an environment with cgroup cpuset restrictions (such as
in containerized environments or with certain resource managers), it will respect
these constraints. ``geopmctl`` will only pin its thread to a CPU that is available
within the current cgroup cpuset. If the cgroup restricts the available CPUs,
``geopmctl`` will automatically detect these constraints and limit its CPU affinity
within the allowed set. This behavior ensures proper operation in containerized
environments and when running under resource managers that enforce CPU constraints.

Options
-------
--help     Print brief summary of the command line usage information, then exit.
--version  Print version of GEOPM to standard output, then exit.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_agent(3) <geopm_agent.3>`,
:doc:`geopm_ctl(3) <geopm_ctl.3>`,
:doc:`geopmagent(1) <geopmagent.1>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
