
Building GEOPM Service
======================

Installing the GEOPM Service relies on the RPM packaging system.  The
RPM packaging system enables some cross-Linux-distribution
compatibility for the installation of the systemd service, and
provides a clean mechanism for uninstall.


Upstream RHEL and CentOS Package Requirements
---------------------------------------------

.. code-block::

    yum install python3 python3-devel python3-gobject-base



Upstream SLES and OpenSUSE Package Requirements
-----------------------------------------------

.. code-block::

    zypper install python3 python3-devel python3-gobject


Dasbus Requirement
------------------

The GEOPM service requires a more recent version of dasbus than is
currently packaged by Linux distributions (dasbus version 1.5 or more
recent).  The script located in ``geopm/test/build_dasbus.sh`` can be
executed to create the required RPM based on dasbus version 1.6.  The
script will print how to install the generated RPM upon successful
completion.

The ``python-dasbus`` requirement is explicitly stated in the
geopm-service spec file.  Because of this, an RPM installation of
dasbus is required.  Alternatively, dasbus may be updated with
``pip``, but unless a python-dasbus RPM installed on the system, the
requirement must be removed from the file
``geopm/service/geopm-service.spec.in`` before building the
geopm-service RPM.


Building ``geopm-service`` RPMs
-------------------------------

Support for packaging for CentOS-8 RHEL-8 and SLES-15-SP2 is provided
by the geopm service build system.

.. code-block::

    git clone git@github.com:geopm/geopm.git
    cd geopm
    git checkout geopm-service
    cd service
    ./autogen.sh
    ./configure
    make rpm


These commands create the geopm-service RPM files here:

.. code-block::

    $HOME/rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm


Installing and Starting the GEOPM Service
-----------------------------------------

After following the instructions above, install the RPM files by
executing the ``install_service.sh`` script located in the
``service/test`` sub-directory of the GEOPM repository.

.. code-block::

    cd geopm/service
    sudo ./test/install_service.sh $(cat VERSION) $USER
    systemctl status geopm

Removing the GEOPM Service
--------------------------

To stop the GEOPM service and uninstall the RPMs from the system,
simply pass the ``--remove`` option to the installer script used in
the previous section:

.. code-block::

    cd geopm/service
    sudo ./test/install_service.sh --remove
