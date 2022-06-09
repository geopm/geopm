.. role:: raw-html-m2r(raw)
   :format: html


geopm::GPUActivityAgent(3) -- agent for selecting GPU frequency based on GPU compute activity
=========================================================






NAMESPACES
----------

The ``GPUActivityAgent`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::GPUActivityAgent``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``GPUActivityAgent`` class is derived from `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`_ class.

SYNOPSIS
--------

#include `<geopm/GPUActivityAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/GPUActivityAgent.hpp>`_\

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**

Requires ``-enable-nvml`` and ``-enable-dcgm`` on systems with NVIDIA GPUs

DESCRIPTION
-----------

The behavior of this agent is described in more detail in the
`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7.html>`_ man page.

For more details on the implementation, see the doxygen
page at <https://geopm.github.io/dox/classgeopm_1_1_gpu_activity.html>.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_
