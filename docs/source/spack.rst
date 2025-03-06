Spack Guide
===========

For users that leverage `Spack <https://spack.io/>`_ to distribute software,
recipes to build the `geopm-service
<https://github.com/spack/spack/blob/v0.22.0/var/spack/repos/builtin/packages/geopm-service/package.py>`_
and `geopm-runtime
<https://github.com/spack/spack/blob/v0.22.0/var/spack/repos/builtin/packages/geopm-runtime/package.py>`_
have been included in their `v0.22.0 release
<https://github.com/spack/spack/tree/v0.22.0>`_.  These recipes currently allow
for building both the v3.0.1 release of GEOPM and our main development branch.

For deploying GEOPM's layers to a compute image in an HPC system context (i.e.
PXE booted via warewulf or similar), a typical configuration would be to have a
system install of the service RPMs baked into the compute image, and use spack
to install ``geopm-runtime``.  This is required as the GEOPM Service will be
launched via systemd, and thus must run against the system installed Python
runtime.

For GEOPM v3.0.1, system install ``geopm-service``, ``geopm-service-devel``,
``libgeopmd2``, and ``python3-geopmdpy``

For GEOPM v3.1, system install ``geopm-service``, ``geopm-service-doc``,
``geopm-service-devel``, ``libgeopmd2``, ``libgeopmd-doc``,
``python3-geopmdpy``, and ``python3-geopmdpy-doc``

In order to build with spack this way, ``geopm-service`` must be configured as an
external package in ``~/.spack/packages.yaml``:

.. code-block:: yaml

    packages:
      geopm-service:
        externals:
        - spec: "geopm-service@3.0.1"
          prefix: /usr
        - spec: "geopm-service@3.1.0"
          prefix: /usr
        - spec: "geopm-service@develop"
          prefix: /usr
        version:
        - 3.0.1
        - 3.1.0
        - develop
        buildable: False

Afterwards, ``geopm-runtime`` can be installed normally with ``spack install
geopm-runtime``.
