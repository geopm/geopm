# System Characterization

This directory contains scripts and utilities for characterizing system hardware
and generating node configuration files for GEOPM agents. The tools here are used to
automate frequency and power sweeps, analyze performance and energy data, and
produce recommended settings for CPU and GPU activity agents.

## Contents

- **gen_cpu_activity_constconfig_recommendation.py**: Python script to analyze
  CPU frequency sweep results and generate ConstConfigIO recommendations for the
  CPU Activity Agent.

- **gen_gpu_activity_constconfig_recommendation.py**: Python script to analyze
  GPU frequency sweep results and generate ConstConfigIO recommendations for the
  GPU Activity Agent.

- **host_config.py**: Utility to combine host-specific configuration files into
  a single configuration.

- **set_env.sh**: This file contains some of the environment variables necessary
  both while characterizing the nodes and generating the frequency efficient values.
  Relevant sections in the file can be (un)commented depending on whether the file
  is being sourced in an environment with a local or a default GEOPM installation.

- **set_vars.sh**: Sample file listing all the input variables that are used by
  the node characterization scripts.

- **sys_charac_freq_sweeps.sh**: Wrapper script to facilitate CPU & GPU frequency
  sweeps independent of platform-level power caps.

- **sys_charac_platfom_sweeps.sh**: Wrapper script to facilitate CPU & GPU frequency
  sweeps under multiple platform-level power caps.

- **sys_charac_utils.sh**: Bash script containing utility functions to setup the
  run environment and automate the launch of CPU and GPU frequency sweeps.

- **test_host_config.py**: Unit tests for the `host_config.py` utility.

These tools are intended for use in benchmarking and tuning system parameters to
optimize energy efficiency and performance with GEOPM.
