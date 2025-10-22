# System Characterization

This directory contains scripts and utilities for characterizing system hardware
and generating node configuration files for GEOPM agents. The tools here are used to
automate frequency and power sweeps, analyze performance and energy data, and
produce recommended settings for CPU and GPU activity agents.

## Contents

- **derive_energy.py**: Script to calculate energy consumed by a specific
  region from the report.
   
- **gen_cpu_activity_constconfig_recommendation.py**: Python script to analyze
  CPU frequency sweep results and generate ConstConfigIO recommendations for the
  CPU Activity Agent.

- **gen_gpu_activity_constconfig_recommendation.py**: Python script to analyze
  GPU frequency sweep results and generate ConstConfigIO recommendations for the
  GPU Activity Agent.

- **gen_plot_heatmap.py**: Script to generate a core-uncore heat map under a given
  power cap

- **host_config.py**: Utility to combine host-specific configuration files into
  a single configuration.

- **run_aib.sh**: Run the AIB benchmark and generate a GEOPM report.

- **run_stream.sh**: Run the Stream benchmark with the GEOPM runtime and generate
  a GEOPM report.

- **run_stream_wort.sh**: Run the Stream benchmark without the GEOPM runtime
  and display the elapsed time.

- **set_vars.sh**: Sample file listing all the input variables that are used by
  the node characterization scripts.

- **sys_charac_freq_sweeps.sh**: Wrapper script to facilitate CPU & GPU frequency
  sweeps independent of platform-level power caps.

- **sys_charac_platfom_sweeps.sh**: Wrapper script to facilitate CPU & GPU frequency
  sweeps under multiple platform-level power caps.

- **test_host_config.py**: Unit tests for the `host_config.py` utility.

- **utils.sh**: Bash script containing utility functions to setup the
  run environment and automate the launch of CPU and GPU frequency sweeps.


These tools & scripts are intended for use in benchmarking and tuning system
parameters to optimize energy efficiency and performance with GEOPM.
