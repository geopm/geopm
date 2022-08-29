Testing for the GEOPM Service
=============================

This directory provides a set of integration test scripts showing
examples of interacting with the GEOPM service using the `geopmaccess`
and `geopmsession` command line tools.  These tests are written in
bash and serve the purpose of testing a fully installed GEOPM service
using only software from the service subdirectory of the GEOPM
reposistory.  The tests in this directory also show examples of how to
use each of the features provided by the service.  The tests each
begin with a long comment describing the feature under test, and
provide a form of tutorial for an end user learning how to interact
with the GEOPM service.


Test Setup
----------

The integration tests require a system-wide installation of the GEOPM
service.  This is typically done by installing the GEOPM RPM packages.
Runing the following commands will create the GEOPM Service RPM files
from the GEOPM source code repository.

```bash
cd geopm/service
./autogen.sh
./configure
make rpm
```

The RPM files are created with the `rpmbuild` command.  Unless
otherwise configured, the `rpmbuild` command creates RPMs in the
location: `${HOME}/rpmbuild/RPMS/x86_64`.  To install these files and
start the GEOPM service, the
`geopm/service/integration/install_service.sh` script may be used.
This script may be run in the `geopm/service` directory by issuing the
following command:

```bash
cd geopm/service
sudo integration/install_service.sh $(cat VERSION) ${USER}
```

Note that the `VERSION` file is created in the `geopm/service`
directory when the `autogen.sh` script is run.  Once the GEOPM Service
is installed, the access lists must be updated to enable the tests.
The following commands may be issued to allow any user in the Unix
user group `test_group` to access all signals and controls

```bash
geopmaccess -a | sudo geopmaccess -w -g test_group
geopmaccess -a -c | sudo geopmaccess -w -c -g test_group
```

How to Run the Tests
--------------------

Each of the entry points for the integration tests is a script of the
form:

    geopm/integration/test/test_*.sh

Some of these tests require root privileges and these tests have names
of the form:

    geopm/integration/test/test_su_*.sh

If the test finishes with an exit code of `0`, and it prints `SUCCESS`
at the end, then the test has succeeded.

A limited set of root privileges may be used by non-root users by
installing the helper scripts

    geopm/integration/check_session_clean.sh
    geopm/integration/get_batch_server.py
    geopm/integration/install_service.sh
    geopm/integration/kill_geopmd.sh

into `/usr/sbin` and adding these and the `geopmaccess` command line
tool to the sudoers file.  This enables the user that executes the
tests to execute these four commands without providing a sudo password.
After this is done the `test_su_*.sh` scripts may be run as a non-root
user.

Example modifications to the `/etc/sudoers` file to enable running
these tests with a non-root user are as follows.  Under the "User
privilege specification" section, add the following:

```bash
# All users can execute these with *no* password
%users  ALL=NOPASSWD: \
            /usr/sbin/check_session_clean.sh,\
            /usr/sbin/get_batch_server.py,\
            /usr/sbin/install_service.sh,\
            /usr/sbin/kill_geopmd.sh,\
            /usr/bin/geopmaccess, \
            /usr/bin/systemctl stop geopm, \
            /usr/bin/systemctl start geopm, \
            /usr/bin/systemctl restart geopm
```

Alternatively these `test_su_*.sh` scripts may be run as the root
user, however, the `geopm/integration` directory must be added to the
root user's `PATH` to enable this.


Where to find other tests
-------------------------

The tests in this directory do not use any of the tools provided by
`libgeopm` or `libgeopmpolicy`.  Integration tests for `libgeopm` and
`libgeopmpolicy` derived features are located in
`geopm/integration/test`.  Some of these tests may use the GEOPM
service on a system where it is required.

The unit tests for the C++ files in `geopm/service/src` are located in
`geopm/service/test`.  The unit tests for the C++ files in `geopm/src`
are located in `geopm/test`.  We have split the unit tests in to two
directories so that the service subdirectory is fully independent.

The unit tests for the geopmdpy module are located in
`geopm/service/geopmdpy_test`.
