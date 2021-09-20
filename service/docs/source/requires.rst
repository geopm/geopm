
Build Requirements
==================

Upstream RHEL and CentOS Package Requirements
---------------------------------------------

.. code-block:: bash

    yum install python3 python3-devel python3-gobject-base systemd-devel



Upstream SLES and OpenSUSE Package Requirements
-----------------------------------------------

.. code-block:: bash

    zypper install python3 python3-devel python3-gobject systemd-devel

Dasbus Requirement
------------------

The geopm service requires a more recent version of dasbus than is
currently packaged by Linux distributions (dasbus version 1.5 or more
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
