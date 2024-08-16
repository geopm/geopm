geopm_agent_frequency_balancer(7) -- Agent reduces imbalance across CPU cores
=============================================================================
.. meta::

    :description: The GEOPM frequency_balancer agent reduces imbalance across
                  CPU cores through frequency controls and performance-guided
                  turbo limits.
    :keywords: frequency balancer agent imbalance SST-TF P-States

Description
-----------
The ``frequency_balancer`` agent reduces imbalance across CPU cores.
The executing application's main compute loop must be annotated with
``geopm_prof_epoch()`` from :doc:`geopm_prof(3) <geopm_prof.3>`. The agent
assumes that core application time can be measured as time spent outside of
networking regions of code. CPU cores that spend less networking time per
epoch are allocated higher CPU frequency limits, while cores that spend more
networking time per epoch are allocated lower CPU frequency limits.

Allocating CPU Core Frequency Limits
------------------------------------
The agent estimates a running application's critical path in each epoch,
estimates the best-case critical path time the agent can make the application
achieve (sometimes with energy savings, sometimes with performance improvement),
and applies frequency control settings that the agent estimates will let each
core achieve the target best-case time.

The following subsections outline the agent's responsibilities at a high level.
See `Guiding Hardware-Driven Turbo with Application Performance Awareness <https://ieeexplore.ieee.org/abstract/document/9969356>`_
for more details.

Critical Path Time Estimates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The agent measures the non-networking time as time that each core spends with
the ``GEOPM_REGION_HINT_NETWORK`` hint *inactive* in each epoch. By default,
GEOPM annotates all MPI function calls with that hint. The agent aims to set
frequency controls that reduce the variation of non-networking time across
cores.

Frequency Control Decisions
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Frequency control decisions applied to one core may impact the achievable
frequency on other cores. In general, if the platform throttles frequency
lower than a user's requested frequency due to resource constraints, then
throttling other cores may lessen the need for platform-induced throttling.
Furthermore some platforms support heterogeneous turbo frequency limits, where
the number of cores configured with high turbo limits impacts the maximum
achievable frequency. The agent performs estimates of the application
performance tradeoffs in these cases.

Heterogeneous Turbo Frequency Limits
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Turbo frequency boosting allows a CPU to temporarily achieve higher than its
base frequency while still respecting hardware design constraints. Typically,
higher turbo frequency is achievable when only one CPU core is active (i.e.,
*single-core turbo*) than when all CPU cores are active (i.e., *all-core
turbo*). Some platforms support heterogeneous user-configurable turbo frequency
limits through the *Intel(R) Speed Select Technology - Turbo Frequency*
(SST-TF) feature.

This agent queries the platform to learn the frequency trade-offs of different
possible SST-TF configurations. The agent estimates the potential critical
path time for each possible configuration, and applies the configuration that
results in the least estimated time.

Agent Configuration
-------------------
Execute an application with this agent by specifying
``--geopm-agent=frequency_balancer`` in the :doc:`geopmlaunch(1) <geopmlaunch.1>`
command line arguments.

The agent is capable of influencing CPU core performance through P-State
settings and through heterogeneous turbo frequency limits via SST-TF. Each
type of control can be used in isolation, or they can be used together.
If SST-TF is requested on a platform that does not support that feature, then
the request to use SST-TF is ignored.

Specify the desired types of performance influencers through a policy json file
and indicate the location of the file with ``--geopm-policy=<path-to-file>``.
The :doc:`geopmagent(1) <geopmagent.1>` tool can help construct a policy file.

Agent Policy Definitions
------------------------
This agent's behavior is influenced by the following GEOPM policy fields. At
least one balancing option must be set to ``1``.

``USE_FREQUENCY_LIMITS``
    Set to ``1`` if you want the agent to use per-core P-States to help balance
    the application. Default: ``1``
``USE_SST_TF``
    Set to ``1`` if you want the agent to use SST-TF to help balance the
    application. Default: ``1``

If both P-States and SST-TF are used, then the agent uses both per-core
P-States and SST-TF controls to help balance the application.

Trace Column Extensions
-----------------------
This agent adds the following columns to GEOPM trace files:

``NON_NET_TIME_PER_EPOCH-core-<core>``
    This collection of columns indicates the amount of non-networking time the
    agent measured for each core in the epoch prior to the in-progress epoch.
    This is the time metric that the agent attempts to balance across cores
    within each CPU package.
``DESIRED_NON_NETWORK_TIME-package-<package>``
    This collection of columns indicates the agent's estimate for the most time
    any core will spend in non-networking code for the current epoch in each
    package.

Report Extensions
-----------------
This agent adds the following fields to the root level of GEOPM reports:

``Agent uses frequency control``
    Indicates whether the agent used P-State frequency limits to manipulate
    application performance across CPU cores.
``Agent uses SST-TF``
    Indicates whether the agent used SST-TF heterogeneous turbo limits to
    manipulate application performance across CPU cores. This may indicate
    ``0`` even if ``USE_SST_TF`` was set to ``1`` in the policy if the platform
    does not support SST-TF, or if the agent is otherwise not able to use that
    feature on the platform.

Control Loop Gate
-----------------
The agent limits each step of the the GEOPM Controller's main loop to execute
no faster than once every *5 ms*.

See Also
--------
:doc:`geopm(7) <geopm.7>`,
:doc:`geopmagent(1) <geopmagent.1>`,
:doc:`geopm_agent(3) <geopm_agent.3>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`,
:doc:`geopm_prof(3) <geopm_prof.3>`
