geopmgrid(1) -- define control parameter grids
==============================================

Synopsis
--------

.. code-block:: bash

    usage: geopmgrid [-h]
                     [--cpu-frequency CPU_FREQUENCY_DOMAIN]
                     [--cpu-frequency-min CPU_FREQUENCY_MIN]
                     [--cpu-frequency-max CPU_FREQUENCY_MAX]
                     [--cpu-frequency-step CPU_FREQUENCY_STEP]
                     [--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN]
                     [--cpu-uncore-frequency-min CPU_UNCORE_FREQUENCY_MIN]
                     [--cpu-uncore-frequency-max CPU_UNCORE_FREQUENCY_MAX]
                     [--cpu-uncore-frequency-step CPU_UNCORE_FREQUENCY_STEP]
                     [--cpu-power CPU_POWER_DOMAIN]
                     [--cpu-power-min CPU_POWER_MIN]
                     [--cpu-power-max CPU_POWER_MAX]
                     [--cpu-power-step CPU_POWER_STEP]
                     [--gpu-frequency GPU_FREQUENCY_DOMAIN]
                     [--gpu-frequency-min GPU_FREQUENCY_MIN]
                     [--gpu-frequency-max GPU_FREQUENCY_MAX]
                     [--gpu-frequency-step GPU_FREQUENCY_STEP]
                     [--gpu-power GPU_POWER_DOMAIN]
                     [--gpu-power-min GPU_POWER_MIN]
                     [--gpu-power-max GPU_POWER_MAX]
                     [--gpu-power-step GPU_POWER_STEP]
                     [--board-power BOARD_POWER_DOMAIN]
                     [--board-power-min BOARD_POWER_MIN]
                     [--board-power-max BOARD_POWER_MAX]
                     [--board-power-step BOARD_POWER_STEP]
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

    geopmgrid --cpu-frequency package --coordinate 3 5

Generate configuration from file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    echo "13 8" > coordinate.txt
    geopmgrid --cpu-frequency board --cpu-power board --coordinate-file coordinate.txt

Apply configuration to platform
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --cpu-frequency package --coordinate 3 5 --write

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

The tool integrates with the GEOPM PlatformIO (pio) interface to determine
available parameter ranges, step sizes, and valid domains for each control type.


Options
-------

Control Parameters
~~~~~~~~~~~~~~~~~~

--cpu-frequency CPU_FREQUENCY_DOMAIN  .. _cpu-frequency option:

    Define a grid dimension over CPU_FREQUENCY_MAX_CONTROL for the specified
    domain. Valid domains include 'board', 'package', 'core', and on some
    platforms 'cpu' depending on capabilities.

--cpu-frequency-min CPU_FREQUENCY_MIN  .. _cpu-frequency-min option:

    Override the automatically detected minimum CPU frequency when constructing
    the grid. The supplied value is used for all selected domains.

--cpu-frequency-max CPU_FREQUENCY_MAX  .. _cpu-frequency-max option:

    Override the automatically detected maximum CPU frequency when constructing
    the grid. The supplied value is used for all selected domains.

--cpu-frequency-step CPU_FREQUENCY_STEP  .. _cpu-frequency-step option:

    Override the step size used to enumerate CPU frequency settings when
    constructing the grid.

--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN  .. _cpu-uncore-frequency option:

    Define a grid dimension over CPU_UNCORE_FREQUENCY_MAX_CONTROL for the
    specified domain. The uncore frequency can be controlled on the 'board' or
    'package' domain.

--cpu-uncore-frequency-min CPU_UNCORE_FREQUENCY_MIN  .. _cpu-uncore-frequency-min option:

    Override the automatically detected minimum CPU uncore frequency when
    constructing the grid.

--cpu-uncore-frequency-max CPU_UNCORE_FREQUENCY_MAX  .. _cpu-uncore-frequency-max option:

    Override the automatically detected maximum CPU uncore frequency when
    constructing the grid.

--cpu-uncore-frequency-step CPU_UNCORE_FREQUENCY_STEP  .. _cpu-uncore-frequency-step option:

    Override the step size used to enumerate CPU uncore frequency settings.

--cpu-power CPU_POWER_DOMAIN  .. _cpu-power option:

    Define a grid dimension over CPU_POWER_LIMIT_CONTROL for the specified
    domain. Commonly used with 'board', or 'package' domains.

--cpu-power-min CPU_POWER_MIN  .. _cpu-power-min option:

    Override the automatically detected minimum CPU power limit when
    constructing the grid.

--cpu-power-max CPU_POWER_MAX  .. _cpu-power-max option:

    Override the automatically detected maximum CPU power limit when
    constructing the grid.

--cpu-power-step CPU_POWER_STEP  .. _cpu-power-step option:

    Override the step size used to enumerate CPU power limit settings.

--gpu-frequency GPU_FREQUENCY_DOMAIN  .. _gpu-frequency option:

    Define a grid dimension over GPU_CORE_FREQUENCY_MAX_CONTROL for the
    specified domain. Valid domains include 'board', 'gpu', and on some
    platforms 'gpu_chip'.

--gpu-frequency-min GPU_FREQUENCY_MIN  .. _gpu-frequency-min option:

    Override the automatically detected minimum GPU frequency when constructing
    the grid.

--gpu-frequency-max GPU_FREQUENCY_MAX  .. _gpu-frequency-max option:

    Override the automatically detected maximum GPU frequency when constructing
    the grid.

--gpu-frequency-step GPU_FREQUENCY_STEP  .. _gpu-frequency-step option:

    Override the step size used to enumerate GPU frequency settings.

--gpu-power GPU_POWER_DOMAIN  .. _gpu-power option:

    Define a grid dimension over GPU_POWER_LIMIT_CONTROL for the specified
    domain. Typically applied at the 'board', or 'gpu' domain.

--gpu-power-min GPU_POWER_MIN  .. _gpu-power-min option:

    Override the automatically detected minimum GPU power limit when
    constructing the grid.

--gpu-power-max GPU_POWER_MAX  .. _gpu-power-max option:

    Override the automatically detected maximum GPU power limit when
    constructing the grid.

--gpu-power-step GPU_POWER_STEP  .. _gpu-power-step option:

    Override the step size used to enumerate GPU power limit settings.

--board-power BOARD_POWER_DOMAIN  .. _board-power option:

    Define a grid dimension over BOARD_POWER_LIMIT_CONTROL for the specified
    domain. Only valid with 'board' domain.

--board-power-min BOARD_POWER_MIN  .. _board-power-min option:

    Override the automatically detected minimum board-level power limit when
    constructing the grid.

--board-power-max BOARD_POWER_MAX  .. _board-power-max option:

    Override the automatically detected maximum board-level power limit when
    constructing the grid.

--board-power-step BOARD_POWER_STEP  .. _board-power-step option:

    Override the step size used to enumerate board-level power settings.

Grid Navigation
~~~~~~~~~~~~~~~

--coordinate COORDINATE  .. _coordinate option:

    Specify a coordinate within the grid to generate configuration for.  The
    number of white-space-separated integer values in COORDINATE must match the
    number of defined grid dimensions.  Each coordinate is an integer index into
    the corresponding dimension's parameter range. For example, to express grid
    point (2, 1, 0) use ``--coordinate 2 1 0``.

--coordinate-file COORDINATE_FILE  .. _coordinate-file option:

    Read grid coordinates from a file. The file should contain
    white-space-separated integer coordinates corresponding to each grid
    dimension.

--coordinate-range  .. _coordinate-range option:

    Display the size of each dimension in the defined grid. This shows the
    number of available parameter values for each control type and domain.

Actions
~~~~~~~

--write  .. _write option:

    Apply the configuration to the platform. Requires either ``--coordinate`` or
    ``--coordinate-file`` to specify which grid point to apply. This option
    pushes the control values directly to the platform hardware.

-h, --help  .. _help option:

    Print help message and exit.


Examples
--------

Exploring CPU frequency grid
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define a grid over CPU frequency for all packages and display its dimensions:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate-range
   28 28

This shows that the CPU frequency grid has 28 available frequency settings
across both package domains. This implies that valid coordinate values are 0
through 27.

Generating configuration commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Generate geopmwrite commands for a specific grid point:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate 10 15
   CPU_FREQUENCY_MAX_CONTROL package 0 2000000000.0
   CPU_FREQUENCY_MAX_CONTROL package 1 2500000000.0

This generates a ``geopmwrite(1)`` configuration file that sets package 0 to 2 GHz
and package 1 to 2.5 GHz.

Multi-dimensional grid exploration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define a 2D grid over CPU frequency and power limit:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate-range
   28 28 155 155

   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate 12 17 125 101
   CPU_FREQUENCY_MAX_CONTROL package 0 2200000000.0
   CPU_FREQUENCY_MAX_CONTROL package 1 2700000000.0
   CPU_POWER_LIMIT_CONTROL package 0 271.0
   CPU_POWER_LIMIT_CONTROL package 1 247.0

This creates a 4D grid with 28x28x155x155 = 18,835,600 possible configurations
and generates commands for coordinate (12, 17, 125, 101).

Using coordinate files
~~~~~~~~~~~~~~~~~~~~~~

Store coordinates in a file for repeated use:

.. code-block:: shell-session

   $ echo 18 14 122 96 > my_config.coord
   $ geopmgrid --cpu-frequency package --cpu-power package --coordinate-file my_config.coord
   CPU_FREQUENCY_MAX_CONTROL package 0 2800000000.0
   CPU_FREQUENCY_MAX_CONTROL package 1 2400000000.0
   CPU_POWER_LIMIT_CONTROL package 0 268.0
   CPU_POWER_LIMIT_CONTROL package 1 242.0

This can also be helpful for systems with high dimensionality.

Direct platform configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Apply a configuration directly to the platform:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency package --coordinate 15 19 --write

This immediately applies the configuration to the platform hardware without
printing the intermediate geopmwrite commands.

GPU control grids
~~~~~~~~~~~~~~~~~

Define grids for GPU controls:

.. code-block:: shell-session

   $ geopmgrid --gpu-frequency gpu --gpu-power gpu --coordinate-range
   187 187 187 187 101 101 101 101
   $ geopmgrid --gpu-frequency gpu --gpu-power gpu --coordinate 111 122 133 144 80 90 100 70
   GPU_CORE_FREQUENCY_MAX_CONTROL gpu 0 967500000.0
   GPU_CORE_FREQUENCY_MIN_CONTROL gpu 0 967500000.0
   GPU_CORE_FREQUENCY_MAX_CONTROL gpu 1 1050000000.0
   GPU_CORE_FREQUENCY_MIN_CONTROL gpu 1 1050000000.0
   GPU_CORE_FREQUENCY_MAX_CONTROL gpu 2 1132500000.0
   GPU_CORE_FREQUENCY_MIN_CONTROL gpu 2 1132500000.0
   GPU_CORE_FREQUENCY_MAX_CONTROL gpu 3 1215000000.0
   GPU_CORE_FREQUENCY_MIN_CONTROL gpu 3 1215000000.0
   GPU_POWER_LIMIT_CONTROL gpu 0 280
   GPU_POWER_LIMIT_CONTROL gpu 1 290
   GPU_POWER_LIMIT_CONTROL gpu 2 300
   GPU_POWER_LIMIT_CONTROL gpu 3 270

Complete system grid
~~~~~~~~~~~~~~~~~~~~

Define a comprehensive grid covering multiple subsystems:

.. code-block:: shell-session

   $ geopmgrid --cpu-frequency board \
               --cpu-uncore-frequency board \
               --cpu-power board \
               --gpu-frequency board \
               --gpu-power board \
               --coordinate-range
   28 15 155 187 1001

This creates a 5D grid with 28x15x155x187x1001 = 12,185,873,700 possible
configurations for comprehensive system optimization.

Domain Types
------------

``geopmgrid`` supports various domain types depending on the platform and control type:

**CPU Controls:**
- ``package``: CPU package/socket level
- ``core``: Individual CPU core level
- ``cpu``: Linux logical CPU (hardware thread)

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
application launches for parameter sweeps with the --geopm-init-control option.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmopt(1) <geopmopt.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
:doc:`geopmlaunch(1) <geopmlaunch.1>`
