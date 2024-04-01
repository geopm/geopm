geopm_pio_const_config(7) -- Signals for ConstConfigIOGroup
===========================================================

Description
-----------

ConstConfigIOGroup implements the
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` interface to provide a
predefined set of signals with constant values. This is mainly useful to
provide configuration information needed by some agents (e.g. system
characterization information).

Signals
-------

This IOGroup doesn't provide any signals by default. In order to define
signals, a configuration file in JSON format needs to be provided to the
IOGroup via the ``GEOPM_CONST_CONFIG_PATH`` environment variable. If not
provided, it will default to looking for a file named ``const_config_io.json``
in the GEOPM configuration path (``/etc/geopm``). In either case, the
configuration file must comply with the following schema:

.. literalinclude:: ../json_schemas/const_config_io.schema.json
    :language: json

Notice how all signal fields are required. The ``values`` field is an array
that should contain a value per domain item. The name of the signal will be
derived from the object's name and prefixing it with ``CONST_CONFIG::``.

The following example helps illustrate the schema:

.. code-block:: JSON

    {
        "GPU_CORE_FREQUENCY_MAX": {
            "domain": "gpu",
            "description": "Defines the max core frequency to use for available GPUs",
            "units": "hertz",
            "aggregation": "average",
            "values": [1200, 1300, 1500]
        }
    }

This basic example only defines one signal. Its name, once processed by the
IOGroup, will be: ``CONST_CONFIG::GPU_CORE_FREQUENCY_MAX``. Also, it can be
inferred that for the system being characterized, there are three available
GPUs, each with the specified max frequency.

Controls
--------

Due to the nature of this IOGroup, it doesn't provide any controls.

Aliases
-------

To avoid a configuration from introducing signals that may collide with
signals provided by other IOGroups, this IOGroup does not provide any
high-level aliases.

Example
-------

Following the example configuration provided earlier, ``geopmread`` could be
used to read configuration values for the defined signal, for example:

``geopmread CONST_CONFIG::GPU_CORE_FREQUENCY_MAX gpu 1``

This example would return the signal value at domain index 1 (1300).

Using ``geopmread`` like this would be useful for manually checking the loaded
configuration values from a configuration file, but the main use case for this
will be to provide agents with configuration information.

See Also
--------

:doc:`geopm_pio(7) <geopm_pio.7>`\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
