geopm_pio_sysfs(7) -- Signals and controls for sysfs attributes
===============================================================

Description
-----------

The SysfsIOGroup implements the
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` interface to provide
hardware signals and controls for sysfs attribute files.

The set of signals and controls supported by an instance of SysfsIOGroup is
configurable through a JSON file used by the IOGroup. The JSON file informs the
IOGroup about which sysfs attributes to expose through GEOPM, and how to
interpret those attributes.

The JSON files must follow this schema:

.. literalinclude:: ../json_schemas/sysfs_attributes.schema.json
    :language: json

For an example of a sysfs configuration file, please see:
`<sysfs_attributes.json> <https://github.com/geopm/geopm/blob/dev/docs/json_data/sysfs_attributes_cpufreq.json>`_

This guide includes a list of signals and controls that are available in
instances of SysfsIOGroup that are bundled with GEOPM. Your system may expose
a subset of these signals and controls (e.g., if a driver is not loaded, or if
it is loaded but in a state that does not expose all of the attributes), or
may expose additional signals and controls (e.g., if you have configured
additional instances of SysfsIOGroup that are not bundled with GEOPM). Use
``geopmread`` and ``geopmwrite`` to query the full set of signals and controls
that are available on a particular system.

Signals and controls in this IOGroup come from multiple drivers that provide
sysfs attributes. Some signals and controls come from the ``cpufreq`` driver.
For more information, see the `cpufreq documentation
<https://docs.kernel.org/admin-guide/pm/cpufreq.html>`_.

Some signals and controls are made available to GEOPM through the sysfs
interface to the ``i915 DRM`` or ``Xe DRM`` (Direct Rendering Manager)
driver. For more information, see the `i915 documentation
<https://www.kernel.org/doc/html/next/gpu/i915.html>`_, `Xe documentation
<https://www.kernel.org/doc/html/next/gpu/driver-uapi.html#drm-xe-uapi>`_, and
the `oneAPI GPU Optimization Guide
<https://www.intel.com/content/www/us/en/docs/oneapi/optimization-guide-gpu/2024-0/configuring-gpu-device.html>`_.
Furthermore, the DRM devices may link to hwmon devices. If the hwmon
links are present, this IOGroup also exposes signals and controls from the `i915 hwmon interface
<https://www.kernel.org/doc/html/latest/admin-guide/abi-testing.html#file-testing-sysfs-driver-intel-i915-hwmon>`_
and the `xe hwmon interface
<https://www.kernel.org/doc/html/latest/admin-guide/abi-testing.html#file-testing-sysfs-driver-intel-xe-hwmon>`_
The i915 driver is available in `upstream Linux i915
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/i915>`_.
Additional features are available in the `out-of-tree version of the driver
<https://github.com/intel-gpu/intel-gpu-i915-backports>`_.
The Xe driver is available in `upstream Linux Xe
<https://web.git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/xe>`_.
This IOGroup is intended for use with any of these driver interfaces.

If a GEOPM sysfs-based signal came from the DRM system, then its name begins
with ``DRM``. The driver (e.g., ``i915`` that made that signal available to
``DRM`` can be viewed by running ``geopmread --info SIGNAL_NAME``. For example:

.. code-block:: shell-session

   $ geopmread --info DRM::HWMON::ENERGY1_INPUT::GPU
   DRM::HWMON::ENERGY1_INPUT::GPU:
       description: GPU card-level energy counter
       units: joules
       aggregation: sum
       domain: gpu
       iogroup: DRM from driver: i915

Some signals are made available to GEOPM through the sysfs interface to the
``powercap`` driver. For more information, see the `powercap documentation
<https://www.kernel.org/doc/html/next/power/powercap/powercap.html>`_.

Signals
-------
.. contents:: Categories of sysfs signals:
   :local:

Cpufreq Signals
^^^^^^^^^^^^^^^
The following signals are made available to GEOPM through the sysfs interface
to the ``cpufreq`` driver.

.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
   :no-controls:

DRM Signals
^^^^^^^^^^^
The following signals are made available to GEOPM through the sysfs interface
to the ``i915 DRM`` and ``Xe DRM`` drivers.

.. geopm-sysfs-json:: DRM ../json_data/sysfs_attributes_drm.json
   :no-controls:

Powercap Signals
^^^^^^^^^^^^^^^^
The following signals are made available to GEOPM through the sysfs interface
to the ``powercap`` driver.

.. geopm-sysfs-json:: POWERCAP ../json_data/sysfs_attributes_powercap.json
   :no-controls:

Controls
--------
.. contents:: Categories of SYSFS controls:
   :local:

Cpufreq Controls
^^^^^^^^^^^^^^^^
The following controls are made available to GEOPM through the sysfs interface
to the ``cpufreq`` driver.

.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
   :no-signals:

DRM Controls
^^^^^^^^^^^^
The following controls are made available to GEOPM through the sysfs interface
to the ``i915 DRM`` and ``Xe DRM`` drivers.

.. geopm-sysfs-json:: DRM ../json_data/sysfs_attributes_drm.json
   :no-signals:

Powercap Controls
^^^^^^^^^^^^^^^^^
The following controls are made available to GEOPM through the sysfs interface
to the ``powercap`` driver.

.. geopm-sysfs-json:: POWERCAP ../json_data/sysfs_attributes_powercap.json
   :no-signals:

Aliases
-------
This IOGroup provides the following high-level aliases. Note that some aliases
may map to multiple potential sysfs attributes. Your system will map only one
attribute to a given alias, depending on the driver's configuration. Use
``geopmread`` and ``geopmwrite`` to query information about each alias on your system.

Signal Aliases
^^^^^^^^^^^^^^
.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
   :no-controls:
   :aliases:

.. geopm-sysfs-json:: DRM ../json_data/sysfs_attributes_drm.json
   :no-controls:
   :aliases:

.. geopm-sysfs-json:: POWERCAP ../json_data/sysfs_attributes_powercap.json
   :no-controls:
   :aliases:

Control Aliases
^^^^^^^^^^^^^^^
.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
   :no-signals:
   :aliases:

.. geopm-sysfs-json:: DRM ../json_data/sysfs_attributes_drm.json
   :no-signals:
   :aliases:

.. geopm-sysfs-json:: POWERCAP ../json_data/sysfs_attributes_powercap.json
   :no-signals:
   :aliases:

Example
-------
The following example uses ``geopmread`` and ``geopmwrite`` command-line tools.
These steps can also be followed within an agent.

Setting Frequency
^^^^^^^^^^^^^^^^^
* Set the maximum allowed operating frequency on core 0 to 1.7 GHz:

``geopmwrite CPU_FREQUENCY_MAX_CONTROL core 0 1700000000``

* Read the current maximum allowed frequency and the recent operating frequency of core 0:

``geopmread CPU_FREQUENCY_MAX_CONTROL core 0``

``geopmread CPU_FREQUENCY_STATUS core 0``


See Also
--------
:doc:`geopm_pio(7) <geopm_pio.7>`\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
