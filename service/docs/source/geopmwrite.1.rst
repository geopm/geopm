geopmwrite(1) -- modify platform state
======================================






SYNOPSIS
--------

PRINT ALL CONTROL NAMES

.. code-block:: bash

    geopmwrite


PRINT DOMAIN NAME AND SIZE

.. code-block:: bash

    geopmwrite --domain


PRINT CONTROL DESCRIPTION

.. code-block:: bash

    geopmwrite --info CONTROL_NAME


PRINT ALL CONTROL DESCRIPTIONS

.. code-block:: bash

    geopmwrite --info-all


WRITE CONTROL

.. code-block:: bash

    geopmwrite CONTROL_NAME DOMAIN_TYPE DOMAIN_INDEX VALUE


CREATE CACHE

.. code-block:: bash

    geopmwrite --cache


GET HELP OR VERSION

.. code-block:: bash

    geopmwrite --help
    geopmwrite --version

DESCRIPTION
-----------

Provides a command line interface to PlatformIO and PlatformTopo.
This tool can be used to write hardware/OS state through
high-level control aliases and query other information
about the platform such as the type and number of hardware domains.
Details of the hardware domains can also be inferred from the output
of `lscpu(1) <http://man7.org/linux/man-pages/man1/lscpu.1.html>`_.

When run without any arguments, the default behavior is to print a
summary of available controls.

To write a specific control, ``geopmwrite`` should be run with four
arguments.  CONTROL_NAME is the name of the control of interest.
DOMAIN_TYPE is hardware domain to which the control should be applied.
DOMAIN_INDEX is used to indicate which instance of the domain to write
to; indexing starts from 0 and goes up to the domain size - 1.  VALUE
is the floating-point number in SI units that the control will be
adjusted to.  If the DOMAIN_TYPE is a larger containing domain than
that of the control, the same value will be applied to every contained
subdomain.  Refer to the domain hierarchy described in
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ for the descriptions of the domains and how
they are contained within one another.

| ``board`` - domain for node-wide signals and controls
| ++ ``package`` - socket
| ++++ ``core`` - physical core
| ++++++ ``cpu`` - Linux logical CPU
| ++++ ``package_integrated_memory`` - on-package memory
| ++++ ``package_integrated_nic`` - NIC within the package
| ++++ ``package_integrated_gpu`` - domain for GPUs within the package
| ++ ``memory`` - other memory outside the package
| ++ ``nic`` - NIC attached to the board
| ++ ``gpu`` - domain for GPUs on the board

This utility can be used to create a geopm::PlatformTopo cache file in
the tmpfs.  When this file is not present the `geopmread(1) <geopmread.1.html>`_\ ,
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
  Print description of the provided CONTROL_NAME.

*
  ``-I``\ , ``--info-all``\ :
  Print a list of all available controls with their descriptions,
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

   $ geopmwrite --domain
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


List all available controls on the system with domain type and number:

.. code-block::

   $ geopmwrite
   CPU_FREQUENCY_CONTROL
   POWER_PACKAGE_LIMIT


Show the description for a control:

.. code-block::

   $ geopmwrite --info CPU_FREQUENCY_CONTROL
   CPU_FREQUENCY_CONTROL: Set processor frequency


Show domain type for POWER_PACKAGE control:

.. code-block::

   $ geopmwrite --domain POWER_PACKAGE
   package


Set the frequency of CPU 2 to 1.9 GHz:

.. code-block::

   $ geopmwrite CPU_FREQUENCY_CONTROL cpu 2 1.9e9
   $ geopmread CPU_FREQUENCY_CONTROL cpu 2
   1.9e9


Set all CPUs on package 0 to 1.5 GHz (cpu 1 is on package 0):

.. code-block::

   $ geopmwrite CPU_FREQUENCY_CONTROL package 0 1.5e9
   $ geopmread CPU_FREQUENCY_CONTROL cpu 1
   1.5e9


SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_\ ,
`lscpu(1) <http://man7.org/linux/man-pages/man1/lscpu.1.html>`_
