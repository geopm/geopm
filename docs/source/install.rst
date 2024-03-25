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

Installation of ``geopm-service`` will involve the dependency packages:
``libgeopmd2``, ``python3-geopmdpy`` and ``python3-dasbus``. However, explicit
installation is obligatory for anyone seeking to use ``geopm-service-devel``.

.. note::

    Besides these routinely curated packages, the download repositories also
    grant access to ``python3-dasbus`` version 1.6. This version is a
    prerequisite for ``python3-geopmdpy``, but it is not included by some of
    the Linux distributions that support GEOPM.  In particular, starting with
    SLES 15.4 the necessary ``python3-dasbus`` version is available in the
    regular package repositories.  This support will be removed from OBS in
    a future release.

----

GPU Support
-----------

In order to leverage the GEOPM Service's support for GPUs, you must install the
necessary libraries for either Intel GPUs (levelzero) or Nvidia GPUs (NVML).

Prerequisites for Level Zero
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``levelzero`` allows GEOPM to read metrics and enact controls from Intel GPUs.
Currently, none of the Linux distributions supporting GEOPM provide this
package upstream.

To install ``level-zero``, use the one of the following:

For up-to-date distributions (e.g. SLES 15.4 and Ubuntu 22.04):

* `Intel(R) software for general purpose GPU capabilities
  <https://dgpu-docs.intel.com/driver/installation.html>`__

For older, deprecated distributions (e.g. SLES 15.3 and Ubuntu 20.04):

* `Intel(R) software for general purpose GPU capabilities
  <https://dgpu-docs.intel.com/installation-guides/index.html>`__

Once added your system's OS appropriate package management repository, you can
use ``yum``, ``zypper``, or ``apt`` to install the package.

.. note::

   The ``level-zero-devel`` package is optional unless building GEOPM from
   source with GPU support.

Prerequisites for NVML
^^^^^^^^^^^^^^^^^^^^^^

.. TODO Include blurb about DCGM, where to get it, and why it's important.
   This only makes sense if/when we build with DCGM in something that is packaged
   on Launchpad or OBS.

To build with support for Nvidia's GPUs please follow their installation guide
for CUDA here:

* `NVIDIA CUDA Installation Guide for Linux
  <https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html>`__

.. note::

   Source builds that use ``--enable-nvml`` require the following package to be
   installed: ``libnvidia-ml-dev``

----

Download Repositories
---------------------

The ``release-v3.0`` branch tracks the v3.0 GEOPM release, including any
potential hotfixes that could occur in the future. It captures the latest
stable GEOPM release.

The ``dev`` branch, however, presents the most up-to-date stable development.
See below for download repository information for your Linux distribution.

SLES, OpenSUSE, and CentOS
^^^^^^^^^^^^^^^^^^^^^^^^^^

When using the below links, it is preferable to click on your desired OS and
follow the procedure for "Add repositoriy and install manually".  If you do
this, installing the ``geopm-service`` package will automatically install the
remaining dependencies (e.g. ``libgeopmd2``, etc.).  If required for source
builds, you still must manually install ``geopm-service-devel``.

While you *can* download the binary packages directly and install through your
package manager, you must specify all of the required packages at install time
(i.e. ``geopm-service``, ``libgeopmd2``, ``python3-geopmdpy``, and optionally
``geopm-service-devel``).

.. note::

   You will not automatically pull updates and bug fixes unless the repository
   is added.

- Release Packages (``release-v3.0`` branch - Intel GPU support via levelzero)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=geopm-service-devel>`__

- Release Packages (``release-v3.0`` branch - no GPU support)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=geopm-service-devel>`__

- Development Packages (``dev`` branch - Intel GPU support via levelzero)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=geopm-service-devel>`__

- Development Packages (``dev`` branch - no GPU support)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service-devel>`__

.. warning::

   Do not add more than one of the above repositories to your system package
   manager at the same time.  Only add one, and ensure all GEOPM packages are
   completely removed from the system when changing GEOPM repo configuration in
   the package manager.

Ubuntu
^^^^^^

There are 2 repositories that are maintained for GEOPM support on Ubuntu: one
corresponding to the ``release-v3.0`` branch while the other corresponds to the
``dev`` branch.  Both are built with Nvidia GPU support **only**.

First, add the necessary upstream repository:

.. code-block:: bash

    # ONLY DO ONE OF THE FOLLOWING add-apt-repository COMMANDS:

    # Add the release repo:
    $ sudo add-apt-repository ppa:geopm/release
    # OR add the dev repo:
    $ sudo add-apt-repository ppa:geopm/dev

Then pull all the current updates, install GEOPM, start/enable the service, and
configure the initial access lists:

.. code-block:: bash

    $ sudo apt update
    $ apt install geopm-service libgeopmd-dev libgeopmd2 python3-geopmdpy
    # Start and enable the service
    $ sudo systemctl start geopm
    $ sudo systemctl enable geopm
    # Setup initial access: all users can access all signals and controls
    $ sudo geopmaccess -a | sudo geopmaccess -w
    $ sudo geopmaccess -ac | sudo geopmaccess -wc

For more information see:

 * `GEOPM release repo on Launchpad
   <https://launchpad.net/~geopm/+archive/ubuntu/release>`__
 * `GEOPM dev repo on Launchpad
   <https://launchpad.net/~geopm/+archive/ubuntu/dev>`__


.. MOVE TO SOURCE BUILD PAGE
.. .. note::

..    Source builds that use ``--enable-nvml`` require the following package to be
..    installed: ``libnvidia-ml-dev``

----

What the Packages Don't Include
-------------------------------

Please note that the packages listed above do not offer the :doc:`GEOPM Runtime
<runtime>` features (e.g. :doc:`geopmlaunch(1) <geopmlaunch.1>`,
:doc:`geopm_prof(3) <geopm_prof.3>`, :doc:`geopm_report(7) <geopm_report.7>`,
``libgeopm.so`` and others.)

For information on how to install the GEOPM Runtime, see :doc:`GEOPM Runtime
<runtime>`.

For GEOPM features not included in these packages, build GEOPM from
source. The best instructions for this process can be found in the
:ref:`developer build process <devel:developer build process>` within the
:doc:`developer guide <devel>`. Keep in mind that you may need to git checkout
a git tag (e.g. ``v3.0.0``) to create a build based on a specific release.
