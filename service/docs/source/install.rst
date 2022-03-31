
Guide to Installation
=====================

Continuous integration creates packages for a variety of Linux
distributions each time certain branches hosted on GitHub are updated.
This is supported by GitHub Actions, in conjunction with the OpenSUSE
Build Service.  The ``dev`` branch, which is the default branch
containing the most up to date stable development, is published here:

    https://download.opensuse.org/repositories/home:/geopm/

Until the version 2.0.0 version of GEOPM is tagged, packages
reflecting the latest ``release-v2.0-candidate`` branch will be
published here:

    https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/

The packages built from this service are named:

- ``geopm-service``
- ``geopm-service-devel``
- ``libgeopmd0``
- ``python3-geopmdpy``

In addition to these packages that are built each time a tracked
branch is updated, the download repositories also provide
``python3-dasbus`` version 1.6.  This version of dasbus is required by
``python3-geopmdpy``, and is not provided by the Linux distributions
that we support.  By installing the GEOPM Service packages from the
download repositories, version 1.6 of dasbus will also be installed.


Package Repositories
--------------------

The OBS builds currently support six distributions and two branches
for a total of twelve download repositories.  The links to the twelve
repo data URLs are provided in the list below.

- SLES 15-SP2
   + `dev <https://download.opensuse.org/repositories/home:/geopm/SLE_15_SP2/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/SLE_15_SP2/home:geopm:release-v2.0-candidate.repo>`__
- OpenSUSE 15.3
   + `dev <https://download.opensuse.org/repositories/home:/geopm/15.3/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/15.3/home:geopm:release-v2.0-candidate.repo>`__
- OpenSUSE 15.4
   + `dev <https://download.opensuse.org/repositories/home:/geopm/15.4/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/15.4/home:geopm:release-v2.0-candidate.repo>`__
- Fedora 35
   + `dev <https://download.opensuse.org/repositories/home:/geopm/Fedora_35/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/Fedora_35/home:geopm:release-v2.0-candidate.repo>`__
- CentOS 8
   + `dev <https://download.opensuse.org/repositories/home:/geopm/CentOS_8/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/CentOS_8/home:geopm:release-v2.0-candidate.repo>`__
- CentOS 8-Stream
   + `dev <https://download.opensuse.org/repositories/home:/geopm/CentOS_8_Stream/home:geopm.repo>`__
   + `release-v2.0-candidate <https://download.opensuse.org/repositories/home:/geopm:/release-v2.0-candidate/CentOS_8_Stream/home:geopm:release-v2.0-candidate.repo>`__

One of these URLs should be added to the package configuration by
running one of the two commands below depending on the package
management system for the operating system:

.. code-block:: bash

    # On SUSE based distros
    zypper addrepo <URL>

    # On RH based distros
    yum-config-manager --add-repo <URL>

This will enable ``zypper`` or ``yum`` to be used to install and
update the GEOPM Service packages.


Install Commands
----------------

The following bash commands will add the development branch OBS build
repository and install the GEOPM Service.

.. code-block:: bash

    # On SUSE based distros (e.g. dev branch - OpenSUSE 15.3)
    REPO_URL=https://download.opensuse.org/repositories/home:/geopm/15.3/home:geopm.repo
    zypper addrepo ${REPO_URL}
    zypper install -y geopm-service

    # On RH based distros (e.g. dev branch - CentOS 8)
    REPO_URL=https://download.opensuse.org/repositories/home:/geopm/CentOS_8/home:geopm.repo
    yum-config-manager --add-repo ${REPO_URL}
    yum install -y geopm-service


The ``dev`` branch is frequently updated.  At a later date after the
initial install, it may be useful to update the installation to match
the version on the ``dev`` branch.  To do this it is recommended that
you update all the packages built from the GEOPM source.

.. code-block:: bash

    # On SUSE based distros
    zypper update -y geopm-service libgeopmd0 python3-geopmdpy

    # On RH based distros
    yum update -y geopm-service libgeopmd0 python3-geopmdpy


Note that the development branch always has a version which is at
least as recent as the release candidate branch.  For this reason, if
both the development branch repository and the release candidate
repository are added to your package configuration, updates will
always come from the development branch repository.
