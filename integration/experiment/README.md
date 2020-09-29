Scripts are grouped into sub-folders based on the type of
experiment, such as a sweep across power limits or processor
frequency settings.  In each experiment directory there should be
a python script with a name that matches the experiment directory
name.  This script contains common code used by the other scripts
in the directory.  These other scripts are broken into two
categories: run scripts which are prefixed with "run_" and
analysis scripts which are prefixed with "gen_".

Each run script is specific to an application, and an application
may have several run scripts that describe different application
configurations.  This is because applications have specific
runtime requirements and are scaled to different node and rank
counts.  The base experiment script describes output requirements
for the run scripts: whether traces required for the analysis and
which extra signals are added to the reports and traces.  In this
way the output data from all of the run scripts will meet the
requirements of all of the analysis scripts.  Each analysis script
is designed to generate particular summary tables or plots based
on output reports and traces located in a user provided directory.
