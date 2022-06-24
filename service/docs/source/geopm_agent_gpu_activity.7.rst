
geopm_agent_gpu_activity(7) -- agent for selecting GPU frequency based on GPU compute activity
=================================================================================================






Description
-----------

The goal of this Agent is to save GPU energy by scaling GPU frequency based upon
the compute activity of each GPU as provided by the GPU_COMPUTE_ACTIVITY
signal and modified by the GPU_UTILIZATION signal.

The **GPUActivityAgent** scales frequency in the range of ``Fe`` to ``Fmax``, where ``Fmax``
is provided via the policy as ``GPU_FREQ_MAX`` and ``Fe`` is provided via
the policy as ``GPU_FREQ_EFFICIENT``.  Low activity regions (compute activity
of 0.0) run at the ``Fe`` frequency, high activity regions (compute activity of 1.0)
run at the ``Fmax`` frequency, and regions in between the extremes run at a frequency (F)
selected using the equation:

``F = Fe + (Fmax - Fe) * GPU_COMPUTE_ACTIVITY/GPU_UTILIZATION``

``GPU_UTILIZATION`` is used to scale the ``GPU_COMPUTE_ACTIVITY`` in order
to scale frequency selection with the percentage of time a kernel is running on
the GPU.  This tends to help with workloads that contain short but highly
scalable GPU phases.

``Fe`` is intended to be an energy efficient frequency that is selected via system
characterization.  The recommended approach to selecting ``Fe`` is to perform a
frequency sweep on the GPUs of interest using a workload that scales strongly with
frequency.  With this approach, ``Fe`` will be the frequency that provides the lowest
GPU energy consumption for the workload.

``Fmax`` is intended to be the maximum allowable frequency, and may be set as the
default GPU maximum frequency, or limited based upon user/admin preference.

The GPUActivityAgent provides an optional input of ``phi`` that allows for biasing the
frequency range used by the agent.  The default ``phi`` value of 0.5 provides frequency
selection in the full range from ``Fe`` to ``Fmax``.  A ``phi`` value less than 0.5 biases the
agent towards higher frequencies by increasing the ``Fe`` value provided by the policy.
In the extreme case (``phi`` of 0) ``Fe`` will be raised to ``Fmax``.  A ``phi`` value greater than
0.5 biases the agent towards lower frequencies by reducing the ``Fmax`` value provided
by the policy.  In the extreme case (``phi`` of 1.0) ``Fmax`` will be lowered to ``Fe``.

For NVIDIA based systems the GPUActivityAgent attempts to set the
``DCGM::FIELD_UPDATE_RATE`` to 100 ms, ``DCGM::MAX_STORAGE_TIME`` to 1 s, and ``DCGM::MAX_SAMPLES``
to 100.  While the DCGM documentation indicates that users should generally query
no faster than 100 ms, the interface allows for setting the polling rate in the
microsecond range. If the agent is intended to be used with workloads that exhibit
extremely short phase behavior a 1 ms polling rate can be used.
This has been shown to work for a small number of profiling metrics queried from DCGM.
As the 1 ms polling rate is not officially recommended by the DCGM API the 100 ms
setting is used by default.

Agent Name
----------

The agent described in this manual is selected in many geopm
interfaces with the ``"gpu_activity"`` agent name.  This name can be
passed to :doc:`geopmlaunch(1) <geopmlaunch.1>` as the argument to the ``--geopm-agent``
option, or the ``GEOPM_AGENT`` environment variable can be set to this
name (see :doc:`geopm(7) <geopm.7>`\ ).  This name can also be passed to the
:doc:`geopmagent(1) <geopmagent.1>` as the argument to the ``'-a'`` option.

Policy Parameters
-----------------

The ``Fe``, ``Fmax``, ``phi``, and agent sample period are provided
as policy values.  Setting ``Fe`` & ``Fmax`` to the same value will
result in the entire application to run at a fixed frequency.


  ``GPU_FREQ_MAX``\ :
      The maximum frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the
      maximum available frequency by default.

  ``GPU_FREQ_EFFICIENT``\ :
      The minimum frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use
      (maximum frequency + minimum frequency) / 2 by default.

  ``GPU_PHI``\ :
      The performance bias knob.  The value must be between
      0.0 and 1.0. If NAN is passed, it will use 0.5 by default.

  ``SAMPLE_PERIOD``\ :
      The rate at which the agent control loop operates.  20ms by
      default.

Report Extensions
-----------------

  ``GPU Frequency Requests``\ :
      The number of frequency requests made by the agent

  ``Resolved Max Frequency``\ :
     ``Fmax`` after ``phi`` has been taken into account

  ``Resolved Efficient Frequency``\ :
     ``Fe`` after ``phi`` has been taken into account

  ``Resolved Frequency Range``\ :
     The frequency selection range of the agent after ``phi`` has
     been taken into account

  ``GPU # Active Region Energy``\ :
     Per GPU energy reading during the Region
     of Interest (ROI) where ROI is determined as the
     first sample of GPU activity to the last sample of GPU
     activity.
  ``GPU # Active Region Time``\ :
     Per GPU time during the Region
     of Interest (ROI) where ROI is determined as the
     first sample of GPU activity to the last sample of GPU
     activity.
  ``GPU # Active Region Start Time``\ :
     Per GPU start time for the Region
     of Interest (ROI) where ROI is determined as the
     first sample of GPU activity to the last sample of GPU
     activity.
  ``GPU # Active Region Stop Time``\ :
     Per GPU stop time for the Region
     of Interest (ROI) where ROI is determined as the
     first sample of GPU activity to the last sample of GPU
     activity.

Control Loop Rate
-----------------

The agent gates the control loop to a cadence of 20ms.

SEE ALSO
--------

:doc:`geopm(7) <geopm.7.html>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`\ ,
:doc:`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`\ ,
:doc:`geopm_agent_c(3) <geopm_agent_c.3.html>`\ ,
:doc:`geopm_prof_c(3) <geopm_prof_c.3.html>`\ ,
:doc:`geopmagent(1) <geopmagent.1.html>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1.html>`
