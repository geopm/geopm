
Guide for Service Clients
=========================

The GEOPM Service facilitates the PlatformIO interfaces by providing
security and quality guarantees.  Clients of the GEOPM service may
access the PlatformIO features through a variety of interfaces
including C, C++, Python, and command line tools.  The administrator
of the system may manage client access rights through the
``geopmaccess`` command line tool.  PlatformIO uses the PlatformTopo
abstraction to define the hardware available on the system.

A high level overview of the PlatformIO abstraction is described in
the :doc:`geopm_pio(7) <geopm_pio.7>` man page.  This page and the
pages linked within describe all signals and controls that are available
through this interface.  A client may read the signals or write the
controls described in the overview with the interfaces listed below.



Language Bindings
-----------------

C API
^^^^^
- :doc:`geopm_pio(3) <geopm_pio.3>`
- :doc:`geopm_topo(3) <geopm_topo.3>`


C++ API
^^^^^^^
- :doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`
- :doc:`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3>`


Python3 API
^^^^^^^^^^^
- :ref:`geopmdpy.7:geopmdpy.pio`
- :ref:`geopmdpy.7:geopmdpy.topo`


Command Line Tools
------------------
- Read one value: :doc:`geopmread(1) <geopmread.1>`
- Write one value: :doc:`geopmwrite(1) <geopmwrite.1>`
- Read time series of values :doc:`geopmsession(1) <geopmsession.1>`
- Access management :doc:`geopmaccess(1) <geopmaccess.1>`
