.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_gpu_activity(7) -- agent for selecting GPU frequency based on GPU compute activity
=================================================================================================






DESCRIPTION
-----------

The goal of this Agent is to save GPU energy by scaling GPU frequency based upon
the compute activity of each GPU as provided by the ACCELERATOR_COMPUTE_ACTIVITY
signal and modified by the UTILIZATION_ACCELERATOR signal.

The **GPUActivityAgent** scales frequency in the range of Fe to Fmax, where Fmax
is provided via the policy as ``ACCELERATROR_FREQ_MAX`` and Fe is provided via
the policy as ``ACCELERATOR_FREQ_EFFICIENT``.  Low activity regions (compute activity
of 0.0) run at the Fe frequency, high activity regions (compute activity of 1.0)
run at the Fmax frequency, and regions in between the extremes run at a frequency
selected via
F = Fe + (Fmax - Fe) * ACCELERATOR_COMPUTE_ACTIVITY/UTILIZATION_ACCELERATOR.
``UTILIZATION_ACCELERATOR`` is used to scale the ``ACCELERATOR_COMPUTE_ACTIVITY`` in order
to scale frequency selection with the percentage of time a kernel is running on
the GPU.  This tends to help with workloads that contain short but highly
scalable GPU phases.

Fe is intended to be an energy efficient frequency that is selected via system
characterization.  The recommended approach to selecting Fe is to perform a
frequency sweep on the GPUs of interest using a workload that scales strongly with
frequency.  With this approach, Fe will be the frequency that provides the lowest
GPU energy consumption for the workload.

Fmax is intended to be the maximum allowable frequency, and may be set as the
default GPU maximum frequency, or limited based upon user/admin preference.

The GPUActivityAgent provides an optional input of phi that allows for biasing the
frequency range used by the agent.  The default phi value of 0.5 provides frequency
selection in the full range from Fe to Fmax.  A phi value less than 0.5 biases the
agent towards higher frequencies by increasing the Fe value provided by the policy.
In the extreme case (phi of 0) Fe will be raised to Fmax.  A phi value greater than
0.5 biases the agent towards lower frequencies by reducing the Fmax value provided
by the policy.  In the extreme case (phi of 1.0) Fmax will be lowered to Fe.

For NVIDIA based systems the GPUActivityAgent attempts to set the
DCGM::FIELD_UPDATE_RATE to 100ms, DCGM::MAX_STORAGE_TIME to 1s, and DCGM::MAX_SAMPLES
to 100.  While the DCGM documentation indicates that users should 'generally' query
no faster than 100ms, the interface allows for setting the polling rate in the
microsecond range. If the agent is intended to be used with workloads that exhibit
extremely short phase behavioor a 1ms polling rate can be used.
This has been shown to work for a small number of profiling metrics queried from DCGM.
As the 1ms polling rate is not officially recommended by the DCGM API the 100ms
setting is used by default.

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.

*
  **Agent Name**:

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to ``"gpu_activity"`` and the Controller will select the
      EnergyEfficientAgent for its control handler.  See
      `geopm_launch(1) <geopm_launch.1.html>`_ and `geopm(7) <geopm.7.html>`_ for more information about
      launch options and environment variables.

*
  **Agent Policy Definitions**:

      The Fe, Fmax, and  and maximum frequency are policy values.
      Setting both to the same value can be used to force the entire
      application to run at one frequency.

  ``ACCELERATOR_FREQ_MAX``\ :
      The maximum frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the
      maximum available frequency by default.

  ``ACCELERATOR_FREQ_EFFICIENT``\ :
      The minimum frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use
      (maximum frequency + minimum frequency) / 2 by default.

  ``ACCELERATOR_PHI``\ :
      The performance bias knob.  The value must be between
      0.0 and 1.0. If NAN is passed, it will use 0.5 by default.

*
  **Agent Sample Definitions**\ :
  N/A

*
  **Trace Column Extensions**\ :
  N/A

*
  **Report Extensions**\ :

      Per node extensions added to the report:
  ``Accelerator Frequency Requests``\ :
      The number of frequency requests made by the agent

   ``Resolved Max Frequency``\ :
      Fmax after phi has been taken into account

   ``Resolved Efficient Frequency``\ :
      Fe after phi has been taken into account

   ``Resolved Frequency Range``\ :
      The selection range of the agent after phi has been taken
      into account

      Per GPU extensions added to the report:
   ``Accelerator # Active Region Energy``\ :
       Per Accelerator GPU energy reading during the Region
       of Interest (ROI) where ROI is determined as the
       first sample of GPU activity to the last sample of GPU
       activity.
   ``Accelerator # Active Region Time``\ :
       Per Accelerator GPU time during the Region
       of Interest (ROI) where ROI is determined as the
       first sample of GPU activity to the last sample of GPU
       activity.
   ``Accelerator # Active Region Start Time``\ :
       Per Accelerator GPU start time for the Region
       of Interest (ROI) where ROI is determined as the
       first sample of GPU activity to the last sample of GPU
       activity.
   ``Accelerator # Active Region Start Time``\ :
       Per Accelerator GPU stop time for the Region
       of Interest (ROI) where ROI is determined as the
       first sample of GPU activity to the last sample of GPU
       activity.
*
  **Control Loop Gate**\ :

      The agent gates the Controller's control loop to a cadence of 20ms.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
