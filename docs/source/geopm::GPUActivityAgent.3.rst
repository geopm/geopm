
geopm::GPUActivityAgent(3) -- agent for selecting GPU frequency based on GPU compute activity
=============================================================================================


Synopsis
--------

#include `"GPUActivityAgent.hpp" <https://github.com/geopm/geopm/blob/dev/libgeopm/src/GPUActivityAgent.hpp>`_\

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**

Requires ``-enable-nvml`` and ``-enable-dcgm`` on systems with NVIDIA GPUs

Description
-----------

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>` man page.

For more details on the implementation, see the
`doxygen page <https://geopm.github.io/geopm-runtime-dox/classgeopm_1_1_g_p_u_activity_agent.html>`_.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`
