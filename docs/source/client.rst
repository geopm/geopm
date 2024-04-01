Service Clients
===============

The GEOPM Service enhances the PlatformIO interfaces by ensures
reliability and quality.  Users of the GEOPM service can
utilize the PlatformIO capabilities through different interfaces
such as C, C++, Python, and command line tools. The system
administrator can manage user access rights through using the ``geopmaccess``
command line tool. PlatformTopo is an abstraction that outlines the
system's hardware.

An comprehensive overview of the PlatformIO abstraction is outlined in
the :doc:`geopm_pio(7) <geopm_pio.7>` man page. This page along with
the linked pages describe all signals and controls available
through this interface. Clients can read the signals or utilizes the
controls explained in the overview with the interfaces provided below.


Language Bindings
-----------------

C API
^^^^^
- :doc:`geopm_pio(3) <geopm_pio.3>`
- :doc:`geopm_topo(3) <geopm_topo.3>`


C++ API
^^^^^^^
- :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`
- :doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>`


Python3 API
^^^^^^^^^^^
- :ref:`geopmdpy.7:geopmdpy.pio`
- :ref:`geopmdpy.7:geopmdpy.topo`


Command Line Tools
------------------
- Read a single value: :doc:`geopmread(1) <geopmread.1>`
- Write a single value: :doc:`geopmwrite(1) <geopmwrite.1>`
- Read time series of values :doc:`geopmsession(1) <geopmsession.1>`
- Manage access :doc:`geopmaccess(1) <geopmaccess.1>`
