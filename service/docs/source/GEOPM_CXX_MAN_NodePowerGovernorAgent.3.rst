
geopm::PowerGovernorAgent(3) -- agent that enforces a power cap
===============================================================


Synopsis
--------

#include `<geopm/NodePowerGovernorAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/NodePowerGovernorAgent.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**

Description
-----------

The ``NodePowerGovernorAgent`` enforces a per-compute-node power cap of the total platform power in
systems that support the Platform Power Limit feature.  The platform power is determined by the platform vendor implementation.
If the feature is not supported on the system of interest the
:doc:`geopm::PowerGovernorAgent(3) <GEOPM_CXX_MAN_PowerGovernorAgent.3>` can be used for CPU only power limiting.

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_node_power_governor(7) <geopm_agent_node_power_governor.7>` man page.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm_agent_node_power_governor(7) <geopm_agent_node_power_governor.7>`\ ,
