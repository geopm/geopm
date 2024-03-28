geopm_pio_sysfs(7) -- Signals and controls for sysfs attributes
===============================================================

Description
-----------

The SysfsIOGroup implements the
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>` interface to provide
hardware signals and controls for sysfs attribute files.

The set of signals and controls supported by an instance of SysfsIOGroup is
configurable through a JSON file used by the IOGroup. The JSON file informs the
IOGroup about which sysfs attributes to expose through GEOPM, and how to
interpret those attributes.

The JSON files must follow this schema:

.. literalinclude:: ../json_schemas/sysfs_attributes.schema.json
    :language: json

For an example of a sysfs configuration file, please see:
`<sysfs_attributes.json> <https://github.com/geopm/geopm/blob/dev/service/docs/json_data/sysfs_attributes_cpufreq.json>`_

This guide includes a list of signals and controls that are available in
instances of SysfsIOGroup that are bundled with GEOPM. Your system may expose
a subset of these signals and controls (e.g., if a driver is not loaded, or if
it is loaded but in a state that does not expose all of the attributes), or
may expose additional signals and controls (e.g., if you have configured
additional instances of SysfsIOGroup that are not bundled with GEOPM). Use
``geopmread`` and ``geopmwrite`` to query the full set of signals and controls
that are available on a particular system.

Signals
-------
.. contents:: Categories of sysfs signals:
   :local:

Cpufreq Signals
^^^^^^^^^^^^^^^
The following signals are made available to GEOPM through the sysfs interface
to the ``cpufreq`` driver. For more information, see the
`cpufreq documentation <https://docs.kernel.org/admin-guide/pm/cpufreq.html>`_.

.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
   :no-controls:

Controls
--------
.. contents:: Categories of SYSFS controls:
   :local:

Cpufreq Controls
^^^^^^^^^^^^^^^^
The following controls are made available to GEOPM through the sysfs interface
to the ``cpufreq`` driver. For more information, see the
`cpufreq documentation <https://docs.kernel.org/admin-guide/pm/cpufreq.html>`_.

.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
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

Control Aliases
^^^^^^^^^^^^^^^
.. geopm-sysfs-json:: CPUFREQ ../json_data/sysfs_attributes_cpufreq.json
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
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
