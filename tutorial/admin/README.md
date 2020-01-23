GEOPM TUTORIAL FOR SYSTEM ADMINISTRATORS
========================================
This documentation presents a tutorial for system administrators who
would like to configure their HPC systems to enable the use of GEOPM.
There are several different use cases that are broken out into
numbered sections below.  The tutorial starts out with the least
restrictive configuration of GEOPM.  The other sections show ways that
system administrators can use GEOPM to achieve specific goals for
energy, power and performance.

The directory contains scripts that can be used to configure systems
for use with the GEOPM runtime.  The scripts associated with each
section will be numbered accordingly.

0. Enable users to do research with the GEOPM runtime
-----------------------------------------------------
Follow the instructions in "RUNTIME REQUIREMENTS" section of the
top level GEOPM README.md.

1. Restrict CPU frequency for jobs that do not use the GEOPM runtime
--------------------------------------------------------------------

2. Set the energy efficient agent as the default agent
------------------------------------------------------

3. Restrict CPU frequency or enforce energy efficient agent
-----------------------------------------------------------

4. Restrict power cap or enforce power balancing agent
------------------------------------------------------

5. Control location of report and trace file output
---------------------------------------------------

6. Restrict the maximum size of trace file generation
-----------------------------------------------------