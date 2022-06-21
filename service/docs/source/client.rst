
Guide for Service Clients
=========================

The ``geopmsession`` command line tool is one way to use the GEOPM
Service's client facing DBus APIs.  This provides an easy way to read
signals once, read them periodically, or write to a set of controls
for the lifetime of the session.  Note that the system administrator
must grant access to the signals or controls with the ``geopmaccess``
command line tool before they will be available to a non-root user
with the ``geopmsession`` command.


Reading Instructions Retired
----------------------------

In this example we will read the hardware counter that measures
instructions retired for Linux logical CPU 0 and 1.  This read will be
done every 100 milliseconds for one second.  Additionally we will read
the TIME signal for the first column of output.  The result will be a
comma separated list of the three values read printed ten times to
standard output.

.. code-block:: bash

    echo "TIME board 0
          CPU_INSTRUCTIONS_RETIRED cpu 0
          CPU_INSTRUCTIONS_RETIRED cpu 1" | geopmsession -t 1 -p 0.1
