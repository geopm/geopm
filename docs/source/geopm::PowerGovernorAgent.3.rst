
geopm::PowerGovernorAgent(3) -- agent that enforces a power cap
===============================================================


Synopsis
--------

#include `<geopm/PowerGovernorAgent.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/PowerGovernorAgent.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**

Description
-----------

The ``PowerGovernorAgent`` enforces a per-compute-node power cap of the total power of all packages (sockets).

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>` man page.  The power limit is
enforced using the :doc:`geopm::PowerGovernor(3) <geopm::PowerGovernor.3>` class.

For more details, see the `doxygen page <https://geopm.github.io/geopm-runtime-dox/classgeopm_1_1_power_governor_agent.html>`_.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`\ ,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`\ ,
:doc:`geopm::PowerGovernor(3) <geopm::PowerGovernor.3>`
