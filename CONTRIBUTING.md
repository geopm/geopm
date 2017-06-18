CONTRIBUTING
============
This file describes how to contribute to the geopm project.  All
feedback is appreciated.

BUG REPORT
----------
Please file an issue on our github site for any bug observed:

https://github.com/geopm/geopm/issues

If possible, please reproduce issue with the --enable-debug option
passed to configure at build time and then report any error messages
emitted by geopm at run time.  Please provide the version of geopm
that produced the problem.  The geopm version can be found by running
the command

    geopmpolicy --version

Providing code that is small and reproduces the issue is nice to have.

FEATURE REQUEST
---------------
Please submit a issue on github for any feature requests.

CHANGE REQUEST
--------------
There are two ways to submit a change request.  Opening a pull request
on github from a fork of geopm is preferred for most cases.  It is
also acceptable to submit a change request directly to gerrithub, and
this is the workflow for maintainers and active developers.

### Github pull request
All pull requests to the geopm/geopm repository on github will
automatically be submitted to travis-ci for unit testing.  The link
for creating a pull request is here:

https://github.com/geopm/geopm/pulls

and the link to the results of the unit testing is here:

https://travis-ci.org/geopm/geopm

If the tests pass in travis-ci then the pull request will be brought
into gerrithub for review by the geopm maintainers.  Including a
Change-id: and Signed-off-by: line in the commit message is
appreciated, but not required (they will be added for you by the
maintainers).  The link for the gerrithub reviews is here:

https://review.gerrithub.io/#/q/project:geopm/geopm

We encourage users and developers of the geopm software to participate
in our code review process through gerrithub.  When the request passes
review the change will be integrated into the geopm development branch
on github through a gerrit submission.

### Gerrithub review request
It is also possible to submit a change request directly to gerrithub.
This is the primary workflow of active developers.  When using this
method please include a Change-id: and Signed-off-by: line in the
commit message.  The geopm gerrit server information can be found
here:

https://review.gerrithub.io/#/admin/projects/geopm/geopm

Please refer to the gerrit documentation on how to create review
request.

TEST INSTRUCTIONS
-----------------
To launch the geopm unit tests run the following command in the geopm
directory:

    make check

Please run these tests before submitting a change request.  Set the
MPIEXEC environment variable as is appropriate for your system to
launch the tests with an MPI dependency.  The default if MPIEXEC not
set is "mpiexec".

COVERAGE INSTRUCTIONS
---------------------
To generate a coverage report, first be sure that you have installed
the lcov package available here:

http://ltp.sourceforge.net/coverage/lcov.php

The geopm build must be configured with the "--enable-coverage" option.  Then
simply run

    make coverage

which runs tests and produce a coverage report in

    coverage/index.html

Note that all tests must pass in order to generate a coverage report.
Any help in increasing code coverage levels is appreciated.

CODING STYLE
------------
Code formatting can be corrected to conform to the geopm standard
using astyle with the following options:

    astyle --style=linux --indent=spaces=4 -y -S -C -N

Note that astyle is not perfect (in particular it is confused by C++11
initializer lists), and some versions of astyle will format the code
slightly differently.

Use C style variable names with lower case and underscores.  Upper
camel case is used exclusively for class names.  Prefix all member
variables with "m_" and all global variables with "g_".  Do not prefix
class methods with "get_" or "set_", the interface should reflect the
usage.

Please avoid global variables as much as possible and if it is
necessary to use a global (primarily for C code) please scope them
statically to the compilation unit.

Avoid preprocessor macros as much as possible (use enum not #define).
Preprocesssor usage should be reserved for expressing configure time
options.

Introducing a new file requires a license comment in its header with a
corresponding copying_headers/header.* file.  The new file path must
be listed in the corresponding copying_headers/MANIFEST.* file.  This
can be tested by running the copying_headers/test_license script after
committing the new file to git, removing the geopm/MANIFEST file and
running the autogen.sh script.  Files for which a license comment is
not appropriate should be listed in copying_headers/MANIFEST.EXEMPT.
