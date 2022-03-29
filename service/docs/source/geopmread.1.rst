geopmread(1) -- query platform information
==========================================


SYNOPSIS
--------

PRINT ALL SIGNAL NAMES

.. code-block:: bash

    geopmread


PRINT DOMAIN NAME AND SIZE

.. code-block:: bash

    geopmread --domain


PRINT SIGNAL DESCRIPTION

.. code-block:: bash

    geopmread --info SIGNAL_NAME


PRINT ALL SIGNAL DESCRIPTIONS

.. code-block:: bash

    geopmread --info-all


READ SIGNAL

.. code-block:: bash

    geopmread SIGNAL_NAME DOMAIN_TYPE DOMAIN_INDEX


CREATE CACHE

.. code-block:: bash

    geopmread --cache


GET HELP OR VERSION

.. code-block:: bash

    geopmread --help
    geopmread --version

DESCRIPTION
-----------

Provides a command line interface to PlatformIO and PlatformTopo.
This tool can be used to read hardware/OS state through high-level
signal aliases and query other information about the platform such as
the type and number of hardware domains.  Details of the hardware
domains can also be inferred from the output of `lscpu(1) <http://man7.org/linux/man-pages/man1/lscpu.1.html>`_.

When run without any arguments, the default behavior is to print a
summary of available signals.  Signal names ending in # represent the
raw bits of MSRs without any interpretation and will be printed in
hex.  This feature is mainly useful for debugging; for profiling, the
signal aliases should be preferred.

To read a specific signal, ``geopmread`` should be run with the three
arguments.  SIGNAL_NAME is the name of the signal of interest.
DOMAIN_TYPE is the hardware domain for which this signal should be
read.  The domain type should be a lowercase string from the list shown
by ``--domain``.  DOMAIN_INDEX is used to indicate which instance of the domain
to read; indexing starts from 0 and goes up to the domain size - 1.
Values read for signals are in SI units.  Note that the domain can be
the native domain of the signal (as shown in the summary) or any
larger containing domain, in which case the signal value will be
aggregated into a single value for the larger domain.  Refer to the
domain hierarchy described in `geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ for the
descriptions of the domains and how they are contained within one
another.

The aggregation functions used for each signal are described in
`geopm(7) <geopm.7.html>`_ under the description for ``GEOPM_TRACE_SIGNALS``.  The
same functions are used to aggregate signals in the trace into the
board domain.  Note that not all signals have aggregation functions,
and if a signal is not readable at board domain, it cannot be printed
in the trace.

This utility can be used to create a geopm::PlatformTopo cache file in
the tmpfs.  When this file is not present `geopmread(1) <geopmread.1.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ , `geopmctl(1) <geopmctl.1.html>`_ and `geopmlaunch(1) <geopmlaunch.1.html>`_ will
**popen(1)** a subprocess which provides the platform topology
information.  This subprocess will not be created if the cache file
exists.  See the ``--cache`` option below for more information.

OPTIONS
-------


*
  ``-d``\ , ``--domain``\ :
  Print a list of all domains on the system.

*
  ``-i``\ , ``--info``\ :
  Print description of the provided SIGNAL_NAME.

*
  ``-I``\ , ``--info-all``\ :
  Print a list of all available signals with their descriptions,
  if any.

*
  ``-c``\ , ``--cache``\ :
  Create a cache file for the geopm::PlatformTopo object if one does
  not exist.  File permissions of the cache file are set to
  "-rw-rw-rw-", i.e. 666. The path for the cache file is
  "/tmp/geopm-topo-cache".  If the file exists no operation will be
  performed.  To force the creation of a new cache file, remove the
  existing cache file prior to executing this command.

*
  ``-h``\ , ``--help``\ :
  Print brief summary of the command line usage information,
  then exit.

*
  ``-v``\ , ``--version``\ :
  Print version of `geopm(7) <geopm.7.html>`_ to standard output, then exit.

EXAMPLES
--------

List domains and size:

.. code-block::

   $ geopmread --domain
   board                1
   package              2
   core                 64
   cpu                  128
   board_memory         2
   package_memory       1
   board_nic            1
   package_nic          0
   board_accelerator    0
   package_accelerator  0


List all available signals on the system:

.. code-block::

   $ geopmread
   TIME
   ENERGY_PACKAGE
   ENERGY_DRAM
   POWER_PACKAGE
   POWER_DRAM
   CPU_FREQUENCY_STATUS


Show the description for a signal:

.. code-block::

   $ geopmread --info TIME
   TIME: Time in seconds since the IOGroup load.


Show domain type for ENERGY_DRAM signal:

.. code-block::

   $ geopmread --domain ENERGY_DRAM
   board_memory


Read the current energy for package 1:

.. code-block::

   $ geopmread ENERGY_PACKAGE package 1
   34567


Read the total energy for both packages:

.. code-block::

   $ geopmread ENERGY_PACKAGE board 0
   56789


SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`lscpu(1) <http://man7.org/linux/man-pages/man1/lscpu.1.html>`_
