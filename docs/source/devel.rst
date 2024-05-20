Developer Guide
===============

If you wish to modify the source code in the GEOPM git repository, this guide
provide instructions for the process.

Before proceeding with the build steps, it's advisable to familiarize yourself
with the build requirements for the :doc:`GEOPM Access Service<requires>` and
:doc:`GEOPM Runtime Service<runtime>`.

Developer Build Process
-----------------------

To build all the software in the GEOPM repository, run the following commands
described in the repository README.md.

Upon successful build completion, if you wish to execute unit tests, each build
has a `check` target in the makefiles.  The `geopmdpy` and `geopmpy` directories
have a README.md that describes how to run the unit tests.

.. code-block:: bash

    cd libgeopmd
    make -j check
    cd ../libgeopm
    make -j check


Configuring the Build
---------------------

Several options can be passed to each of the two configure scripts that
determine the build process. The files managed by the scripts are responsible
for GEOPM's version embedding in build artifacts and create two `configure`
scripts using the autotools package.

The configure scripts output several files, including the `Makefile` used
for further build steps. These scripts also accept various command line
options and environmental variables that customize behavior. For detailed
user documentation, refer to the `./configure --help` command. Some notable
options and environment variables are listed below:

- Both configure scripts

* ``--prefix``: Path prefix for install artifacts
* ``--enable-debug``: Enable verbose error and warning messaging while disabling optimization.
* ``--enable-coverage``: Enable coverage report generation with gcov
* ``--enable-beta``: Enable beta features, which remain in beta until their
  interfaces are considered finalized and stable for future releases.
* ``export CC=``: Set the C compiler with environment variable
* ``export CXX=``: Set the C++ compiler with environment variable

- libgeopmd configure script

* ``--enable-nvml``: Adds support for the Nvidia NVML library
* ``--enable-dcgm``: Adds support for the Nvidia DCGM library
* ``--enable-levelzero``: Adds support for OneAPI LevelZero
* ``--disable-systemd``: Excludes GEOPM service access from PlatformIO
* ``--disable-io-uring``: Disable support for libiouring for batch IO operations

- Base configure script

* ``--with-geopmd=``: Specify the installation location of the service build
* ``--disable-mpi``: Excludes MPI dependencies from the base directory build
* ``--disable-fortran``: Excludes Fortran dependencies from the base directory build
* ``--disable-openmp``: Excludes OpenMP dependencies from the base directory build
* ``--disable-geopmd-local``: Use system installed geopmd package, do not use local service build
* ``export FC=``: Set the Fortran compiler with an environment variable
* ``export F77=``: Set the Fortran 77 compiler with an environment variable
* ``export MPICC=``: Set the MPI C compiler wrapper with an environment variable
* ``export MPICXX=``: Set the MPI C++ compiler wrapper with an environment variable
* ``export MPIFC=``: Set the Fortran compiler wrapper with environmental variable
* ``export MPIF77=``: Set the Fortran 77 compiler

Intel Compiler and MPI Toolchain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. TODO this section and runtime->Build Requirements need to be refactored.
   IMO all this text belongs in the runtime.rst.

To enable the use of the Intel toolchain for both the compiler and MPI support,
export the following variables prior to configuring the base build of the GEOPM
repository:

.. code-block:: bash

    export CC=icx
    export CXX=icpx
    export FC=ifx
    export F77=ifx
    export F90=ifx
    export MPICC=mpiicc
    export MPICXX=mpiicpc
    export MPIFORT=mpiifort
    export MPIFC=mpiifort
    export MPIF77=mpiifort
    export MPIFR90=mpiifort

We recommend using the system compiler toolchain for compiling the GEOPM service
when creating an installed RPM.  The ``make rpm`` target of the service
directory uses the geopm-service spec file to ensure that the system GCC
toolchain is used to create the RPM.

Coverage Instructions
---------------------

To generate a coverage report, first be sure that you have installed the lcov
package.  Note that if you are using GCC 9 or above, you must use lcov v1.15 or
later to work around `this issue
<https://github.com/linux-test-project/lcov/issues/58>`_.

The lcov source is available here:

https://github.com/linux-test-project/lcov

The GEOPM build must be configured with the "--enable-coverage" option prior to
running the tests.  Then in either the service directory or the root directory,
simply run

.. code-block::

   make coverage


which runs the corresponding unit tests and produces a coverage report in

.. code-block::

   ./coverage/index.html


Note that all tests must pass in order to generate a coverage report.
Any help in increasing code coverage levels is appreciated.

Coverage from the latest release is [posted to our web
page](http://geopm.github.io/coverage/index.html).

Coding Style
------------

Python code should follow the PEP8 standard as described in
https://peps.python.org/pep-0008/.

C++ code can be corrected to conform to the GEOPM standard using astyle with the
following options:

.. code-block::

   astyle --style=linux --indent=spaces=4 -y -S -C -N

Note that astyle is not perfect (in particular it is confused by C++11
initializer lists), and some versions of astyle will format the code slightly
differently.

Use C style variable names with lower case and underscores.  Upper camel case is
used exclusively for class names.  Prefix all member variables with "m\ *" and
all global variables with "g*\ ".

Please avoid global variables as much as possible and if it is necessary to use
a global (primarily for C code) please scope them statically to the compilation
unit.

Avoid preprocessor macros as much as possible (use enum not #define).
Preprocessor usage should be reserved for expressing configure time options.

The number of columns in a source file should not exceed 70 or 80 before
wrapping the line.  Exceptions are allowed when it is required for compilation
or similar.  In general, follow the style in the file you are modifying.

Pre-Commit Checks
-----------------

This repository includes a configuration for `pre-commit
<https://pre-commit.com/>`_ that uses some of their standard hooks that are
relevant to GEOPM, and adds a hook that performs the GEOPM license checks.

To install the pre-commit infrastructure and our configuration::

    pip install pre-commit
    pre-commit install

Now you will automatically run some checks whenever you make a commit, instead
of waiting until you make a pull request to see all of them.

License Headers
---------------

Introducing a new file requires a license comment in its header with a
corresponding file.  Any new installed files should also be added to the package's
`.spec` file and a `debian/*.install` file.

Creating Manuals
----------------

Introducing a new man page requires changes in multiple files:

#.
   The build target (man page) should be added to rst_file in conf.py
#.
   The gzipped installed man page should be listed in the %files section of
   geopm-doc.spec.in
#.
   A link to the new html page should be added to the SEE ALSO section of
   geopm.7.rst and any other related man pages.

.. note::
    In addition, new documentation should follow the style guidelines defined here:

    .. toctree::
       :maxdepth: 1

       docu
