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

    geopmpolicy --version

Providing code that is small and reproduces the issue is nice to have.

FEATURE REQUEST
---------------
Please submit a issue on GitHub for any feature requests.

CHANGE REQUEST
--------------
There are two ways to submit a change request.  Opening a pull request
on GitHub from a fork of GEOPM is preferred for most cases.  It is
also acceptable to submit a change request directly to GerritHub, and
this is the workflow for maintainers and active developers.

### GitHub pull request
All pull requests to the geopm/geopm repository on GitHub will
automatically be submitted to Travis CI for unit testing.  The link
for creating a pull request is here:

https://github.com/geopm/geopm/pulls

and the link to the results of the unit testing is here:

https://travis-ci.org/geopm/geopm

If the tests pass in Travis CI then the pull request will be brought
into GerritHub for review by the GEOPM maintainers.  Including a
Change-Id: and Signed-off-by: line in the commit message is
appreciated, but not required (they will be added for you by the
maintainers).  The link for the GerritHub reviews is here:

https://review.gerrithub.io/#/q/project:geopm/geopm

We encourage users and developers of the GEOPM software to participate
in our code review process through GerritHub.  When the request passes
review the change will be integrated into the GEOPM development branch
on GitHub through a Gerrit submission.

### GerritHub review request

It is also possible to submit a change request directly to GerritHub.
This is the primary workflow of active developers.  When using this
method please include a Change-Id: and Signed-off-by: line in the
commit message; Change-Id is automatically added when you clone from
GerritHub with the commit-msg hook.  The GEOPM Gerrit server
information can be found here:

https://review.gerrithub.io/#/admin/projects/geopm/geopm

Our process for requesting a patch to be reviewed/merged through
Gerrit is:

1.  The patch owner submits the patch to Gerrit.  Please refer to the
    Gerrit documentation on how to create review request.
    See <https://review.gerrithub.io/Documentation/intro-quick.html>.
2.  The patch owner marks the patch CR+1 (code review) and V+1
    (verified) when complete.  CR+1 signifies that the code compiles,
    and the code is sufficient and correct per the patch owner and/or
    the person scoring it.  V+1 signifies the code has been
    tested/vetted and is ready to be merged per the patch owner and/or
    the person scoring it.  Verified means passing all unit tests
    (and, if possible, all integration tests) before and after this
    patch was introduced.  In general, the patch owner is the only
    person that needs to verify the patch, but in some cases, dev team
    members may explicitly cherry-pick and test a patch on behalf of a
    submitter.
3.  Dev team members (NOT the patch owner) review the patch only after
    (2) is complete.  The last person to review a patch should mark it CR+2
    instead of +1.  Once the patch has been marked CR+2 and V+1, it can be
    merged.


TEST INSTRUCTIONS
-----------------
To launch the GEOPM unit tests, run the following command in the geopm
directory:

    make check

Please run these tests before submitting a change request.

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
Code formatting can be corrected to conform to the GEOPM standard
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