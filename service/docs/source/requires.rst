
External Requirements
=====================

There are several external dependencies that GEOPM requires to enable
certain features.  The requirements for the GEOPM Service features are
all provided by commonly used Linux distributions.  A user that is
only interested in using the GEOPM Service will find all of the
dependencies in this page.  A user of the GEOPM HPC Runtime should
refer to the documentation for that feature
`here <https://geopm.github.io/runtime.html>`__ to learn
about the external dependencies that the runtime requires.

Build Requirements
------------------

There are several packages that are required to run the GEOPM service
build.  These packages are available from standard Linux distributions.
The following commands can be used to install them using several RPM
based Linux distributions.


Upstream RHEL and CentOS Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    yum install python3 python3-devel python3-gobject-base \
                python3-Sphinx python3-sphinx_rtd_theme \
                systemd-devel



Upstream SLES and OpenSUSE Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    zypper install python3 python3-devel python3-gobject \
                   python3-Sphinx python3-sphinx_rtd_theme \
                   systemd-devel


Dasbus Requirement
^^^^^^^^^^^^^^^^^^

The geopm service requires a more recent version of dasbus than is
currently packaged by Linux distributions (dasbus version 1.5 or more
recent).  The script located in the subdirectory of the geopm repo:

``service/integration/build_dasbus.sh``

can be executed to create the required RPM based on dasbus version 1.6.
The script will print how to install the generated RPM upon successful
completion.

The ``python-dasbus`` requirement is explicitly stated in the
geopm-service spec file.  Because of this, an RPM installation of
dasbus is required.  Alternatively, dasbus may be updated with
``pip``, but unless a python-dasbus RPM is installed on the system,
the requirement must be removed from the file
``service/geopm-service.spec.in`` in the GEOPM repo before building
the GEOPM service RPMs.


Systemd Library Requirement
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``systemd-devel`` package requirement may be omitted if the geopm
service build is configured with the ``--disable-systemd`` option.
This will be required on older Linux distributions like CentOS 7.  The
effect of disabling use of ``libsystemd`` is that a user-space loaded
PlatformIO interface will not access signals or controls provided by
the GEOPM Systemd Service.  Providing ``--disable-systemd`` configure
option will have no impact on the end user of GEOPM unless the GEOPM
Systemd Service is installed and active in the running Linux OS.


Sphinx Requirement
^^^^^^^^^^^^^^^^^^

The sphinx python package is used to generate man pages and HTML
documentation.  The generated man pages are required when running the build.
The man pages are included in a distribution tarball created with the
`make dist` target, so building from such an archive does not explicitly require
sphinx.  This requirement may also be satisfied with PIP if installing the RPM
packages for sphinx is an issue on your system:

.. code-block:: bash

    python3 -m pip install --user sphinx sphinx_rtd_theme
    export PATH=$HOME/.local/bin:$PATH


These commands will install sphinx into your user's local python packages and
add the the local python package bin directory to your path for access to the
sphinx-build script.


Run Requirements
----------------

There are some time-of-use requirements for both the GEOPM Service and
the GEOPM HPC Runtime.  These requirements relate to the MSR driver
and the configuration of systemd.

The use of MSRs enable many important features of the GEOPM Service
for gathering hardware telemetry and setting hardware controls.  The
GEOPM Service will function without access to MSRs, however it will
provide a more restricted set of hardware features.  These MSR related
hardware features are required for the GEOPM HPC Runtime to function
correctly.  For this reason, MSR support is a hard requirement for the
GEOPM HPC Runtime.

There may be some problematic settings for systemd which could lead to
inter-process communication issues for both the GEOPM Service and the
GEOPM HPC Runtime.


The MSR Driver
^^^^^^^^^^^^^^

The msr-safe kernel driver provides two features.  One of these is a
performance feature providing a low latency interface for reading and
writing many MSR values at once through an `ioctl(2)` system call.
This may enable better performance of the GEOPM HPC runtime or other
uses of MSRs, but also may not be critical depending on the
algorithmic requirements.

The other feature that the msr-safe kernel driver provides is
user-level read and write of the model specific registers (MSRs) with
access managed through an allowed list that is controlled by the system
administrator.  This feature is required by the GEOPM runtime if the
GEOPM Service is not active on the system.  Alternately, the access
management for MSRs may be configured by the system administrator
using the GEOPM Service if it is active.

The msr-safe kernel driver is distributed with OpenHPC and can be
installed using the RPMs distributed there.  The source code for the
driver can be found `here <https://github.com/LLNL/msr-safe>`__.

If both the msr-safe kernel driver and the GEOPM Systemd Service are
unavailable, then GEOPM when run by the root user may access MSRs
through the standard msr driver.  This may be loaded with the
following command:

.. code-block:: bash

    modprobe msr

The standard msr driver must also be loaded to enable MSR access
through the GEOPM Systemd Service when msr-safe is not installed.


Systemd Configuration
^^^^^^^^^^^^^^^^^^^^^

In order for GEOPM to properly use shared memory to communicate
between the Controller and the application, it may be necessary to
alter the configuration for systemd.  The default behavior of systemd
is to clean-up all inter-process communication for non-system users.
This causes issues with GEOPM's initialization routines for shared
memory.  This can be disabled by ensuring that `RemoveIPC=no` is set
in `/etc/systemd/logind.conf`.  Most Linux distributions change the
default setting to disable this behavior.  More information can be
found `here <https://superuser.com/a/1179962>`__.
