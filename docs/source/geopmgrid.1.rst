geopmgrid(1) -- define control parameter grids
==============================================

Synopsis
--------

.. code-block:: bash

   usage: geopmgrid [-h] [--cpu-frequency CPU_FREQUENCY_DOMAIN]
                    [--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN]
                    [--cpu-power CPU_POWER_DOMAIN]
                    [--gpu-frequency GPU_FREQUENCY_DOMAIN]
                    [--gpu-power GPU_POWER_DOMAIN]
                    [--board-power BOARD_POWER_DOMAIN]
                    [--coordinate COORDINATE [COORDINATE ...] |
                     --coordinate-file COORDINATE_FILE |
                     --coordinate-range]
                    [--write]

Display grid dimensions
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --cpu-frequency package --coordinate-range

Generate configuration for a grid point
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --cpu-frequency package --coordinate 0

Generate configuration from file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    echo "0 1" > coordinate.txt
    geopmgrid --cpu-frequency package --cpu-power board --coordinate-file coordinate.txt

Apply configuration to platform
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --cpu-frequency package --coordinate 0 --write

Get Help
~~~~~~~~

.. code-block:: bash

    geopmgrid -h
    geopmgrid --help


Description
-----------

Command line interface for defining and exploring N-dimensional control
parameter grids for GEOPM. The tool allows users to define parameter spaces
for various control knobs including CPU frequency, CPU power limits, GPU
frequency, and GPU power limits across different platform domains.

The grid tool operates in several modes:

1. **Grid exploration**: Use ``--coordinate-range`` to display the dimensions
   and size of the defined parameter grid.

2. **Configuration generation**: Specify a coordinate within the grid using
   ``--coordinate`` or ``--coordinate-file`` to generate geopmwrite
   configuration commands for that specific parameter combination.

3. **Direct application**: Add the ``--write`` flag to directly apply the
   configuration to the platform using the GEOPM service.

The tool integrates with the GEOPM Platform I/O (PIO) interface to determine
available parameter ranges, step sizes, and valid domains for each control type.


Options
-------

Control Parameters
~~~~~~~~~~~~~~~~~~

--cpu-frequency CPU_FREQUENCY_DOMAIN  .. _cpu-frequency option:

    Define a grid dimension over CPU_FREQUENCY_MAX_CONTROL for the specified
    domain. Valid domains include 'package', 'core', and others depending on
    platform capabilities.

--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN  .. _cpu-uncore-frequency option:

    Define a grid dimension over CPU_UNCORE_FREQUENCY_MAX_CONTROL for the
    specified domain. Typically used with 'package' domain.

--cpu-power CPU_POWER_DOMAIN  .. _cpu-power option:

    Define a grid dimension over CPU_POWER_LIMIT_CONTROL for the specified
    domain. Commonly used with 'package' or 'board' domains.

--gpu-frequency GPU_FREQUENCY_DOMAIN  .. _gpu-frequency option:

    Define a grid dimension over GPU_CORE_FREQUENCY_MAX_CONTROL for the
    specified domain. Used with 'gpu' or 'gpu_chip' domains.

--gpu-power GPU_POWER_DOMAIN  .. _gpu-power option:

    Define a grid dimension over GPU_POWER_LIMIT_CONTROL for the specified
    domain. The tool automatically detects whether to use Intel Level Zero
    or NVIDIA NVML interfaces.

--board-power BOARD_POWER_DOMAIN  .. _board-power option:

    Define a grid dimension over BOARD_POWER_LIMIT_CONTROL for the specified
    domain. Typically used with 'board' domain.

Grid Navigation
~~~~~~~~~~~~~~~

--coordinate COORDINATE [COORDINATE ...]  .. _coordinate option:

    Specify a coordinate within the grid to generate configuration for.
    The number of coordinates must match the number of defined grid dimensions.
    Each coordinate is an integer index into the corresponding dimension's
    parameter range.

--coordinate-file COORDINATE_FILE  .. _coordinate-file option:

    Read grid coordinates from a file. The file should contain space-separated
    integer coordinates corresponding to each grid dimension.

--coordinate-range  .. _coordinate-range option:

    Display the size of each dimension in the defined grid. This shows the
    number of available parameter values for each control type and domain.

Actions
~~~~~~~

--write  .. _write option:

    Apply the configuration to the platform using the GEOPM service. Requires
    either ``--coordinate`` or ``--coordinate-file`` to specify which grid
    point to apply. This option pushes the control values directly to the
    platform hardware.

-h, --help  .. _help option:

    Print help message and exit.


Examples
--------

Exploring CPU frequency grid
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define a grid over CPU frequency for all packages and display its dimensions:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate-range
   Dimension 0: 21 points

This shows that the CPU frequency grid has 21 available frequency settings
across all package domains.

Generating configuration commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Generate geopmwrite commands for a specific grid point:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate 10
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 0 2400000000
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 1 2400000000

This generates commands to set CPU frequency to 2.4 GHz for all packages.

Multi-dimensional grid exploration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define a 2D grid over CPU frequency and power limit:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate-range
   Dimension 0: 21 points
   Dimension 1: 150 points

   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate 10 75
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 0 2400000000
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 1 2400000000
   geopmwrite CPU_POWER_LIMIT_CONTROL package 0 125000000
   geopmwrite CPU_POWER_LIMIT_CONTROL package 1 125000000

This creates a 2D grid with 21×150 = 3,150 possible configurations and
generates commands for coordinate (10, 75).

Using coordinate files
~~~~~~~~~~~~~~~~~~~~~~

Store coordinates in a file for repeated use:

.. code-block:: shell-session

   $ echo "10 75" > my_config.coord
   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate-file my_config.coord
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 0 2400000000
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 1 2400000000
   geopmwrite CPU_POWER_LIMIT_CONTROL package 0 125000000
   geopmwrite CPU_POWER_LIMIT_CONTROL package 1 125000000

Direct platform configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Apply a configuration directly to the platform:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate 15 --write

This immediately applies the configuration to the platform hardware without
printing the intermediate geopmwrite commands.

GPU control grids
~~~~~~~~~~~~~~~~~

Define grids for GPU controls:

.. code-block:: shell-session

   $ geopmgrid --gpu-frequency gpu --gpu-power gpu --coordinate-range
   Dimension 0: 15 points
   Dimension 1: 50 points

   $ geopmgrid --gpu-frequency gpu --gpu-power gpu --coordinate 7 25
   geopmwrite GPU_CORE_FREQUENCY_MAX_CONTROL gpu 0 1200000000
   geopmwrite GPU_POWER_LIMIT_CONTROL gpu 0 200000000

Complete system grid
~~~~~~~~~~~~~~~~~~~~

Define a comprehensive grid covering multiple subsystems:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --cpu-power package \
             --gpu-frequency gpu --board-power board \
             --coordinate-range
   Dimension 0: 21 points
   Dimension 1: 150 points
   Dimension 2: 15 points
   Dimension 3: 100 points

This creates a 4D grid with 21×150×15×100 = 4,725,000 possible configurations
for comprehensive system optimization.


Domain Types
------------

The tool supports various domain types depending on the platform and control type:

**CPU Controls:**
- ``package``: CPU package/socket level
- ``core``: Individual CPU core level

**GPU Controls:**
- ``gpu``: GPU device level
- ``gpu_chip``: GPU chip level

**System Controls:**
- ``board``: System board level

Use ``geopmread --domain`` to list all available domains on your platform.


Grid Generation
---------------

The tool automatically determines parameter ranges using the GEOPM PIO interface:

**Minimum Values:** Read from ``*_MIN_AVAIL`` signals where available,
otherwise use safe default values.

**Maximum Values:** Read from ``*_MAX_AVAIL`` signals or current limit controls.

**Step Sizes:** Read from ``*_STEP`` signals or use appropriate defaults
based on control type.

The grid covers the full available range with uniform step sizes, ensuring
all generated coordinates correspond to valid hardware settings.


Error Handling
--------------

The tool validates configurations before generation:

- **Invalid domains:** Reports if a domain type is not supported for the
  specified control.
- **No available settings:** Reports if a control has no valid parameter
  range on the platform.
- **Coordinate out of bounds:** Reports if specified coordinates exceed
  the grid dimensions.
- **Missing coordinates:** Reports if ``--write`` is used without specifying
  a coordinate.


Integration
-----------

The ``geopmgrid`` tool integrates seamlessly with other GEOPM utilities:

**With geopmwrite:** Generated commands can be piped directly to geopmwrite
or saved to configuration files.

**With geopmopt:** The optimizer uses ControlGrid internally to define
parameter spaces for Bayesian optimization.

**With geopmsession:** Configurations can be applied before launching
monitoring sessions to study parameter effects.

**With geopmlaunch:** Grid configurations can be used in conjunction with
application launches for parameter sweeps.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmopt(1) <geopmopt.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
