.. role:: raw-html-m2r(raw)
   :format: html


geopm::PowerBalancerAgent(3) -- agent optimizing performance under a power cap
==============================================================================






SYNOPSIS
--------

#include `<geopm/PowerBalancerAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/PowerBalancerAgent.hpp>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``

DESCRIPTION
-----------

The behavior of this agent is described in more detail in the
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_ man page.  The balancing algorithm
is implemented using the `geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3.html>`_ class.

For more details, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_power_balancer_agent.html.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3.html>`_
