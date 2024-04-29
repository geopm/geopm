Requirements
============

The GEOPM Service library depends on several external dependencies for enabling
certain features. However, most of them are readily available in standard Linux
distributions. If you intend to use only the GEOPM Service, all the required
dependencies can be found on this page. For users of the GEOPM Runtime, refer to
its dedicated documentation :doc:`here <runtime>` for the necessary external
dependencies.

Build Requirements
------------------

To run the GEOPM Service build, the packages listed below are required. All
these packages are available in standard Linux distributions and can be
installed using commands specific to RPM-based or Debian-based Linux
distributions.

Upstream RHEL and CentOS Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    yum install gcc-c++ unzip libtool \
                python3 python3-devel python3-gobject-base \
                python3-sphinx python3-sphinx_rtd_theme \
                python3-jsonschema python3-psutil python3-cffi \
                python3-setuptools python3-dasbus \
                systemd-devel liburing-devel

Upstream SLES and OpenSUSE Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    zypper install gcc-c++ unzip libtool \
                   python3 python3-devel python3-gobject \
                   python3-Sphinx python3-sphinx_rtd_theme \
                   python3-jsonschema python3-psutil python3-cffi \
                   python3-setuptools python3-dasbus \
                   systemd-devel liburing-devel

Upstream Ubuntu Package Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    apt install g++ unzip libtool autoconf unzip \
                python3 liburing-dev python3-gi python3-yaml \
                python3-sphinx python3-sphinx-rtd-theme \
                python3-jsonschema python3-psutil python3-cffi \
                python3-setuptools python3-dasbus libsystemd-dev \
                liburing-dev

Systemd Library Requirement
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``systemd-devel`` package can be omitted in the building sequence with the
``--disable-systemd`` option. Note that this is mandatory for older Linux
distributions like CentOS 7.  However, disabling ``libsystemd`` means the
user-space loaded PlatformIO interface won't access signals or controls from the
GEOPM Systemd Service. The ``--disable-systemd`` option doesn't impact the GEOPM
user unless the GEOPM Systemd Service is installed and active on the running
Linux OS.

Sphinx Requirement
^^^^^^^^^^^^^^^^^^

The Sphinx python package is essential for generation of man pages and HTML
documentation.  The source and build infrastrucure for this documentation is
located in the ``docs`` subdirectory of the Git repository. The Sphinx
requirement can be satisfied by PIP if there are issues installing the OS
distribution packages:

.. code-block:: bash

    python3 -m pip install --user sphinx sphinx_rtd_theme sphinxemoji
    export PATH=$HOME/.local/bin:$PATH

These commands install Sphinx into your user's local Python packages and add the
local Python package bin directory to your path for access to the
``sphinx-build`` script.


The MSR Driver
^^^^^^^^^^^^^^

Access to MSRs enhances the capabilities of the GEOPM Access Service by
providing additional hardware telemetry and controls. While the **GEOPM Access
Service** can function without access to MSRs, it provides a limited set of CPU
features. For the **GEOPM Runtime** to function correctly, these MSR-related
CPU features are necessary. Hence, MSR support is a hard requirement for
the GEOPM Runtime which may be relaxed in a future release.

One of two drivers may be used by the GEOPM Access Service to enable the MSR
features: the standard Linux (in-tree) MSR driver or the msr-safe kernel driver
maintained by LLNL.  The msr-safe driver is preferred by GEOPM if both kernel
modules are loaded because it provides low latency interface for reading and
writing many MSR values at once through an `ioctl(2)
<https://man7.org/linux/man-pages/man2/ioctl.2.html>`_ system call, possibly
improving the performance of GEOPM Runtime or other MSR usages.

The msr-safe kernel driver source code can be found `here
<https://github.com/LLNL/msr-safe>`__.  It's distributed with the `OpenSUSE
Hardware Repository <https://download.opensuse.org/repositories/hardware/>`_ and
can be installed from the RPMs provided there.  For more information about the
necessary configuration of msr-safe see: :ref:`geopmaccess.1:Configuring
msr-safe` and :ref:`overview:admin configuration`.  Note that subsequent to
v1.7.0 of msr-safe, it is required that the msr-safe allow list be configured
prior to starting the GEOPM Access Service.

In the absence of the msr-safe kernel driver, users may access MSRs using the
standard Linux MSR driver. This can be loaded with the command:

.. code-block:: bash

    modprobe msr

The standard MSR driver be loaded to enable MSR access through the GEOPM Systemd
Service when msr-safe is not installed.
