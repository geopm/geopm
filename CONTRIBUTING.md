CONTRIBUTING
============
This file describes how to contribute to the GEOPM project.  All
feedback is appreciated.

BUG REPORT
----------
Please file an issue on our GitHub site for any bug observed:

https://github.com/geopm/geopm/issues

If possible, please reproduce issue with the --enable-debug option
passed to configure at build time and then report any error messages
emitted by GEOPM at run time.  Please provide the version of GEOPM
that produced the problem.  The GEOPM version can be found by running
the command

    geopmread --version

Providing code that is small and reproduces the issue is nice to have.

FEATURE REQUEST
---------------
Please submit an issue on GitHub for any feature requests.

CHANGE REQUEST
--------------

Changes to GEOPM are submitted by opening a pull request on GitHub
from a fork of GEOPM.  Including a Signed-off-by: line in the commit
message is appreciated, but not required.  All pull requests to the
geopm/geopm repository on GitHub will automatically be submitted to
Travis CI for unit testing.

The link for creating a pull request is here:

https://github.com/geopm/geopm/pulls

and the link to the results of the unit testing is here:

https://travis-ci.org/geopm/geopm

We encourage users and developers of the GEOPM software to participate
in our code review process through GitHub.

### Review process

Our process for requesting a pull request to be reviewed and merged
is as follows:

1.  The author submits the pull request.  Please refer to the Git
    documentation on how to create pull request:
    <https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests>.

2.  The submitter may optionally request specific people to be reviewers.
    Otherwise, any GEOPM maintainer may be an approver for the pull request.

3.  During the review process, a reviewer may request changes to the pull
    request.  If possible, these should be submitted as new commits
    without rebasing or squashing to simplify the reader's ability to
    view changes.

4.  When the pull request is accepted by the reviewer(s), it will be
    marked "Approve".  Once the request is approved by all reviewers
    and all CI checks are passing, the change will be integrated into
    the GEOPM development branch by a maintainer.

TEST INSTRUCTIONS
-----------------
To launch the GEOPM unit tests, run the following command in the geopm
directory:

    make check

Please run these tests before submitting a change request.  You can
build and run unit tests on a standalone computer (such as a laptop) by
configuring without MPI as follows:

    ./configure --disable-mpi
    make
    make check

The integration tests require a system with MPI and multiple compute nodes.

COVERAGE INSTRUCTIONS
---------------------
To generate a coverage report, first be sure that you have installed
the lcov package available here:

http://ltp.sourceforge.net/coverage/lcov.php

The GEOPM build must be configured with the "--enable-coverage" option.  Then
simply run

    make coverage

which runs tests and produce a coverage report in

    coverage/index.html

Note that all tests must pass in order to generate a coverage report.
Any help in increasing code coverage levels is appreciated.

CODING STYLE
------------

Python code should follow the PEP8 standard as described in
<https://www.python.org/dev/peps/pep-0008/>.

C++ code can be corrected to conform to the GEOPM standard
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
Preprocessor usage should be reserved for expressing configure time
options.

Introducing a new file requires a license comment in its header with a
corresponding copying_headers/header.* file.  The new file path must
be listed in the corresponding copying_headers/MANIFEST.* file.  This
can be tested by running the copying_headers/test_license script after
committing the new file to git, removing the geopm/MANIFEST file and
running the autogen.sh script.  Files for which a license comment is
not appropriate should be listed in copying_headers/MANIFEST.EXEMPT.
Any new installed files should also be added geopm.spec.in.

Introducing a new man page requires changes in multiple files:
1. The build target (man page) should be added to ronn_man in
   Makefile.am.
2. The ronn source file should be added to EXTRA_DIST in Makefile.am.
3. The ronn source file should be added to MANIFEST.EXEMPT as
   described above.
4. The gzipped installed man page should be listed in the %files section of
   geopm.spec.in.
5. A link from the man page name to the man page file should be added
   to ronn/index.txt.
6. A link to the new man page should be added to the SEE ALSO section of
   geopm.7.ronn and any other related man pages.