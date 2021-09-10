
Building GEOPM Service
======================

Installing the GEOPM Service relies on the RPM packaging system.  The
RPM packaging system enables some cross-Linux-distribution
compatibility for the installation of the systemd service, and
provides a clean mechanism for uninstall.

We are currently supporting the GEOPM service for the CentOS 8, RHEL
8, and SLES 15 SP2 Linux distributions.


Upstream RHEL and CentOS Package Requirements
---------------------------------------------

.. code-block:: bash

    yum install python3 python3-devel python3-gobject-base



Upstream SLES and OpenSUSE Package Requirements
-----------------------------------------------

.. code-block:: bash

    zypper install python3 python3-devel python3-gobject


Dasbus Requirement
------------------

the geopm service requires a more recent version of dasbus than is
currently packaged by linux distributions (dasbus version 1.5 or more
recent).  The script located in the subdirectory of the geopm repo:

``service/integration/build_dasbus.sh``

can be executed to create the required rpm based on dasbus version 1.6.
the script will print how to install the generated RPM upon successful
completion.

The ``python-dasbus`` requirement is explicitly stated in the
geopm-service spec file.  Because of this, an RPM installation of
dasbus is required.  Alternatively, dasbus may be updated with
``pip``, but unless a python-dasbus RPM is installed on the system,
the requirement must be removed from the file
``service/geopm-service.spec.in`` in the GEOPM repo before building
the GEOPM service RPMs.


Building GEOPM Service RPMs
---------------------------

Support for packaging for CentOS 8, RHEL 8, and SLES 15 SP2 is provided
by the GEOPM service build system.

.. code-block:: bash

    git clone git@github.com:geopm/geopm.git
    cd geopm
    git checkout geopm-service
    cd service
    ./autogen.sh
    ./configure
    make rpm


These commands create the GEOPM service RPM files in your rpmbuild
directory:

.. code-block:: bash

    $HOME/rpmbuild/RPMS/x86_64/geopm-service-<VERSION>-1.x86_64.rpm
    $HOME/rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm


Installing and Starting the GEOPM Service
-----------------------------------------

After following the instructions above, install the RPM files by
executing the ``install_service.sh`` script located in the
``service/integration`` sub-directory of the GEOPM repository.

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh $(cat VERSION) $USER
    systemctl status geopm

Removing the GEOPM Service
--------------------------

To stop the GEOPM service and uninstall the RPMs from the system,
simply pass the ``--remove`` option to the installer script used in
the previous section:

.. code-block:: bash

    cd geopm/service
    sudo ./integration/install_service.sh --remove
