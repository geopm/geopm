geopm_pio_time(7) -- Signals and controls for Time IO Group
===========================================================

Description
-----------

The TimeIOGroup implements the :doc:`geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3>` interface to provide
signals relating to time measurements.

Signals
-------

``TIME``
    Returns a monotonic measurement of time in seconds from some point in the
    past.

    * **Aggregation**: select_first
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``TIME::ELAPSED``
    Returns a monotonic measurement of time in seconds since the TimeIOGroup
    was constructed.

    * **Aggregation**: select_first
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`
