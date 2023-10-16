Installation
============

Each time specified branches hosted on GitHub are updated, continuous
integration generates packages for numerous Linux distributions. This
process is uses GitHub Actions, combined with the OpenSUSE Build Service
(OBS). Here are the packages curated by OBS:

- ``geopm-service``: Sets up and enables the geopm systemd service.
- ``libgeopmd2``: Provides the library that supports the PlatformIO interface.
- ``python3-geopmdpy``: Houses the implementation of geopmd, CLI utilities, and bindings for PlatformIO.
- ``geopm-service-devel``: Provides headers and man pages for C and C++ APIs supportable by ``libgeopmd2``.

Besides these routinely curated packages, the download repositories also grant
access to ``python3-dasbus`` version 1.6. This version is a prerequisite
for ``python3-geopmdpy``, but not included by the supported Linux distributions.

Installation of ``geopm-service`` will involve the dependency packages:
``libgeopmd2``, ``python3-geopmdpy`` and ``python3-dasbus``. However, explicit
installation is obligatory for anyone seeking to use ``geopm-service-devel``.


Prerequisites for Level Zero
----------------------------

Before you install the GEOPM Service packages, ensure you have the
``level-zero`` package. This package supports GEOPM for GPU metrics and
controls. Currently, none of the Linux distributions supporting GEOPM
provide this package upstream.

To install ``level-zero``, use the `Intel(R)
GPGPU software package installation guide for Linux
<https://dgpu-docs.intel.com/installation-guides/index.html>`__. Once
added your system's OS appropriate package management repository,
you can use ``yum`` or ``zypper`` to install the package. Note that the
``level-zero-devel`` package is optional unless building GEOPM from source
with GPU support is intended.


Download Repositories
---------------------

The ``release-v2.0`` branch logs v2.0 releases, including any potential
hotfixes for v2.0.0. It captures the latest stable GEOPM release. The ``dev``
branch, however, presents the most updated stable development. Builds from
these two branches have the following download repository pages:

- Release Packages Installation (``release-v2.0`` branch)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm%3Arelease-v2.0&package=geopm-service-devel>`__

- Development Packages Installation (``dev`` branch)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service-devel>`__

Use the provided links to install GEOPM Service packages on your system. The
links will also guide you through instructions for each supported operating
system. If you add both the development repository and the release repository
to your package configuration, you will always receive updates from the
development branch.


What the Packages Don't Include
-------------------------------

Please note that the packages listed above do not offer the :doc:`GEOPM HPC
Runtime <runtime>` features (e.g. :doc:`geopmlaunch(1) <geopmlaunch.1>`,
:doc:`geopm_prof(3) <geopm_prof.3>`, :doc:`geopm_report(7) <geopm_report.7>`,
``libgeopm.so`` and others.)

The GEOPM HPC Runtime should be built against a specific implementation of
MPI, often unique to the system running the HPC application. An exception to
this rule is the OpenHPC distribution. We've packaged GEOPM version 1 with
`OpenHPC releases <http://openhpc.community/downloads/>`_, and we hope to
offer version 2 in the future.

The packages above don't provide support for NVIDIA GPUs with NVML or
DCGM. This support depends on libraries not included in the OpenSUSE OBS
build system where the GEOPM packages are curated. To enable the use of NVML,
or DCGM, you will need to source-build the GEOPM Service, which requires
an environment supporting the dependency libraries and headers.

For GEOPM features not included in these packages, build GEOPM from
source. The best instructions for this process can be found in the
:ref:`developer build process <devel:developer build process>` within the
:doc:`developer guide <devel>`. Keep in mind that you may need to git checkout
a git tag (e.g. ``v2.0.0``) to create a build based on a specific release.
