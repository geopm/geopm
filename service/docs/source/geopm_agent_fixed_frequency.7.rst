
geopm_agent_fixed_frequency(7) -- agent for setting fixed CPU, Uncore, and Accelerator frequencies
===================================================================================================

Description
-----------

The fixed frequency agent adjusts the CPU, Uncore, and Accelerator
frequencies based upon the agent policy.  The Agent policy provides a
single frequency value for all three frequency domains.  The uncore
policy value is used to set both the minimum and maximum.  If the
policy value for the CPU or Uncore is NAN, the frequency of that
domain is not modified.  If the policy value for the Accelerator is
NAN, the maximum Accelerator frequency is used.

Agent Name
----------

The agent described in this manual is selected in many geopm
interfaces with the ``"fixed_frequency"`` agent name.  This name can
be passed to :doc:`geopmlaunch(1) <geopmlaunch.1>` as the argument to
the ``--geopm-agent`` option, or the ``GEOPM_AGENT`` environment
variable can be set to this name (see :doc:`geopm(7) <geopm.7>`).
This name can also be passed to the :doc:`geopmagent(1)
<geopmagent.1>` as the argument to the ``'-a'`` option.

Policy Parameters
-----------------

  ``GPU_FREQUENCY``\ :
      The operating frequency in units of *Hz* for the Board
      Accelerators.  Setting it to a NAN will result in the system
      default value.

  ``CPU_FREQUENCY``\ :
      The operating frequency in units of *Hz* for all CPUs. Setting
      it to a NAN will result in the system default value.

  ``UNCORE_MIN_FREQUENCY``\ :
      The min operating frequency in units of *Hz* for the uncore
      domain.  If specified the uncore clock will operate with lower
      min range of the fixed frequency provided.  If the parameter is
      NAN, then the system default range of uncore frequency will be
      allowed.

  ``UNCORE_MAX_FREQUENCY``\ :
      The max operating frequency in units of *Hz* for the uncore
      domain.  If specified the uncore clock will operate with upper
      max range of the fixed frequency provided.  If the parameter is
      NAN, then the system default range of uncore frequency will be
      allowed.

  ``SAMPLE_PERIOD``\ :
      The sample period in units of seconds.  If the parameter is
      NAN, then the default period is 5e-3 (.005) (5ms).

Policy Requirements
-------------------

The fixed frequency values must be in the order of GPU_FREQUENCY,
CPU_FREQUENCY, UNCORE_MIN_FREQUENCY, UNCORE_MAX_FREQUENCY,
SAMPLE_PERIOD.  A value must be provided, but can be NAN.

Report Extensions
-----------------

None.

Control Loop Rate
-----------------

The agent gates the control loop to sample signals for the geopm
report specified by SAMPLE_PERIOD.  The default is 5ms.

Examples
--------

The recommended way to generate a policy file for this agent is to use
the :doc:`geopmagent(1) <geopmagent.1>` command line tool.

To create a policy with an accelerator frequency of *1.53 GHz*, a core
frequency of *3.1 GHz*, and an uncore frequency of *2 GHz*

.. code-block::

    geopmagent -a fixed_frequency -p '1.530e9,3.1e9,2e9,2.7e9,5e-2'

Although the :doc:`geopmagent(1) <geopmagent.1>` is the recommended
tool for creating the json policy string, the json string without this
tool can be used.  For example, the above policy json is:

.. code-block:: json

    {"GPU_FREQUENCY": 1530000000,
     "CORE_FREQUENCY": 3100000000,
     "UNCORE_MIN_FREQUENCY": 2000000000,
     "UNCORE_MAX_FREQUENCY": 2700000000,
     "SAMPLE_PERIOD": 0.05}

The :doc:`geopmread(1) <geopmread.1>` command line tool can be useful
for learning the bounds of these system parameters.

The minimum and maximum accelerator frequencies are queried as below:

.. code-block:: bash

    $ geopmread GPU_FREQUENCY_MIN_AVAIL board 0
    135000000

    $ geopmread GPU_FREQUENCY_MAX_AVAIL board 0
    1530000000

The minimum, sticker, and maximum CPU frequencies are queried as
below:

.. code-block:: bash

    $ geopmread FREQUENCY_MIN board 0
    1000000000

    $ geopmread FREQUENCY_STICKER board 0
    2400000000

    $ geopmread FREQUENCY_MAX board 0
    3700000000

The minimum, and maximum uncore frequencies are queried as below:

.. code-block:: bash

    $ geopmread MSR::UNCORE_RATIO_LIMIT:MIN_RATIO board 0
    1200000000

    $ geopmread MSR::UNCORE_RATIO_LIMIT:MAX_RATIO board 0
    2400000000


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`,
:doc:`geopm_agent(3) <geopm_agent.3>`,
:doc:`geopmagent(1) <geopmagent.1>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
