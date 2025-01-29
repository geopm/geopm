.. spack_build:

Building with Spack
===================

Spack is a flexible package manager that supports multiple versions,
configurations, platforms, and compilers. This guide provides detailed
instructions on how to build and install GEOPM using Spack.

Installing Spack
----------------

First, you need to install Spack. Follow the instructions on the `Spack website
<https://spack.io>`_ to clone the Spack repository and set up your environment.

.. code-block:: bash

    $ git clone https://github.com/spack/spack.git
    $ . spack/share/spack/setup-env.sh

Adding GEOPM to Spack
---------------------

GEOPM is included in the Spack package repository. To add GEOPM to your Spack
environment, use the following command:

.. code-block:: bash

    $ spack install geopm

Building
--------

To build GEOPM with Spack, you can specify the desired version and any
additional variants or dependencies. For example, to build the latest version of
GEOPM:

.. code-block:: bash

    $ spack install geopm

You can also specify a particular version:

.. code-block:: bash

    $ spack install geopm@3.1.0

For users that leverage `Spack <https://spack.io/>`_ to distribute software,
recipes to build the `geopm-service
<https://github.com/spack/spack/blob/v0.22.0/var/spack/repos/builtin/packages/geopm-service/package.py>`_
and `geopm-runtime
<https://github.com/spack/spack/blob/v0.22.0/var/spack/repos/builtin/packages/geopm-runtime/package.py>`_
have been included in their `v0.22.0 release
<https://github.com/spack/spack/tree/v0.22.0>`_. These recipes currently allow
for building both the v3.0.1 release of GEOPM and our main development branch.

For deploying GEOPM's layers to a compute image in an HPC system context (i.e.,
PXE booted via warewulf or similar), a typical configuration would be to have a
system install of the service RPMs baked into the compute image, and use Spack
to install ``geopm-runtime``. This is required as the GEOPM Service will be
launched via systemd, and thus must run against the system installed Python
runtime.

For GEOPM v3.0.1, system install ``geopm-service``, ``geopm-service-devel``,
``libgeopmd2``, and ``python3-geopmdpy``.

For GEOPM v3.1, system install ``geopm-service``, ``geopm-service-doc``,
``geopm-service-devel``, ``libgeopmd2``, ``libgeopmd-doc``,
``python3-geopmdpy``, and ``python3-geopmdpy-doc``.

Configuring
-----------

You can customize the build by specifying variants and dependencies. For
example, to build GEOPM with MPI support:

.. code-block:: bash

    $ spack install geopm +mpi

You can view all available variants and dependencies for GEOPM by running:

.. code-block:: bash

    $ spack info geopm

Environment Module
------------------

Once GEOPM is installed, you can load the environment module to use it:

.. code-block:: bash

    $ spack load geopm

This will set up your shell environment to use the GEOPM executables and libraries.

Updating GEOPM
--------------

To update GEOPM to the latest version, you can use the following commands:

.. code-block:: bash

    $ spack uninstall geopm
    $ spack install geopm

Contributing to Spack
---------------------

If you want to contribute to the GEOPM Spack recipes, you can find them in the
Spack repository. Follow the Spack contribution guidelines to submit your
changes.

For more information, visit the `Spack documentation <https://spack.readthedocs.io/en/latest/>`_.

----

For additional information on installing GEOPM, see :doc:`install`.

