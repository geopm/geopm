
Guide for GEOPM Developers
==========================

These are instructions for a developer that would like to modify the
source code in the GEOPM git repository.  The GEOPM repository
contains two independent autotools based build systems that are used
to compile, test and install software components written in C++ and
Python.  The ``service`` subdirectory of the GEOPM repository contains
all files related to the GEOPM systemd service including the build
system and all source code for the software components.  The base
directory of the GEOPM git repository is populated with a build system
that supports all sofware components not located in the ``service``
directory.  The base build depends soley on components in the
``service`` directory that are installed by the service build
including: the ``libgeopmd.so`` library, the C and C++ public
interface header files for that library, and the ``geopmdpy`` Python
module.


Developer Build Process
-----------------------

The basic procedure for building all of the software in the GEOPM
repository is to run the following commands with the base of the GEOPM
repository:

.. code-block:: bash

    # Build the geopm-service package
    cd service/
    ./autogen.sh
    ./configure
    make

    # Optionaly, build the geopm HPC runtime package
    cd ..
    ./autogen.sh
    ./configure
    make


After the build is complete, a developer may wish to execute the unit
tests.  Each of the two builds have a ``check`` target for their
makefiles.  The test programs may be build separately from the
``check`` target by specifying the ``checkprogs`` make target.

.. code-block:: bash

    # Run the geopm-service package unit tests
    cd service/
    make checkprogs
    make check

    # Optionally run the geopm HPC runtime package unit tests
    cd ..
    make checkprogs
    make check


The developer may be interested in installing the build artifacts to a
separate directory.  In this case, the build process differs slightly:
some extra options will be provided to configure.

.. code-block:: bash

    # Define the install location
    GEOPM_INSTALL=$HOME/build/geopm

    # Build the geopm-service package
    cd service/
    ./autogen.sh
    ./configure --prefix=$GEOPM_INSTALL
    make
    make install

    # Optionally, build the geopm HPC runtime package
    cd ..
    ./autogen.sh
    ./configure --prefix=$GEOPM_INSTALL --with-geopmd=$GEOPM_INSTALL
    make
    make install


The libraries, binaries and python tools will not be installed into
the standard system paths if GEOPM is built from source and configured
with the `--prefix` option.  In this case, it is required that the
user augment their environment to specify the installed location.  If
the configure option is specified as above. then the following
modifications to the user's environment should be made prior to
running any GEOPM tools:

.. code-block:: bash

    export LD_LIBRARY_PATH=$GEOPM_INSTALL/lib:$LD_LIBRARY_PATH
    export PATH=$GEOPM_INSTALL/bin:$PATH
    export PYTHONPATH=$(ls -d $GEOPM_INSTALL/lib/python*/site-packages | tail -n1):$PYTHONPATH


Use a PYTHONPATH that points to the site-packages created by the geopm
build.  The version created is for whichever version of python 3 was
used in the configure step.  If a different version of python is
desired, override the default with the --with-python option in the
configure script.


Configuring the Build
---------------------

There are many options that may be passed to each of the two configure
scripts that are part of the GEOPM repository build system.  Two
scripts called ``autogen.sh`` are provided, one in the base of the
GEOPM repository and the other in the service directory.  Each of
these scripts manage the GEOPM version that is imbedded in the build
artifacts, and create the two ``configure`` scripts using the
autotools package.

Running the configure scripts generate a number of output files,
including the ``Makefile`` that is used for the rest of the build
steps.  The ``configure`` scripts accept a large number of command
line options, and environment variables that affect the behavior.
Each of configure script will provide user documentation through the
``./configure --help`` command.  Some important options and
environment variables are listed below.

Both configure scripts
^^^^^^^^^^^^^^^^^^^^^^

* ``--prefix``
  Path prefix for install artifacts

* ``--enable-debug``
  Create more verbose error and warning messaging and disable
  optimization.

* ``--enable-coverage``
  Enable coverage report generation with gcov

* ``export CC=``
  Set the C compiler with environment variable

* ``export CXX=``
  Set the C++ compiler with environment variable


Service configure script
^^^^^^^^^^^^^^^^^^^^^^^^

* ``--enable-nvml``
  Add support for the Nvidia NVML library

* ``--enable-levelzero``
  Add support for OneAPI LevelZero

* ``--disable-systemd``
  Do not build GEOPM Service access into PlatformIO


Base configure script
^^^^^^^^^^^^^^^^^^^^^

* ``--with-geopmd=``
  Provide install location of the service build

* ``--disable-mpi``
  Build the base directory without MPI dependencies

* ``--disable-fortran``
  Build the base directory without fortran dependencies

* ``--disable-openmp``
  Build the base directory without OpenMP dependencies

* ``export FC=``
  Set the Fortran compiler with environment variable

* ``export F77=``
  Set the Fortran 77 compiler with environment variable

* ``export MPICC=``
  Set the MPI C compiler wrapper with environment variable

* ``export MPICXX=``
  Set the MPI C++ compiler wrapper with environment variable

* ``export MPIFC=``
  Set the Fortran compiler wrapper with environment variable

* ``export MPIF77=``
  Set the Fortran 77 compiler wrapper with environment variable


Intel Compiler and MPI Toolchain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To enable the use of the Intel toolchain for both the compiler and MPI support, export
the following variables prior to configuring the base build of the GEOPM repository:

.. code-block:: bash

    export CC=icc
    export CXX=icpc
    export FC=ifort
    export F77=ifort
    export MPICC=mpiicc
    export MPICXX=mpiicpc
    export MPIFC=mpiifort
    export MPIF77=mpiifort

We recommend using the system compiler toolchain for compiling the
GEOPM service when creating an installed RPM.  The ``make rpm`` target
of the service directory uses the geopm-service spec file to ensure
that the system GCC toolchain is used to create the RPM.


Coverage Instructions
---------------------

To generate a coverage report, first be sure that you have installed
the lcov package.  Note that if you are using GCC 9 or above, you must
use lcov v1.15 or later to work around `this issue
<https://github.com/linux-test-project/lcov/issues/58>`_.

The lcov source is available here:

`https://github.com/linux-test-project/lcov/`

The GEOPM build must be configured with the "--enable-coverage" option
prior to running the tests.  Then in either the service directory or
the root directory, simply run

.. code-block::

   make coverage


which runs the corresponding unit tests and produces a coverage report in

.. code-block::

   ./coverage/index.html


Note that all tests must pass in order to generate a coverage report.
Any help in increasing code coverage levels is appreciated.


Coding Style
------------

Python code should follow the PEP8 standard as described in
https://www.python.org/dev/peps/pep-0008/.

C++ code can be corrected to conform to the GEOPM standard
using astyle with the following options:

.. code-block::

   astyle --style=linux --indent=spaces=4 -y -S -C -N


Note that astyle is not perfect (in particular it is confused by C++11
initializer lists), and some versions of astyle will format the code
slightly differently.

Use C style variable names with lower case and underscores.  Upper
camel case is used exclusively for class names.  Prefix all member
variables with "m\ *" and all global variables with "g*\ ".

Please avoid global variables as much as possible and if it is
necessary to use a global (primarily for C code) please scope them
statically to the compilation unit.

Avoid preprocessor macros as much as possible (use enum not #define).
Preprocessor usage should be reserved for expressing configure time
options.


License Headers
---------------

Introducing a new file requires a license comment in its header with a
corresponding copying_headers/header.\ * file.  The new file path must
be listed in the corresponding copying_headers/MANIFEST.* file.  This
can be tested by running the copying_headers/test_license script after
committing the new file to git, and rerunning the autogen.sh script.
Files for which a license comment is not appropriate should be listed
in copying_headers/MANIFEST.EXEMPT.  Any new installed files should
also be added to specs/geopm.spec.in or service/geopm-service.spec.in.


Creating Manuals
----------------

Introducing a new man page requires changes in multiple files:


#.
   The build target (man page) should be added to rst_file in conf.py
#.
   The rst source file should be added to EXTRA_DIST in service/docs/Makefile.mk
#.
   The build target (man page) should be added to dist_man_MANS in service/docs/Makefile.mk
#.
   The ronn source file should be added to copying_headers/MANIFEST.EXEMPT as
   described above.
#.
   The gzipped installed man page should be listed in the %files section of
   geopm-service.spec.in
#.
   A link to the new html page should be added to the SEE ALSO section of
   geopm.7.rst and any other related man pages.
