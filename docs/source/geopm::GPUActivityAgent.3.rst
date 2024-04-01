
geopm::GPUActivityAgent(3) -- agent for selecting GPU frequency based on GPU compute activity
=============================================================================================


Synopsis
--------

#include `<geopm/GPUActivityAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/GPUActivityAgent.hpp>`_\

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**

Requires ``-enable-nvml`` and ``-enable-dcgm`` on systems with NVIDIA GPUs

Description
-----------

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>` man page.

For more details on the implementation, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_gpu_activity.html.

.. note::
    This is currently an experimental agent and is only available when
    building GEOPM with the ``--enable-beta`` flag. Some areas or aspects that
    are subject to change include its interface (e.g. the policy) and
    algorithm. It is also possible that this agent may be refactored and
    combined with other agents.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`
