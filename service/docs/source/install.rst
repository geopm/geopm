
Guide to Installation
=====================

Continuous integration creates packages for a variety of Linux
distributions each time certain branches hosted on GitHub are updated.
This is supported by GitHub Actions, in conjunction with the OpenSUSE
Build Service.

The packages built from this service with each update are:

- ``geopm-service``:
   Installs and activates the geopm systemd service
- ``libgeopmd0``:
   Provides the library that supports the PlatformIO interface
- ``python3-geopmdpy``:
   Implementation of geopmd, CLI tools, and bindings for PlatformIO
- ``geopm-service-devel``:
   Headers and man pages for C and C++ APIs provided by ``libgeopmd0``

In addition to these packages that are built each time a tracked
branch is updated, the download repositories also provide
``python3-dasbus`` version 1.6.  This version of dasbus is required by
``python3-geopmdpy``, and is not provided by the supported Linux
distributions.

Installing the ``geopm-service`` package will also install the
``libgeopmd0``, ``python3-geopmdpy`` and ``python3-dasbus`` dependency
packages.  The ``geopm-service-devel`` package must be explicitly
installed if it is required by the user.


Download Repositories
---------------------

The ``dev`` branch is the default branch containing the most up to
date stable development.  The ``release-v2.0-candidate`` branch will
point to a historical commit in the ``dev`` branch, and the release
candidate branch will be updated much less frequently than the
development branch.  The builds from these two branches have the
following download repository pages:

- Install Development Packages (``dev`` branch)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service>`__
   + `libgeopmd0 <https://software.opensuse.org/download.html?project=home%3Ageopm&package=libgeopmd0>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service-devel>`__

- Install Release Candidate Packages (``release-v2.0-candidate`` branch)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=geopm-service>`__
   + `libgeopmd0 <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=libgeopmd0>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=geopm-service-devel>`__


Use these links to install the GEOPM Service packages on your system.  The
links contain instructions and examples for each of the supported operating
systems.

The ``dev`` branch is frequently updated, and the
``release-v2.0-candidate`` branch will be updated several times prior
to the ``v2.0.0`` release.  At a later date, after the initial
install, it may be useful to update the installation to match the
upstream branch.

Note that the development branch always provides a version which is at
least as recent as the release candidate branch.  For this reason,
updates will always come from the development branch download
repository if both the development repository and the release
candidate repository are added to your package configuration,
