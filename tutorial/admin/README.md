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
```GEOPM_AGENT: frequency_map```

2. Set the energy efficient agent as the default agent
----------------------------------------------------------

3. Restrict power limit for jobs that do not use the GEOPM runtime
------------------------------------------------------------------
```GEOPM_AGENT: power_governor```

4. Set the power balancing agent as the default agent
-----------------------------------------------------

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