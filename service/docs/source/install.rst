
Guide to Service Installation
=============================

Continuous integration creates packages for a variety of Linux
distributions each time certain branches hosted on GitHub are updated.
This is supported by GitHub Actions, in conjunction with the OpenSUSE
Build Service (OBS).

Packages built by OBS:

- ``geopm-service``:
   Installs and activates the geopm systemd service
- ``libgeopmd1``:
   Provides the library that supports the PlatformIO interface
- ``python3-geopmdpy``:
   Implementation of geopmd, CLI tools, and bindings for PlatformIO
- ``geopm-service-devel``:
   Headers and man pages for C and C++ APIs provided by ``libgeopmd1``

In addition to these packages that are built each time a tracked
branch is updated, the download repositories also provide
``python3-dasbus`` version 1.6.  This version of dasbus is required by
``python3-geopmdpy``, and is not provided by the supported Linux
distributions.

Installing the ``geopm-service`` package will also install the
``libgeopm1``, ``python3-geopmdpy`` and ``python3-dasbus`` dependency
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
   + `libgeopm1 <https://software.opensuse.org/download.html?project=home%3Ageopm&package=libgeopm1>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service-devel>`__

- Install Release Candidate Packages (``release-v2.0-candidate`` branch)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=geopm-service>`__
   + `libgeopm1 <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=libgeopm1>`__
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


Un-packaged Features
--------------------

The packages described above do not provide the
:doc:`GEOPM HPC Runtime <runtime>` features (e.g.
:doc:`geopmlaunch(1) <geopmlaunch.1>`, :doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopm_report(7) <geopm_report.7>`, ``libgeopm.so``, etc.).
The GEOPM HPC Runtime must be built against a particular implementation of the
Message Passing Interface (MPI).  Typically the implementation of MPI is
specific to the system where the HPC application is being run.  An exception
to this rule is the OpenHPC distribution.  GEOPM version 1 has been packaged
with `OpenHPC releases <http://openhpc.community/downloads/>`_, and we hope to distribute version 2 with OpenHPC in the future.
For users interested in GEOPM version 2, a source build is required.

Support for GPUs is also not provided by installing the packages described
above.  Support for GPUs relies on libraries that are not bundled with the
OpenSUSE OBS build system where the GEOPM packages are built and distributed.
To enable LevelZero, NVML, or DCGM a source build of the GEOPM Service is
required.  This build process must be done in an environment where the
dependency libraries and headers are present.

In order to access these GEOPM features which are not packaged, a user should
build GEOPM from source.  The best recommendation for building GEOPM from
source is to follow the :ref:`developer build process <devel:developer build process>`
posted in the :doc:`developer guide <devel>`.  Note that it may be of interest
to git checkout a git tag (e.g. ``v2.0.0+rc2``) to create a build based on a
particular release.
