.. role:: raw-html-m2r(raw)
   :format: html


geopm::EnergyEfficientRegion(3) -- region frequency tuning history
==================================================================






NAMESPACES
----------

The ``EnergyEfficientRegion`` class and the ``EnergyEfficientRegionImp`` class are members of
the ``namespace geopm``, but the full names, ``geopm::EnergyEfficientRegion`` and
``geopm::EnergyEfficientRegionImp``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

Note that the ``EnergyEfficientRegion`` class is an abstract base class.  There is one
concrete implementation, ``EnergyEfficientRegionImp``.

SYNOPSIS
--------

#include `<geopm/EnergyEfficientRegion.hpp> <https://github.com/geopm/geopm/blob/dev/src/EnergyEfficientRegion.hpp>`_\ 

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       virtual double EnergyEfficientRegion::freq(void) const;

       virtual void EnergyEfficientRegion::update_freq_range(double freq_min, duble freq_max, double freq_step);

       virtual void EnergyEfficientRegion::update_exit(double curr_perf_metric);

       virtual bool EnergyEfficientRegion::is_learning(void) const;

DESCRIPTION
-----------

Holds the performance history of a Region.

This class is used by `geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_ to store
history about the performance of a region at each frequency setting
and find the best frequency for each region.

For more details, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_energy_efficient_region.html.

CLASS METHODS
-------------

**TODO**

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_
`geopm::EnergyEfficientAgent(3) <GEOPM_CXX_MAN_EnergyEfficientAgent.3.html>`_
