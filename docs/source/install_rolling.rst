Development Snapshot Install Guide
----------------------------------

Install packages created by GitHub CI built from the most recent changes to the
GEOPM ``dev`` branch in a rolling release.  These packages are useful for
testing new features and providing early feedback to developers.


GEOPM Access Service - Development
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest development snapshot of the
:doc:`GEOPM Access Service<service>` and its dependency packages is broken down
by OS and OS version.  Note: the commands in this table should be run with
``sudo`` or as the ``root`` user.


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

         .. group-tab:: 24.04 Noble

            .. tabs::

               .. group-tab:: Nvidia GPU support + Expanded Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/dev
                     apt update
                     # GEOPM Access Service python module
                     apt install python3-geopmdpy
                     # SystemD service configuration and geopmd executable
                     apt install geopmd
                     # C/C++ development files
                     apt install libgeopmd-dev
                     # Man pages for GEOPM Access Service
                     apt install geopmd-doc
                     # Man pages for C/C++ development
                     apt install libgeopmd-doc



GEOPM Runtime Service - Development
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instructions on how to install the latest development snapshot of the
:doc:`GEOPM Runtime Service<runtime>` and its dependency packages is broken down
by OS and OS version.  Note: the commands in this table should be run with
``sudo`` or as the ``root`` user.


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

         .. group-tab:: 24.04 Noble

            .. tabs::

               .. group-tab:: Nvidia GPU support + Expanded Intel GPU support

                  .. code-block:: bash

                     # Add Launchpad PPA
                     add-apt-repository ppa:geopm/dev
                     apt update
                     # GEOPM Runtime Command Line Interface
                     apt install geopm-cli
                     # GEOPM Runtime Agent development files
                     apt install libgeopm-dev
                     # GEOPM Runtime post-processing scripts
                     apt install python3-geopmpy
                     # Man pages for GEOPM Runtime Service
                     apt install geopm-doc
                     # Man pages for C/C++ development
                     apt install libgeopm-doc


