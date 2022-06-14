
Guide to Installation
=====================

Continuous integration creates packages for a variety of Linux
distributions each time certain branches hosted on GitHub are updated.
This is supported by GitHub Actions, in conjunction with the OpenSUSE
Build Service.

The packages built from this service with each update are:

- ``geopm-service``
- ``libgeopmd0``
- ``python3-geopmdpy``
- ``geopm-service-devel``

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

- ``dev``
   + `Install Development Packages <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service>`__
- ``release-v2.0-candidate``
   + `Install Release Candidate Packages <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0-candidate&package=geopm-service>`__

Use one of these links to install the GEOPM Service on your system.
The links contain instructions and examples for each of the supported
operating systems.

The ``dev`` branch is frequently updated, and the
``release-v2.0-candidate`` branch will be updated several times prior
to the ``v2.0.0`` release.  At a later date, after the initial
install, it may be useful to update the installation to match the
upstream branch.  To do this it is recommended that you update all of
the installed packages built from the GEOPM source (including the
optional ``geopm-service-devel`` package if it is installed):

.. code-block:: bash
    # On SUSE based distros
    zypper update -y geopm-service libgeopmd0 python3-geopmdpy


.. code-block:: bash
    # On RH based distros
    yum update -y geopm-service libgeopmd0 python3-geopmdpy


Note that the development branch always provides a version which is at
least as recent as the release candidate branch.  For this reason,
updates will always come from the development branch download
repository if both the development repository and the release
candidate repository are added to your package configuration,
