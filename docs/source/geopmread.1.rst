geopmread(1) -- query platform information
==========================================

Synopsis
--------

Print All Signal Names
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    geopmread

Print Domain Name And Size
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    geopmread --domain

Print Signal Description
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    geopmread --info SIGNAL_NAME

Print All Signal Descriptions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    geopmread --info-all

Read Signal
^^^^^^^^^^^

.. code-block:: bash

    geopmread SIGNAL_NAME DOMAIN_TYPE DOMAIN_INDEX

Create Cache
^^^^^^^^^^^^

.. code-block:: bash

    geopmread --cache

Get Help Or Version
^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    geopmread --help
    geopmread --version

Description
-----------

Provides a command line interface to ``PlatformIO`` and ``PlatformTopo``.
This tool can be used to read hardware/OS state through high-level
signal aliases and query other information about the platform such as
the type and number of hardware domains.  Details of the hardware
domains can also be inferred from the output of `lscpu(1) <https://man7.org/linux/man-pages/man1/lscpu.1.html>`_.

When run without any arguments, the default behavior is to print a
summary of available signals.  Signal names ending in ``#`` represent the
raw bits of MSRs without any interpretation and will be printed in
hex.  This feature is mainly useful for debugging; for profiling, the
signal aliases should be preferred.

To read a specific signal, ``geopmread`` should be run with the three
arguments.  ``SIGNAL_NAME`` is the name of the signal of interest.
``DOMAIN_TYPE`` is the hardware domain for which this signal should be
read.  The domain type should be a lowercase string from the list shown
by ``--domain``.  ``DOMAIN_INDEX`` is used to indicate which instance of the domain
to read; indexing starts from 0 and goes up to the domain size - 1.
Values read for signals are in SI units.  Note that the domain can be
the native domain of the signal (as shown in the summary) or any
larger containing domain, in which case the signal value will be
aggregated into a single value for the larger domain.  Refer to the
domain hierarchy described in :doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>` for the
descriptions of the domains and how they are contained within one
another.

The aggregation functions used for each signal are described in
:doc:`geopm(7) <geopm.7>` under the description for ``GEOPM_TRACE_SIGNALS``.  The
same functions are used to aggregate signals in the trace into the
board domain.  Note that not all signals have aggregation functions,
and if a signal is not readable at board domain, it cannot be printed
in the trace.

This utility can be used to create a ``geopm::PlatformTopo`` cache file in
the tmpfs.  When this file is not present :doc:`geopmread(1) <geopmread.1>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ , :doc:`geopmctl(1) <geopmctl.1>` and :doc:`geopmlaunch(1) <geopmlaunch.1>` will
`popen(3) <https://man7.org/linux/man-pages/man3/popen.3.html>`_ a subprocess which provides the platform topology
information.  This subprocess will not be created if the cache file
exists.  See the ``--cache`` option below for more information.

Options
-------
-d, --domain    Print a list of all domains on the system.
-i, --info      Print description of the provided ``SIGNAL_NAME``.
-I, --info-all  Print a list of all available signals with their descriptions,
                if any.
-c, --cache     Create a cache file for the ``geopm::PlatformTopo`` object if one
                does not exist or if the existing cache is from a previous boot
                cycle.  If a privileged user requests this option (e.g. root or
                if invoked with sudo) the file path will be
                ``/run/geopm/geopm-topo-cache``. If a non-privileged user requests
                this option the file path will be ``/tmp/geopm-topo-cache-<UID>``.
                In either case, the permissions will be ``-rw-------``, i.e.
                **600**.  If the file exists from the current boot cycle and
                has the proper permissions no operation will be performed.  To
                force the creation of a new cache file, remove the existing
                cache file prior to executing this command.  Additionally when
                this command is executed any existing GEOPM HPC Runtime shared
                memory keys owned by the user running the command will be
                deleted.
-h, --help      Print brief summary of the command line usage information, then
                exit.
-v, --version   Print version of :doc:`geopm(7) <geopm.7>` to standard output, then
                exit.

Examples
--------

List domains and size:

.. code-block::

   $ geopmread --domain
   board                1
   package              2
   core                 64
   cpu                  128
   memory         2
   package_integrated_memory       1
   nic            1
   package_integrated_nic          0
   gpu    0
   package_integrated_gpu  0

List all available signals on the system:

.. code-block::

   $ geopmread
   TIME
   CPU_ENERGY
   DRAM_ENERGY
   CPU_POWER
   DRAM_POWER
   CPU_FREQUENCY_STATUS

Show the description for a signal:

.. code-block::

   $ geopmread --info TIME
   TIME:
       description: Time since the start of application profiling.
       alias_for: TIME::ELAPSED
       units: seconds
       aggregation: select_first
       domain: cpu
       iogroup: TimeIOGroup

Show domain type for DRAM_ENERGY signal:

.. code-block::

   $ geopmread --domain DRAM_ENERGY
   memory

Read the current energy for package 1:

.. code-block::

   $ geopmread CPU_ENERGY package 1
   34567

Read the total energy for both packages:

.. code-block::

   $ geopmread CPU_ENERGY board 0
   56789

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
`lscpu(1) <https://man7.org/linux/man-pages/man1/lscpu.1.html>`_
