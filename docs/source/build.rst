Source Builds
=============

This documentation provides build instructions for administrators who are
interested in installing the GEOPM Service based on a checkout from the GEOPM
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

Building GEOPM Service Debian Packages
--------------------------------------

Use the following bash commands:

.. code-block:: bash

    git clone git@github.com:geopm/geopm.git
    cd geopm/service
    ./autogen.sh
    ./configure
    make deb

These commands create the GEOPM service debian packages  in your current working
directory, e.g.:

.. code-block:: bash

    $HOME/geopm/service/geopm-service_<VERSION>-1_amd64.deb
    $HOME/geopm/service/libgeopmd2_<VERSION>-1_amd64.deb
    $HOME/geopm/service/libgeopmd-dev_<VERSION>-1_amd64.deb
    $HOME/geopm/service/python3-geopmdpy_<VERSION>-1_amd64.deb

.. note::

   Because the GEOPM Python modules are still packaged with ``setuptools``, you
   must use the package manager installed version of python3-setuptools when
   issuing ``make deb``.  At the of this writing that is version ``59.6.0``.

   If you have a newer version of ``setuptools`` installed via ``pip``, you can
   use the following to remove it: ``python3 -m pip uninstall setuptools``

   Alternatively you can force the usage of the system version of ``setuptools``
   with: ``SETUPTOOLS_USE_DISTUTILS=stdlib make deb``

   For more information see:

   https://bugs.launchpad.net/ubuntu/+source/dput/+bug/2018519

   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1003252

Installing and Running the GEOPM Service
----------------------------------------

Once the packaging is complete, install the RPM files by issuing your OS's
packaging manager (i.e. zypper or yum):

.. code-block:: bash

   $ sudo zypper install --allow-unsigned-rpm \
     $HOME/rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm \
     $HOME/rpmbuild/RPMS/x86_64/geopm-service-devel-<VERSION>-1.x86_64.rpm \
     $HOME/rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm \
     $HOME/rpmbuild/RPMS/x86_64/libgeopmd2-<VERSION>-1.x86_64.rpm

Or the DEB files by issuing:

.. code-block:: bash

   $ cd geopm/service
   $ sudo apt install \
     ./geopm-service_<VERSION>-1_amd64.deb \
     ./libgeopmd2_<VERSION>-1_amd64.deb \
     ./libgeopmd-dev_<VERSION>-1_amd64.deb \
     ./python3-geopmdpy_<VERSION>-1_amd64.deb

To start the GEOPM service and check its status issue:

.. code-block:: bash

    sudo systemctl start geopm
    systemctl status geopm

Removal of the GEOPM Service
----------------------------

To stop the GEOPM service and remove the RPMs from your system, use your OS's
package manager:

.. code-block:: bash

    sudo zypper remove geopm-service libgeopmd2 geopm-service-devel python3-geopmdpy

Or for the DEB packages by issuing:

.. code-block:: bash

    sudo systemctl stop geopm
    sudo apt remove geopm-service libgeopmd2 libgeopmd-dev python3-geopmdpy
