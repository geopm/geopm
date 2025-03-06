Installation
============

This documentation covers how to use pore-built Linux OS packages to install
GEOPM for all users of a system either using the latest stable release of GEOPM,
or a development snapshot. The stable release packages are created as part of
the Git repository tag/release process.  The development snapshot packages are
built by GEOPM's GitHub CI process each time the ``dev`` branch is updated.

GEOPM works with Intel GPUs when GEOPM is built with LevelZero support and
NVIDIA GPUs when GEOPM is built with NVML support. Additional limited support
for GPUs exists when the GPUs are visible in a Linux Direct Rendering Manager
(DRM) sysfs interface.  The Linux DRM interface is implemented for Intel GPUs.
In the table below "Intel GPU support" packages provide only the DRM interfaces
and "Expanded Intel GPU support" packages also provide the LevelZero interfaces.


Install Latest Stable Release
-----------------------------

Install packages from the latest stable release of GEOPM.  These
packages have been upstreamed into Fedora 42 and the openSUSE Hardware
repository, and where applicable these distributions are recommended.
For other scenarios, packages are published by the GEOPM development
team on the `main openSUSE Build System instance
<https://build.opensuse.org/project/show/home:geopm>`_ and through the
`Ubuntu Launchpad package build service
<https://launchpad.net/~geopm>`_.


GEOPM Access Service - Stable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest official release of the
:doc:`GEOPM Access Service<service>` and its dependency packages is
broken down by OS and OS version.

.. tabs::

   .. group-tab:: openSUSE

      .. tabs::

         .. group-tab:: 15.3

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/SLE_15_SP3_Backports/hardware.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc


               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.3/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.4

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/SLE_15_SP4/hardware.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.4/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.5

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/15.5/hardware.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.5/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.6

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/15.6/hardware.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.6/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: Tumbleweed

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/openSUSE_Tumbleweed/hardware.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/openSUSE_Tumbleweed/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc


   .. group-tab:: Fedora

      The GEOPM packages are native in Fedora but man pages are not
      published in the distribution.  The man pages are published
      online, please see `online documentation for the latest release
      <https://geopm.github.io/v3.2.0/reference.html#geopm-manual-pages>`_.

      .. tabs::

         .. group-tab:: 42

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel

         .. group-tab:: Rawhide

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel


   .. group-tab:: CentOS

      .. tabs::

         .. group-tab:: 9_Stream

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/release/CentOS_CentOS-9_Stream/home:geopm:release.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:geopm:release:supplementary/CentOS_CentOS-9_Stream/home:geopm:release:supplementary.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

   .. group-tab:: Rocky

      .. tabs::

         .. group-tab:: 9_Standard

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/release/RockyLinux_9_standard/home:geopm:release.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:geopm:release:supplementary/RockyLinux_9_standard/home:geopm:release:supplementary.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

   .. group-tab:: Ubuntu

      .. tabs::

         .. group-tab:: 22.04 Jammy

            .. tabs::

               .. group-tab:: Nvidia GPU support + Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/release
                     apt update
                     # GEOPM Access Service python module
                     apt install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     apt install geopmd
                     # C/C++ development files
                     apt install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     apt install geopmd-doc
                     # Man pages for C/C++ development
                     apt install libgeopmd-doc


GEOPM Runtime Service - Stable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest official release of the
:doc:`GEOPM Runtime Service<runtime>` and its dependency packages is
broken down by OS and OS version.

.. tabs::

   .. group-tab:: openSUSE

      .. tabs::

         .. group-tab:: 15.3

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/SLE_15_SP3_Backports/hardware.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.3/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.4

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/SLE_15_SP4/hardware.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.4/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.5

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/15.5/hardware.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.5/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.6

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/15.6/hardware.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/15.6/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: Tumbleweed

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/hardware/openSUSE_Tumbleweed/hardware.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:geopm:release:supplementary/openSUSE_Tumbleweed/home:geopm:release:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

   .. group-tab:: Fedora

      GEOPM packages are native in Fedora but man pages are not
      published in the distribution.  The man pages are published
      online, please see `online documentation for the latest release
      <https://geopm.github.io/v3.2.0/reference.html#geopm-manual-pages>`_.

      The ``python3-geopmpy`` package is also not distributed with
      Fedora, but may be installed on a per-user basis using ``pip``:

      .. code-block:: bash

         python3 -m pip install --user geopmpy

      .. tabs::

         .. group-tab:: 42

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel

         .. group-tab:: Rawhide

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel

   .. group-tab:: CentOS

      .. tabs::

         .. group-tab:: 9_Stream

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/release/CentOS_CentOS-9_Stream/home:geopm:release.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:geopm:release:supplementary/CentOS_CentOS-9_Stream/home:geopm:release:supplementary.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

   .. group-tab:: Rocky

      .. tabs::

         .. group-tab:: 9_Standard

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/release/RockyLinux_9_standard/home:geopm:release.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:geopm:release:supplementary/RockyLinux_9_standard/home:geopm:release:supplementary.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

   .. group-tab:: Ubuntu

      .. tabs::

         .. group-tab:: 22.04 Jammy

            .. tabs::

               .. group-tab:: Nvidia GPU support + Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/release
                     apt update
                     # GEOPM Runtime Command Line Interface
                     apt install geopm-cli
                     # GEOPM Runtime Agent development files
                     apt install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     apt install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     apt install geopm-doc
                     # Man pages for C/C++ development
                     apt install libgeopm-doc


Install Current Development Snapshot
------------------------------------

Install packages created by GitHub CI built from the most recent changes to the
GEOPM ``dev`` branch in a rolling release.  These packages are useful for
testing new features and providing early feedback to developers.


GEOPM Access Service - Development
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest development snapshot of the
:doc:`GEOPM Access Service<service>` and its dependency packages is
broken down by OS and OS version.

.. tabs::

   .. group-tab:: openSUSE

      .. tabs::

         .. group-tab:: 15.3

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.3/home:geopm.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.3/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.4

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.4/home:geopm.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.4/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.5

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.5/home:geopm.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.5/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: 15.6

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.6/home:geopm.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.6/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

         .. group-tab:: Tumbleweed

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/openSUSE_Tumbleweed/home:geopm.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/openSUSE_Tumbleweed/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Access Service python module
                     zypper install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     zypper install geopmd
                     # C/C++ development files
                     zypper install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     zypper install geopmd-doc
                     # Man pages for C/C++ development
                     zypper install libgeopmd-doc

   .. group-tab:: CentOS

      .. tabs::

         .. group-tab:: 9_Stream

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm/CentOS_CentOS-9_Stream/home:geopm.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/supplementary/CentOS_CentOS-9_Stream/home:geopm:supplementary.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

   .. group-tab:: Rocky

      .. tabs::

         .. group-tab:: 9_Standard

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm/RockyLinux_9_standard/home:geopm.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/supplementary/RockyLinux_9_standard/home:geopm:supplementary.repo
                     popd
                     # GEOPM Access Service python module
                     dnf install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     dnf install geopmd
                     # C/C++ development files
                     dnf install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     dnf install geopmd-doc
                     # Man pages for C/C++ development
                     dnf install libgeopmd-doc

   .. group-tab:: Ubuntu

      .. tabs::

         .. group-tab:: 22.04 Jammy

            .. tabs::

               .. group-tab:: Nvidia GPU support + Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/dev
                     apt update
                     # GEOPM Access Service python module
                     apt install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     apt install geopmd
                     # C/C++ development files
                     apt install libgeopmd-devel
                     # Man pages for GEOPM Access Service
                     apt install geopmd-doc
                     # Man pages for C/C++ development
                     apt install libgeopmd-doc



GEOPM Runtime Service - Development
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest development snapshot of the
:doc:`GEOPM Runtime Service<runtime>` and its dependency packages is
broken down by OS and OS version.

.. tabs::

   .. group-tab:: openSUSE

      .. tabs::

         .. group-tab:: 15.3

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.3/home:geopm.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.3/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.4

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.4/home:geopm.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.4/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.5

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.5/home:geopm.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.5/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: 15.6

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/15.6/home:geopm.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/15.6/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

         .. group-tab:: Tumbleweed

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm/openSUSE_Tumbleweed/home:geopm.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add zypper repo
                     zypper addrepo https://download.opensuse.org/repositories/home:/geopm:/supplementary/openSUSE_Tumbleweed/home:geopm:supplementary.repo
                     zypper refresh
                     # GEOPM Runtime Command Line Interface
                     zypper install geopm-cli
                     # GEOPM Runtime Agent development files
                     zypper install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     zypper install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     zypper install geopm-doc
                     # Man pages for C/C++ development
                     zypper install libgeopm-doc

   .. group-tab:: CentOS

      .. tabs::

         .. group-tab:: 9_Stream

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm/CentOS_CentOS-9_Stream/home:geopm.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/supplementary/CentOS_CentOS-9_Stream/home:geopm:supplementary.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

   .. group-tab:: Rocky

      .. tabs::

         .. group-tab:: 9_Standard

            .. tabs::

               .. group-tab:: Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm/RockyLinux_9_standard/home:geopm.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

               .. group-tab:: Expanded Intel GPU support

                  .. code-block:: bash

                     # Add yum repo
                     pushd /etc/yum.repos.d/
                     wget https://download.opensuse.org/repositories/home:/geopm:/supplementary/RockyLinux_9_standard/home:geopm:supplementary.repo
                     popd
                     # GEOPM Runtime Command Line Interface
                     dnf install geopm-cli
                     # GEOPM Runtime Agent development files
                     dnf install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     dnf install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     dnf install geopm-doc
                     # Man pages for C/C++ development
                     dnf install libgeopm-doc

   .. group-tab:: Ubuntu

      .. tabs::

         .. group-tab:: 22.04 Jammy

            .. tabs::

               .. group-tab:: Nvidia GPU support + Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/dev
                     apt update
                     # GEOPM Runtime Command Line Interface
                     apt install geopm-cli
                     # GEOPM Runtime Agent development files
                     apt install libgeopm-devel
                     # GEOPM Runtime post-processing scripts
                     apt install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     apt install geopm-doc
                     # Man pages for C/C++ development
                     apt install libgeopm-doc


Enable the GEOPM Access Service
-------------------------------

Installing the ``geopmd`` package is the first step to enabling the GEOPM Access
Service.  The :doc:`geopmaccess(1) <geopmaccess.1>` command line tool is used to
configure the access lists.  Unprivileged users will be unable to use the
service until the access lists have been configured by an administrator.  See
the :doc:`GEOPM Access Service Administrator Guide<admin>` for further details
about how to configure the Access Service and interact with it through the
`systemctl(1) <https://man7.org/linux/man-pages/man1/systemctl.1.html>`_ command
line tool.


Guides for Specialized Install
------------------------------

There are other installation scenarios that are not covered on this page which
may be preferred in certain circumstances.  See links below to supporting
documentation if these situations apply to you.


Packaging local changes
~~~~~~~~~~~~~~~~~~~~~~~

See the :doc:`build guide<build>` to create Linux OS packages for changes you
have made locally to the GEOPM source code or build configuration.  This more
involved process should be followed if you would like to build packages that
differ from the ones maintained by the GEOPM team.

Some typical scenarios for packaging local changes:

- Installing GEOPM on an OS or OS version that is not supported by published
  packages, e.g. Debian
- Configuring a GEOPM C++ library with options not supported by published
  packages, e.g. NVML support on openSUSE
- Changing dependency software libraries e.g. enabling MPICH for MPI support
- Deploying on a non-x86_64 architecture, e.g. packaging for aarch64


Installing for a single user
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See the :doc:`developer guide<devel>` to build and install a version of GEOPM
for your user, rather than system-wide.  Note that many important features of
the GEOPM software are not available without the :doc:`GEOPM Access
Service<service>` which requires administrative privileges to install and
configure.

Some typical scenarios for installing for a single user:

- Unit testing changes to the source code in a developer workflow
- Using new client-side GEOPM features on a system where you do not have
  administrative privileges
- Installing geopmpy Python module on systems without OS package support for
  dependencies
