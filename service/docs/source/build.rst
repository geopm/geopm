Source Build Guide
==================

This documentation is providing build instructions for administrators who are
interested in installing the GEOPM Service based on a checkout from the GEOPM
git repository. Following these instructions will assist users in creating RPM
packages compatible with various Linux distributions.

We have tested packages created through this process on CentOS 8, RHEL 8, and
SLES 15 SP2 Linux distributions. Feedback from installations on other
distributions such as Fedora, openSUSE Leap, and Ubuntu would be greatly
appreciated.

GEOPM Service installation is available via both RPM and deb packages. These
packages provide cross-Linux-distribution compatibility for the installation of
the systemd service and facilitate a clean uninstallation process.

Building GEOPM Service RPMs
-----------------------------

The GEOPM service build system is provides support for packaging for CentOS 8,
RHEL 8, and SLES 15 SP2.

Use the following bash commands:

.. code-block:: bash

    git clone git@github.com:geopm/geopm.git
    cd geopm/service
    ./autogen.sh
    ./configure
    make rpm

These commands create the GEOPM service RPM files in your rpmbuild directory:

.. code-block:: bash

    $HOME/rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/geopm-service-devel-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/libgeopmd2-<VERSION>-1.x86_64.rpm


Installing and Running the GEOPM Service
------------------------------------------------

Post following the aforementioned instructions, install the RPM files by
executing the `install_service.sh` script located within `service/integration`
subdirectory of the GEOPM repository.

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh $(cat VERSION) $USER
    systemctl status geopm

Installation of the GEOPM Service Development Package
--------------------------------------------------------------------

For system administrators looking to enable system users to link to the C or
C++ interfaces provided by libgeopmd.so, consider installing the GEOPM Service
Development Package, which provides C and C++ header files. Manual installation
is mandatory as it is not part of the running `install_service.sh` script
explained in the previous section.

Removal of the GEOPM Service
--------------------------------------

To stop the GEOPM service and remove the RPMs from your system, simply pass use the
`--remove` option with the installer script detailed in the previous section:

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh --remove
