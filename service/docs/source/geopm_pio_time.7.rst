
geopm_pio_time(7) -- Signals and controls for Time IO Group
===========================================================

Description
-----------

The TimeIOGroup implements the `geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to provide 
signals relating to time measurements.


Signals
-------
``TIME``
    Returns a monotonic measurement of time in seconds from some point in the
    past. 

    * **Aggregation**: N/A

    * **Domain**: CPU

    * **Format**: Double

    * **Unit**: Seconds

``TIME::ELAPSED``
    Returns a monotonic measurement of time in seconds since the TimeIOGroup
    was constructed. 

    * **Aggregation**: N/A

    * **Domain**: CPU

    * **Format**: Double

    * **Unit**: Seconds

See Also
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_,
`geopmread(1) <geopmread.1.html>`_
