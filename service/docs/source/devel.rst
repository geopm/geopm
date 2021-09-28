
Guide for GEOPM Developers
==========================

These are instructions for a developer that would like to modify the
source code in the GEOPM repository.

Source Builds
-------------

The GEOPM repository is comprised of 2 directories that must be built
independently: the base directory (i.e. the root of the repository after
issuing ``git clone``) and the ``service/`` directory.  The service directory
must be built first as the base directory build depends on libraries created in
the service directory.

Within the GEOPM repository, a build script has been provided to automate the
build sequence and ensure the right configure time options are specified.  The
``build.sh`` script is located in the root of the repository.

In order to invoke the build script, the following environment variables must
be set: ``GEOPM_SOURCE`` and ``GEOPM_INSTALL``.  Ideally this is done with the
``~/.geopmrc`` file.  See `integration/README <../../../integration/README.md>`_
for information on how to create ``~/.geopmrc``.

By default, the build script will attempt to install the build into the
directory pointed to by ``GEOPM_INSTALL``.  This can be bypassed by setting
``GEOPM_SKIP_INSTALL``. Also by default, the unit tests will not run.  The unit
tests will run if ``GEOPM_RUN_TESTS`` is set.  More information on running the
unit tests is discussed in the next section.  For the full list of environment
variable available to the build script along with usage examples, issue:
``./build.sh --help``.

    TODO Spell out raw configure and make commands as well for users who don't want to
    use the build script.

Test Instructions
-----------------

Similar to the build process, the 2 directories contained within the repository
have separate ``make check`` targets, and must be executed separately.  For the
ease of running the tests, this process has been integrated into the build
script.

To launch the GEOPM unit tests via the build script, run the following command
in the root of the GEOPM repository:

.. code-block::

    GEOPM_RUN_TESTS=yes ./build.sh


The script will first build the service directory, then build the unit tests,
then run the unit tests all for the code contained within the service
directory.  If there were no errors, the script will continue on to build the
base directory, then build the unit tests, then run the unit tests for code
contained with the ``<REPO_ROOT>/src`` directory.  If there were no errors, the
script will exit back to the command prompt with a return code of 0.

To run just the service directory or root directory tests in isloation, one can
issue ``make check`` from the desired directory (i.e. service or root).

    TODO Information about interrogating gtest_links logs and similar.
    TODO Python requirements as related to unit test.

Please run these tests before submitting a pull request.  You can
build and run unit tests on a standalone computer (such as a laptop) by
configuring without MPI or Fortran as follows:

.. code-block::

    source ${GEOPM_SOURCE}/integration/config/gnu_env.sh
    GEOPM_SKIP_COMPILER_CHECK=yes GEOPM_BASE_CONFIG_OPTIONS="--disable-mpi --disable-fortran" GEOPM_RUN_TESTS=yes ./build.sh


The build script is setup to stop if an error is encountered during ``make`` or ``make check``.

    TODO The integration tests require a system with MPI and multiple compute nodes.
    Lots more information needed here.

Coverage Instructions
---------------------

To generate a coverage report, first be sure that you have installed
the lcov package.  Note that if you are using GCC 9 or above, you must use lcov v1.15 or later to work around `this issue <https://github.com/linux-test-project/lcov/issues/58>`_.

The lcov source is available here:

`https://github.com/linux-test-project/lcov/`

The GEOPM build must be configured with the "--enable-coverage" option.  This can be accomplished with:

.. code-block::

    source ${GEOPM_INSTALL}/integration/config/gnu_env.sh
    GEOPM_SKIP_COMPILER_CHECK=yes GEOPM_SKIP_INSTALL=yes GEOPM_GLOBAL_CONFIG_OPTIONS="--enable-coverage" ./build.sh


Then in either the service directory or the root directory, simply run

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


#. The build target (man page) should be added to ronn_man in
   Makefile.am.
#. The ronn source file should be added to EXTRA_DIST in Makefile.am.
#. The ronn source file should be added to MANIFEST.EXEMPT as
   described above.
#. The gzipped installed man page should be listed in the %files section of
   specs/geopm.spec.in.
#. A link from the man page name to the man page file should be added
   to ronn/index.txt.
#. A link to the new man page should be added to the SEE ALSO section of
   geopm.7.ronn and any other related man pages.
