geopmgrid(1) -- define control parameter grids
==============================================

Synopsis
--------

.. code-block:: bash

    usage: geopmgrid [-h]
                     [--sweep DIM]
                     [--list-controls]
                     [--coordinate COORDINATE [COORDINATE ...] |
                      --coordinate-file COORDINATE_FILE |
                      --coordinate-range]
                     [--write]

List available controls
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --list-controls

Display grid dimensions
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --sweep cpu-freq@package --coordinate-range

Generate configuration for a grid point
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --sweep cpu-freq@package --coordinate 3 5

Generate configuration from file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    echo "13 8" > coordinate.txt
    geopmgrid --sweep cpu-freq@board --sweep cpu-power@board --coordinate-file coordinate.txt

Apply configuration to platform
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmgrid --sweep cpu-freq@package --coordinate 3 5 --write

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

--sweep DIM  .. _sweep option:

    Add a control dimension to the grid. May be given multiple times to build
    a multi-dimensional grid. Each ``DIM`` uses the grammar
    ``CONTROL[@DOMAIN][=MIN:MAX:STEP]``:

    - ``CONTROL`` is a control name or alias (see ``--list-controls`` for the
      full catalog). Recognized names include
      ``cpu-freq`` (alias ``cpu-frequency``), ``uncore-freq`` (alias
      ``cpu-uncore-frequency``), ``cpu-power``, ``gpu-freq`` (alias
      ``gpu-frequency``), ``gpu-power``, ``board-power``, and ``prefetch``
      (alias ``prefetch-disable``).

    - ``@DOMAIN`` optionally pins the control to a platform domain such as
      ``board``, ``package``, ``core``, ``cpu``, ``gpu``, or ``gpu_chip``. When
      omitted, the control's native domain is used.

    - ``=MIN:MAX:STEP`` optionally overrides the auto-detected range. Each of
      the three fields is independent and may be left empty to keep its
      auto-detected value, for example ``=1.2GHz:3GHz:100MHz`` (all three),
      ``=::100MHz`` (step only), ``=1.2GHz:3GHz`` (bounds only), or
      ``=1.2GHz::`` (minimum only).

    Frequency values accept the unit suffixes ``Hz``, ``kHz``, ``MHz``, and
    ``GHz``; power values accept ``W`` and ``kW``. A bare number is interpreted
    in the control's canonical unit (Hz for frequency, W for power). The
    ``prefetch`` control takes non-negative integer levels and rejects unit
    suffixes.

--list-controls  .. _list-controls option:

    Print a table of the available control names, their native domain, units,
    and the detected minimum, maximum, and step values, then exit. Controls
    whose range cannot be read on the current platform are shown as ``n/a``.

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

   $ geopmgrid --sweep cpu-freq@package --coordinate-range
   28 28

This shows that the CPU frequency grid has 28 available frequency settings
across both package domains. This implies that valid coordinate values are 0
through 27.

Generating configuration commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Generate geopmwrite commands for a specific grid point:

.. code-block:: shell-session

   $ geopmgrid --sweep cpu-freq@package --coordinate 10 15
   CPU_FREQUENCY_MAX_CONTROL package 0 2000000000.0
   CPU_FREQUENCY_MAX_CONTROL package 1 2500000000.0

This generates a ``geopmwrite(1)`` configuration file that sets package 0 to 2 GHz
and package 1 to 2.5 GHz.

Multi-dimensional grid exploration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Define a 2D grid over CPU frequency and power limit:

.. code-block:: shell-session

   $ geopmgrid --sweep cpu-freq@package --sweep cpu-power@package --coordinate-range
   28 28 155 155

   $ geopmgrid --sweep cpu-freq@package --sweep cpu-power@package --coordinate 12 17 125 101
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
   $ geopmgrid --sweep cpu-freq@package --sweep cpu-power@package --coordinate-file my_config.coord
   CPU_FREQUENCY_MAX_CONTROL package 0 2800000000.0
   CPU_FREQUENCY_MAX_CONTROL package 1 2400000000.0
   CPU_POWER_LIMIT_CONTROL package 0 268.0
   CPU_POWER_LIMIT_CONTROL package 1 242.0

This can also be helpful for systems with high dimensionality.

Direct platform configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Apply a configuration directly to the platform:

.. code-block:: shell-session

   $ geopmgrid --sweep cpu-freq@package --coordinate 15 19 --write

This immediately applies the configuration to the platform hardware without
printing the intermediate geopmwrite commands.

GPU control grids
~~~~~~~~~~~~~~~~~

Define grids for GPU controls:

.. code-block:: shell-session

   $ geopmgrid --sweep gpu-freq@gpu --sweep gpu-power@gpu --coordinate-range
   187 187 187 187 101 101 101 101
   $ geopmgrid --sweep gpu-freq@gpu --sweep gpu-power@gpu --coordinate 111 122 133 144 80 90 100 70
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

   $ geopmgrid --sweep cpu-freq@board \
               --sweep uncore-freq@board \
               --sweep cpu-power@board \
               --sweep gpu-freq@board \
               --sweep gpu-power@board \
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
