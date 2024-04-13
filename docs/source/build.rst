Source Builds
=============

This documentation provides build instructions for administrators who are
interested in installing the GEOPM packages based on a checkout from the GEOPM
git repository. Following these instructions will assist users in creating RPM
or debian packages compatible with various Linux distributions.

We have tested packages created through this process on SLES 15 SP3 and SP4,
as well as Ubuntu jammy 22.04.  Feedback from installations on other
distributions such as CentOS, openSUSE Leap, and Fedora would be greatly
appreciated.

GEOPM Service installation is available via both RPM and debian packages. These
packages provide cross-Linux-distribution compatibility for the installation of
the systemd service and facilitate a clean uninstallation process.

The GEOPM service build system provides support for packaging for:

* SLES 15 SP3 and SP4
* CentOS 8 amd 9-Stream
* Ubuntu 22.04 jammy

Building GEOPM Service RPMs
---------------------------

First review the :doc:`requirements guide<requires>`, then use the following
bash commands to build all of the GEOPM packages.

.. code-block:: bash

    git clone git@github.com:geopm/geopm.git
    cd geopm/service
    ./package.sh

These commands create the .rpm files or .deb files that package the GEOPM
software.  On Ubuntu, these packages are build in the tree:

.. code-block:: bash

    ./docs/geopm-runtime-docs_<VERSION>-1_all.deb
    ./docs/geopm-service-docs_<VERSION>-1_all.deb
    ./docs/libgeopmd-docs_<VERSION>-1_all.deb
    ./docs/libgeopm-docs_<VERSION>-1_all.deb
    ./docs/python3-geopmdpy-docs_<VERSION>-1_all.deb
    ./docs/python3-geopmpy-docs_<VERSION>-1_all.deb
    ./geopmdpy/python3-geopmdpy_<VERSION>-1_all.deb
    ./geopmpy/python3-geopmpy_<VERSION>-1_all.deb
    ./libgeopmd/geopm-service_<VERSION>-1_amd64.deb
    ./libgeopmd/libgeopmd2_<VERSION>-1_amd64.deb
    ./libgeopmd/libgeopmd-dev_<VERSION>-1_amd64.deb
    ./libgeopm/geopm-runtime_<VERSION>-1_amd64.deb
    ./libgeopm/libgeopm2_<VERSION>-1_amd64.deb
    ./libgeopm/libgeopm-dev_<VERSION>-1_amd64.deb


In RPM based operating systems the RPMs are created in the rpmbuild root
(default is ~/rpmbuild).

.. code-block:: bash

    ./rpmbuild/RPMS/x86_64/geopm-runtime-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/geopm-runtime-devel-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/geopm-runtime-docs-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/geopm-service-devel-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/geopm-service-docs-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/libgeopm2-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/libgeopmd2-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/libgeopmd-docs-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/libgeopm-docs-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/python3-geopmdpy-docs-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/python3-geopmpy-<VERSION>-1.x86_64.rpm
    ./rpmbuild/RPMS/x86_64/python3-geopmpy-docs-<VERSION>-1.x86_64.rpm


Installing and Running the GEOPM Service
----------------------------------------

Once the packaging is complete, install the RPM files by issuing your OS's
packaging manager (i.e. zypper or yum):

.. code-block:: bash

   $ sudo zypper install --allow-unsigned-rpm $HOME/rpmbuild/RPMS/x86_64/*geopm*.rpm

Or the DEB files by issuing:

.. code-block:: bash

   $ cd geopm
   $ sudo apt install $(find -name '*geopm*.deb')

To start the GEOPM service and check its status issue:

.. code-block:: bash

    sudo systemctl start geopm
    systemctl status geopm

Removal of the GEOPM Service
----------------------------

To stop the GEOPM service and remove the RPMs from your system, use your OS's
package manager:

.. code-block:: bash

    sudo systemctl stop geopm
    sudo zypper remove geopm-runtime \
        geopm-runtime-devel \
        geopm-runtime-docs \
        geopm-service \
        geopm-service-devel \
        geopm-service-docs \
        libgeopm2 \
        libgeopmd2 \
        libgeopmd-docs \
        libgeopm-docs \
        python3-geopmdpy \
        python3-geopmdpy-docs \
        python3-geopmpy \
        python3-geopmpy-docs

Or for the DEB packages by issuing:

.. code-block:: bash

    sudo systemctl stop geopm
    sudo apt remove geopm-runtime \
        geopm-runtime-devel \
        geopm-runtime-docs \
        geopm-service \
        geopm-service-devel \
        geopm-service-docs \
        libgeopm2 \
        libgeopmd2 \
        libgeopmd-docs \
        libgeopm-docs \
        python3-geopmdpy \
        python3-geopmdpy-docs \
        python3-geopmpy \
        python3-geopmpy-docs
