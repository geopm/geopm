GEOPM Environment Configuration
===============================

This directory contains a collection of `bash` scripts that may be sourced to
set up a user's environment, as well as `build.sh` which is used to automate
the use of the build systems. Scripts with the `_env.sh` suffix are not
intended to be executed directly (this will have no effect), but instead may be
sourced within a user's shell in order to build GEOPM or to run against a
locally installed version of GEOPM.

Please refer to the [README](../README.md) in the parent directory for
information about how to define environment variables relevant to these
scripts in your `.geopmrc` file.


`australis_env.sh`
------------------

A `bash` script that may be sourced within a shell in order to set up
the user's build/run environment on the australis system.


`build.sh`
----------

An executable script that will automate use of the GEOPM and GEOPM Service
build systems.  This script provides documentation if run with the `--help`
command line option.


`build_env.sh`
--------------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment.  By default this will use the Intel
compiler toolchain.


`dudley_env.sh`
---------------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment on the dudley system.


`endeavor_env.sh`
-----------------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment on the endeavor system.


`gnu_env.sh`
----------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment to use the GNU compiler toolchain.


`run_env.sh`
----------

A `bash` script that may be source within a shell in order to set up the
user's run environment to use a locally installed version of the GEOPM
packages.


`smng_env.sh`
-----------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment on the SuperMUC-NG system.


`theta_env.sh`
------------

A `bash` script that may be sourced within a shell in order to set up
the user's build environment on the Theta system.
