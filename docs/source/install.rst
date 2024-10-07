Installation
============

The packages provided below correspond to branches hosted on GitHub.  The
packages are updated through continuous integration via GitHub Actions for
numerous Linux distributions.  RPM-based distributions are hosted on `openSUSE
Build Service (OBS) <https://build.opensuse.org/>`_ while Debian-based
distributions are hosted on `Launchpad <https://launchpad.net/>`_.

Here are the packages curated by OBS and Launchpad:

v3.1.0 to Current
-----------------

- ``geopm-service``: Sets up and enables the geopm systemd service.
- ``libgeopmd2``: Provides the library that supports the PlatformIO interface.
- ``python3-geopmdpy``: Houses the implementation of geopmd, CLI utilities, and bindings for PlatformIO.
- ``geopm-service-devel``: Provides headers and man pages for C and C++ APIs supportable by ``libgeopmd2``.
- ``geopm-doc``: Provides all GEOPM man pages.

v3.0.1
------

- ``geopm-service``: Sets up and enables the geopm systemd service.
- ``libgeopmd2``: Provides the library that supports the PlatformIO interface.
- ``python3-geopmdpy``: Houses the implementation of geopmd, CLI utilities, and bindings for PlatformIO.
- ``geopm-service-devel``: Provides headers and man pages for C and C++ APIs supportable by ``libgeopmd2``.

Installation of ``geopm-service`` will involve the dependency packages:
``libgeopmd2``, ``python3-geopmdpy`` and ``python3-dasbus``. However, explicit
installation is obligatory for anyone seeking to use ``geopm-service-devel`` or
the docs packages.

----

GPU Support
-----------

In order to leverage the GEOPM Service's support for GPUs, you must install the
necessary libraries for either Intel GPUs (levelzero) or Nvidia GPUs (NVML).

Prerequisites for Level Zero
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``level-zero`` allows GEOPM to read metrics and enact controls from Intel GPUs.
Currently, none of the Linux distributions supporting GEOPM provide this
package upstream.

To install ``level-zero``, use the one of the following:

For up-to-date distributions (e.g. SLES 15.4 and Ubuntu 22.04):

* `Intel(R) software for general purpose GPU capabilities
  <https://dgpu-docs.intel.com/driver/installation.html>`__

Once added your system's OS appropriate package management repository, you can
use ``yum``, ``zypper``, or ``apt`` to install the package.

.. note::

   The ``level-zero-devel`` package is optional unless building GEOPM from
   source with GPU support.

.. warning::

   In order to avoid a race condition that exists between the availability
   of the Level Zero interfaces and GEOPM, the kernel graphics driver that
   provides the backing for these interfaces, i915, must be loaded before
   the GEOPM systemd service starts.  Otherwise no GPU signals nor controls
   will be available if the GEOPM systemd service starts before i915 has
   completed initialization.

   The correct load order can be enforced via the systemd-modules-load
   service.  Simply add a file in ``/etc/modules-load.d`` called
   ``i915.conf`` with the only contents being ``i915``.  This forces i915
   to load before the GEOPM systemd service, and resolves the race
   condition.

   The kernel command line can be modified instead for this purpose if
   modifying the file system is not an option.  More information:

   * `modules-load.d <https://www.freedesktop.org/software/systemd/man/latest/modules-load.d.html>`__

   * `systemd-modules-load.service
     <https://www.freedesktop.org/software/systemd/man/latest/systemd-modules-load.service.html>`__

   * `SLES 15-SP4 - Loading kernel modules automatically on boot
     <https://documentation.suse.com/sles/15-SP4/html/SLES-all/cha-mod.html#sec-mod-modprobe-d>`__

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

The ``release-v3.1`` branch tracks the v3.1 GEOPM release, including any
potential hotfixes that could occur in the future. It captures the latest
stable GEOPM release.

The ``release-v3.0`` branch tracks the v3.0 GEOPM release, including any
potential hotfixes that could occur in the future. It captures the previous
stable GEOPM release.

The ``dev`` branch, however, presents the most up-to-date stable development.
See below for download repository information for your Linux distribution.

SLES, openSUSE, and CentOS
^^^^^^^^^^^^^^^^^^^^^^^^^^

When using the below links, it is preferable to click on your desired OS and
follow the procedure for "Add repositoriy and install manually".  If you do
this, installing the ``geopm-service`` package will automatically install the
remaining dependencies and recommended packages (e.g. ``libgeopmd2``,
``geopm-service-doc``, etc.).  If required for source builds, you still must
manually install ``geopm-service-devel``.

While you *can* download the binary packages directly and install through your
package manager, you must specify all of the required packages at install time
(i.e. ``geopm-service``, ``libgeopmd2``, ``python3-geopmdpy``).  Optionally
you may install the following to enable source builds and documentation:
``geopm-service-devel``, ``geopm-service-doc``, ``libgeopmd-doc``, and
``python3-geopmdpy-doc``.

.. note::

   You will not automatically pull updates and bug fixes unless the repository
   is added.

v3.1
""""

- Release Packages (``release-v3.1`` branch - Intel GPU support via levelzero)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=geopm-service-devel>`__
   + `geopm-doc <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease%3Asupplementary&package=geopm-doc>`__

- Release Packages (``release-v3.1`` branch - no GPU support)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=geopm-service-devel>`__
   + `geopm-doc <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease&package=geopm-doc>`__

v3.0
""""

- Release Packages (``release-v3.0`` branch - Intel GPU support via levelzero)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0%3Asupplementary&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0%3Asupplementary&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0%3Asupplementary&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0%3Asupplementary&package=geopm-service-devel>`__

- Release Packages (``release-v3.0`` branch - no GPU support)
   + `geopm-service <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0&package=geopm-service>`__
   + `libgeopm2 <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org//download.html?project=home%3Ageopm%3Arelease-v3.0&package=geopm-service-devel>`__

Development
"""""""""""

- Development Packages (``dev`` branch - Intel GPU support via levelzero)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm%3Asupplementary&package=geopm-service-devel>`__
   + `geopm-doc <https://software.opensuse.org//download.html?project=home%3Ageopm%3Asupplementary&package=geopm-doc>`__

- Development Packages (``dev`` branch - no GPU support)
   + `geopm-service <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service>`__
   + `libgeopmd2 <https://software.opensuse.org/download.html?project=home%3Ageopm&package=libgeopmd2>`__
   + `python3-geopmdpy <https://software.opensuse.org/download.html?project=home%3Ageopm&package=python3-geopmdpy>`__
   + `geopm-service-devel <https://software.opensuse.org/download.html?project=home%3Ageopm&package=geopm-service-devel>`__
   + `geopm-doc <https://software.opensuse.org//download.html?project=home%3Ageopm&package=geopm-doc>`__

.. warning::

   Do not add more than one of the above repositories to your system package
   manager at the same time.  Only add one, and ensure all GEOPM packages are
   completely removed from the system when changing GEOPM repo configuration in
   the package manager.

Ubuntu
^^^^^^

There are 2 repositories that are maintained for GEOPM support on Ubuntu: one
corresponding to the ``release-v3.1`` branch while the other corresponds to the
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
