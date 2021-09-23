
Guide for GEOPM Developers
==========================

These are instructions for a developer that would like to modify the
source code in the GEOPM repository.


Test Instructions
-----------------

To launch the GEOPM unit tests, run the following command in the geopm
build directory after running configure:

.. code-block::

   make check


Please run these tests before submitting a change request.  You can
build and run unit tests on a standalone computer (such as a laptop) by
configuring without MPI as follows:

.. code-block::

   ./configure --disable-mpi
   make
   make check


The integration tests require a system with MPI and multiple compute nodes.

Coverage Instructions
---------------------

To generate a coverage report, first be sure that you have installed
the lcov package available here:

http://ltp.sourceforge.net/coverage/lcov.php

The GEOPM build must be configured with the "--enable-coverage" option.  Then
simply run

.. code-block::

   make coverage


which runs tests and produce a coverage report in

.. code-block::

   coverage/index.html


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
