GEOPM AGENT TUTORIAL
====================

This directory contains a tutorial on extending the behavior of the GEOPM
runtime by creating an Agent that applies a different power cap to each host,
based on caps specified in an environment variable. The agent does not expose
any signals or controls. An agent that uses more features of the GEOPM
framework can be found in $GEOPM_ROOT/tutorial/agent.

Run with GEOPM
--------------

Specify the HostPowerGovernor as the Agent for the Controller by setting
GEOPM_AGENT=host_power_governor. The name of the Agent should match the name
returned by plugin_name() used for registration. This Agent does not use a
policy file. Instead, ensure that a value is set for the
GEOPM_HOST_POWER_LIMITS environment variable when geopmlaunch is run.

The GEOPM_HOST_POWER_LIMITS variable takes a list of (hostname, power limit)
pairs. Pairs are separated by commas, and pair elements are separated by an
equals sign. For example, to place a 180 W limit (summed across packages) on
host1, and a 190 W limit on host2, use the following:

`GEOPM_HOST_POWER_LIMITS='host1=180,host2=190'`

If any other hosts are encountered, the agent will limit them at their TDP
power levels by default.

Note that to be recognized as an agent plugin, the shared library filename must
begin with "libgeopmagent_" and end in ".so.0.0.0". Be sure that the
HostPowerGovernor plugin is in GEOPM_PLUGIN_PATH.

An example run script is provided in host_power_governor_tutorial.sh. It uses
the geopmbench application. Before running, build and install GEOPM and make
sure that the plugin is built using the "tutorial_build_\*.sh" script in the
host_power_goveror folder. The script does not set host power limits, so the 
default behavior should be observed: each host's limit is TDP. Run again
with explicit limits, for example:

    GEOPM_LAUNCHER='srun' GEOPM_HOST_POWER_LIMITS='mycappedhost=180' ./host_power_governor_tutorial.sh

After the run completes, inspect the generated report file. The report should
show that the agent used was "host_power_governor" and you should see the
agent-specific details in the host sections of the report. The trace and the
report should indicate the time-series and average package power consumption at
each node, respectively. Verify that these values do not exceed the configured
per-host power limits.
