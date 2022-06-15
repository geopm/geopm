
geopm::PowerBalancerAgent(3) -- agent optimizing performance under a power cap
==============================================================================


Synopsis
--------

#include `<geopm/PowerBalancerAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/PowerBalancerAgent.hpp>`_

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``

Description
-----------

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>` man page.  The balancing algorithm
is implemented using the :doc:`geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3>` class.

For more details, see the doxygen
page at https://geopm.github.io/doxall/classgeopm_1_1_power_balancer_agent.html.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3>`
