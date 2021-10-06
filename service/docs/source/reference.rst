
Reference Manual
================

geopmaccess
-----------

Access management for the geopm service.

Command line tool for reading and writing the access management lists
for the geopm service signals and controls.


geopmsession
------------

Command line interface for the geopm service read/write features.

This command can be used to read signals and write controls by opening
a session with the geopm service.  The user specifies which signals to
read or which controls to write with standard input. When no command
line options are provided, a single read of the signals requested
though standard input is made and the results are printed to the
screen.


The input to the command line tool has one request per line.  A
request for reading is made of up three strings separated by white
space.  The first string is the signal name, the second string is the
domain name, and the third string is the domain index.  An example
where the entire THERM_STATUS model specific register is read from
core zero:

.. code-block:: bash

    $ geopmsession
    MSR::THERM_STATUS# core 0

    0x0000000088430800


A request for writing is made up of four strings separated by white
space.  The first string is the control name, the second string is the
domain name, and the third string is the domain index.  An example
where the frequency limit of the PERF_CTL model specific register is
set to 2.0 GHz for 10 seconds:


.. code-block:: bash

    $ geopmsession -w -t 10
    MSR::PERF_CTL:FREQ core 0 2.0e9


The time specified is the duration after the write has completed until
the process of restoring all written values begins.  The total run
time for the command will be longer than the time specified because of
the overhead time spent reading and writing all controls for the
save/restore feature of the service in addition to the time spent
writing the requested controls.


geopmdpy.service
----------------

.. automodule:: geopmdpy.service
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.dbus\_xml
------------------

.. automodule:: geopmdpy.dbus_xml
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.pio
------------

.. automodule:: geopmdpy.pio
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.topo
-------------

.. automodule:: geopmdpy.topo
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.session
----------------

.. automodule:: geopmdpy.session
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.access
---------------

.. automodule:: geopmdpy.access
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.runtime
----------------

.. automodule:: geopmdpy.runtime
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.error
--------------

.. automodule:: geopmdpy.error
   :members:
   :undoc-members:
   :show-inheritance:

geopmdpy.version
----------------

.. automodule:: geopmdpy.version
   :members:
   :undoc-members:
   :show-inheritance:
