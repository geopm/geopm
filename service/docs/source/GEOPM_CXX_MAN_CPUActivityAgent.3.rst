geopm::CPUActivityAgent(3) -- agent for selecting CPU frequency based on CPU compute activity
=============================================================================================

Synopsis
--------

#include `<geopm/CPUActivityAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/CPUActivityAgent.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**

Description
-----------

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_cpu_activity(7) <geopm_agent_cpu_activity.7>` man page.

For more details on the implementation, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_cpu_activity.html.

.. note::
    This is currently an experimental agent and is only available when
    building GEOPM with the ``--enable-beta`` flag. Some areas or aspects that
    are subject to change include its interface (e.g. the policy) and
    algorithm. It is also possible that this agent may be refactored and
    combined with other agents.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_cpu_activity(7) <geopm_agent_cpu_activity.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`
