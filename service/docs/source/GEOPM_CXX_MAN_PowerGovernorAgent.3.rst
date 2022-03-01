.. role:: raw-html-m2r(raw)
   :format: html


geopm::PowerGovernorAgent(3) -- agent that enforces a power cap
===============================================================






SYNOPSIS
--------

#include `<geopm/PowerGovernorAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/PowerGovernorAgent.hpp>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``

DESCRIPTION
-----------

The behavior of this agent is described in more detail in the
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_ man page.  The power limit is
enforced using the `geopm::PowerGovernor(3) <GEOPM_CXX_MAN_PowerGovernor.3.html>`_ class.

For more details, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_power_governor_agent.html.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm::PowerGovernor(3) <GEOPM_CXX_MAN_PowerGovernor.3.html>`_
