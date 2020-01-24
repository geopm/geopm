GEOPM TUTORIAL FOR SYSTEM ADMINISTRATORS
========================================
This README presents a tutorial for system administrators who
would like to configure their HPC systems to enable the use of GEOPM.
There are several different use cases that are broken out into
numbered sections below.  The tutorial starts out with the least
restrictive configuration of GEOPM.  The other sections show ways that
system administrators can use GEOPM to achieve specific goals for
energy, power, and performance.

The directory contains scripts that can be used to configure systems
for use with the GEOPM runtime.  The scripts associated with each
section will be numbered accordingly.

0. Enable users to do research with the GEOPM runtime
-----------------------------------------------------

Follow the instructions in "RUNTIME REQUIREMENTS" section of the
top level GEOPM README.md.
Test scripts to check the system requirements have been met can be found in XXXX.

### Prerequisites for different scenarios

- power capping
    - description
    - test: see is_rapl_working.sh
    - test: check that msr-safe, etc. is set up
- frequency limit
    - description
    - test


1. Restrict CPU frequency for jobs that do not use the GEOPM runtime
----------------------------------------------------------------------
In this scenario, the CPU frequency of all nodes in the job will be set
to a fixed value at the beginning of every job.  This set up can be used
on system to save energy by reducing the processor frequency below the
base frequency (also known as "sticker frequency").  This frequency
can be obtained by running `geopmread FREQ_STICKER board 0` on a node
of the target architecture for the configuration.

See the script "setup_fixed_frequency.sh" in this folder for an example of how to set up
a system to run all jobs at 300 MHz below the sticker frequency.  The
test script XXXX can be used to check whether this setup is working.

2. Set the energy efficient agent as the default agent
------------------------------------------------------
When users run jobs with GEOPM, the built-in default agent is the monitor
(no controls, monitoring only).  To change this default to the energy
efficient agent so that jobs launched with GEOPM optimize energy efficiency,
see the script "setup_default_energy_efficient.sh".  In this example, jobs
not using GEOPM will run not have their frequency changed.  The test
script XXXX can be used to check whether this setup is working.

3. Restrict power limit for jobs that do not use the GEOPM runtime
------------------------------------------------------------------
In this scenario, the CPU frequency of all nodes in the job will be set
to a fixed value at the beginning of every job.  This set up can be used
on system to save energy by reducing the processor frequency below the
base frequency (also known as "sticker frequency").  This frequency
can be obtained by running `geopmread FREQ_STICKER board 0` on a node
of the target architecture for the configuration.

See the script "setup_fixed_power_cap.sh" in this folder for an example of how to set up
a system to run all jobs at 50 watts below TDP.  The
test script XXXX can be used to check whether this setup is working.


4. Set the power balancing agent as the default agent and power cap other jobs
------------------------------------------------------------------------------
When users run jobs with GEOPM, the built-in default agent is the monitor
(no controls, monitoring only).  To change this default to the power balancer
agent so that jobs launched with GEOPM optimize performance under a power cap,
see the script "setup_default_power_balancer.sh".  In this example, jobs
not using GEOPM will have their power cap set to a fixed cap across all nodes.
The test script XXXX can be used to check whether this setup is working.


5. Restrict CPU frequency or enforce energy efficient agent
-----------------------------------------------------------

6. Restrict power cap or enforce power balancing agent
------------------------------------------------------

7. Control location of report and trace file output
---------------------------------------------------
Unsupported future feature.

8. Restrict the maximum size of trace file generation
-----------------------------------------------------
Unsupported future feature.