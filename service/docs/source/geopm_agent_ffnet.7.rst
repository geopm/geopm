
geopm_agent_ffnet(7) -- agent for adjusting frequencies based on application behavior
=====================================================================================

Description
-----------

.. note::
    This is currently an experimental agent and is only available when
    building GEOPM with the ``--enable-beta`` flag. The agent is subject
    to change, including being combined with other agents. Also, this agent 
    requires a neural net JSON file, the format of which is described by 
    the schema domainnetmap_neural_net.schema.json and a region hint 
    recommender JSON file described by the schema
    regionhintrecommender_fmap.schema.json. Without these inputs, the agent 
    will throw an error. The autogeneration of these files is future work.

The FFNet agent adjusts frequencies per domain for the goal of improved energy
efficiency with minimal performance loss. The agent instantiates a neural net
per domain that ingests hardware telemetry and outputs a probability distribution
of region classes. This probability distribution is used to determine an optimal
per-domain frequency. The agent policy can specify an energy-performance bias which
determines the degree to which the frequency recommender is adverse to potentially
reducing performance by reducing frequency to save energy.

The neural net for region classification must be provided in a JSON configuration
file pointed to by environment variables GEOPM_CPU_NN_PATH and/or GEOPM_GPU_NN_PATH, 
which must comply with the following schema:

.. literalinclude:: ../../../json_schemas/domainnetmap_neural_net.schema.json
    :language: json

The per-region frequency recommendations must be provided in a JSON configuration
file pointed to by environment variables GEOPM_CPU_FMAP_PATH and/or GEOPM_GPU_FMAP_PATH, 
which must comply with the following schema:

.. literalinclude:: ../../../json_schemas/regionhintrecommender_fmap.schema.json
    :language: json

If you specify a neural net for a domain (CPU/GPU), you must specify a frequency 
recommendation as well.

This agent can be used at the package scope to control CPU frequency
and/or at the per-GPU scope to control GPU frequency.

Agent Name
----------

The agent described in this manual is selected in many geopm
interfaces with the ``"ffnet"`` agent name.  This name can be
passed to :doc:`geopmlaunch(1) <geopmlaunch.1>` as the argument to the ``--geopm-agent``
option, or the ``GEOPM_AGENT`` environment variable can be set to this
name (see :doc:`geopm(7) <geopm.7>`\ ).  This name can also be passed to the
:doc:`geopmagent(1) <geopmagent.1>` as the argument to the ``'-a'`` option.

Policy Parameters
-----------------

  ``PERF_ENERGY_BIAS`` \:
      A value between [0-1] that determines how aggressively
      the agent will reduce frequency in order to save energy.
      A value of 0 indicates that no performance loss can be
      tolerated, while a value of 1 indicates that energy
      efficiency is paramount. Note that there are no absolute
      guarantees about performance; rather, the general trend
      of performance impact and energy efficiency will be
      monotonic relative to this bias.


Policy Requirements
-------------------

The ``PERF_ENERGY_BIAS`` must be between 0 and 1.

Report Extensions
-----------------

N/A

Control Loop Rate
-----------------

The agent gates the control loop to sample hardware telemetry and 
control frequency at 20 millisecond intervals.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
