Guide to Requirements
=====================

The GEOPM library depend on several external dependencies for enabling
certain features. However, most of them are readily available in standard Linux
distributions. If you intend to use only the GEOPM Service, all the required
dependencies can be found on this page. For users of the GEOPM HPC Runtime,
refer to its dedicated documentation
`here <https://geopm.github.io/runtime.html>`__ for the necessary external dependencies.

Build Requirements
------------------

To run the GEOPM service build, the packages listed below are required. All these
packages are available in standard Linux distributions and can be installed
using commands specific to RPM-based Linux distributions.

Upstream RHEL and CentOS Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    yum install python3 python3-devel python3-gobject-base \
                python3-sphinx python3-sphinx_rtd_theme \
                systemd-devel



Upstream SLES and OpenSUSE Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    zypper install python3 python3-devel python3-gobject \
                   python3-Sphinx python3-sphinx_rtd_theme \
                   systemd-devel


Dasbus Requirement
^^^^^^^^^^^^^^^^^^

A more recent version of Dasbus (version 1.5 or later) is required for the GEOPM service.
This version currently isn't packaged in Linux distributions. The GEOPM repository
contains a script:

``service/integration/build_dasbus.sh``

that can be executed to create an RPM for Dasbus version 1.6. The script, upon
successful execution, will direct how to install the generated RPM.

The requirement for ``python-dasbus`` is specified in the geopm-service spec file. Thus,
an RPM installation of Dasbus is essential. It is also possible to update Dasbus using
``pip``, but in its absence, the ``python-dasbus`` requirement must be removed from the
``service/geopm-service.spec.in`` file within the GEOPM repository prior to building
the GEOPM service RPMs.

Systemd Library Requirement
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``systemd-devel`` package can be omitted in the building sequence with the
``--disable-systemd`` option. Note that this is mandatory for older Linux distributions like CentOS 7.
However, disabling ``libsystemd`` means the user-space loaded PlatformIO interface won't access signals
or controls from the GEOPM Systemd Service. The ``--disable-systemd`` option doesn't impact the GEOPM user unless
the GEOPM Systemd Service is installed and active on the running Linux OS.

Sphinx Requirement
^^^^^^^^^^^^^^^^^^

The Sphinx python package is essential for generate man pages and HTML
documentation. For the build to function correctly, the man pages must be generated. These man pages are included in the distribution tarball created by the
``make dist`` target, hence, building using this archive doesn't particularly require
Sphinx. The requirement can be satisfied by PIP if there are issues installing the RPM
packages for sphinx:

.. code-block:: bash

    python3 -m pip install --user sphinx sphinx_rtd_theme sphinxemoji
    export PATH=$HOME/.local/bin:$PATH

These commands install Sphinx into your user's local Python packages and
add the local Python package bin directory to your path for access to the
sphinx-build script.

Run Requirements
----------------

There are runtime requirements for both the GEOPM Service and
the GEOPM HPC Runtime. These requirements relate to the MSR driver
and the configuration of systemd.

Access to MSRs enhances the capability of the GEOPM Service in terms of hardware
telemetry and controls. While the GEOPM Service can function without access to MSRs,
it provides a limited set of hardware features. For the GEOPM HPC Runtime to function
correctly, these MSR-related hardware features are necessary. Hence, MSR support is a
hard requirement for the GEOPM HPC Runtime.

Erroneous settings for systemd can cause inter-process communication issues for both the
GEOPM Service and the GEOPM HPC Runtime.

The MSR Driver
^^^^^^^^^^^^^^

The msr-safe kernel driver provides two key features. Firstly, it offers a low
latency interface for reading and writing many MSR values at once through an
`ioctl(2) <https://man7.org/linux/man-pages/man2/ioctl.2.html>`_ system call, possibly
improving the performance of GEOPM HPC runtime or other MSR usages.

Secondly, the msr-safe kernel driver enables user-level read and write operations of
the model-specific registers (MSRs) with access controlled by the system administrator.
This feature is mandatory if the GEOPM Service is not active on the system. Alternatively, the access can
also be managed by the system administrator using the GEOPM Service, if active.

The msr-safe kernel driver code can be found `here <https://github.com/LLNL/msr-safe>`__.
It's distributed with OpenHPC and can be installed from the RPMs provided there.

In the absence of both the msr-safe kernel driver and the GEOPM Systemd Service, root users may access MSRs
using the standard MSR driver. This can be loaded with the command:

.. code-block:: bash

    modprobe msr

The standard MSR driver must also be loaded to enable MSR access
through the GEOPM Systemd Service when msr-safe is not installed.

Systemd Configuration
^^^^^^^^^^^^^^^^^^^^^

For GEOPM to utilize shared memory for communication between the Controller and the
application, it may be necessary to alter systemd configuration. By default, systemd
removes all inter-process communication for non-system users. This poses a problem to
GEOPM's shared memory initialization routines.

This behavior can be disabled by setting ``RemoveIPC=no`` in
``/etc/systemd/logind.conf``. A majority of Linux distributions change the default
setting to prevent this issue. More information can be found
`here <https://superuser.com/a/1179962>`__.
