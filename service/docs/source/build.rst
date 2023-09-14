
Guide to Source Builds
======================

These are build instructions for an administrator that would like to
install the GEOPM Service based on a checkout of the GEOPM git
repository.  Following these instructions will enable the user to
create RPM packages that can be installed on several Linux
distributions.

Packages created through this process have been tested on the CentOS 8,
RHEL 8, and SLES 15 SP2 Linux distributions.  We welcome feedback
from testing on other distributions like Fedora, openSUSE Leap, and
Ubuntu (note there is currently no .deb support).

Installing the GEOPM Service relies on the RPM packaging system.  The
RPM packaging system enables some cross-Linux-distribution
compatibility for the installation of the systemd service, and
provides a clean mechanism for uninstall.


Building GEOPM Service RPMs
---------------------------

Support for packaging for CentOS 8, RHEL 8, and SLES 15 SP2 is provided
by the GEOPM service build system.

.. code-block:: bash

    git clone git@github.com:geopm/geopm.git
    cd geopm/service
    ./autogen.sh
    ./configure
    make rpm


These commands create the GEOPM service RPM files in your rpmbuild
directory:

.. code-block:: bash

    $HOME/rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/geopm-service-devel-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/libgeopmd2-<VERSION>-1.x86_64.rpm


Installing and Starting the GEOPM Service
-----------------------------------------

After following the instructions above, install the RPM files by
executing the ``install_service.sh`` script located in the
``service/integration`` sub-directory of the GEOPM repository.

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh $(cat VERSION) $USER
    systemctl status geopm


Installing the GEOPM Service Development Package
------------------------------------------------

A system administrator that would like to enable users of the system to
link to the C or C++ interfaces provided by libgeopmd.so should
install the GEOPM Service Development Package.  This provides the C
and C++ header files.  Installation of this package must be done
manually as it is **not** installed as part of running the
``install_service.sh`` script documented above.


Removing the GEOPM Service
--------------------------

To stop the GEOPM service and uninstall the RPMs from the system,
simply pass the ``--remove`` option to the installer script used in
the previous section:

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh --remove
