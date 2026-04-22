Spack Guide
===========

For users that leverage `Spack <https://spack.io/>`_ to distribute software,
recipes to build
`libgeopmd <https://github.com/spack/spack-packages/blob/develop/repos/spack_repo/builtin/packages/libgeopmd/package.py>`_,
`py-geopmdpy <https://github.com/spack/spack-packages/blob/develop/repos/spack_repo/builtin/packages/py_geopmdpy/package.py>`_,
`libgeopm <https://github.com/spack/spack-packages/blob/develop/repos/spack_repo/builtin/packages/libgeopm/package.py>`_,
and `py-geopmpy <https://github.com/spack/spack-packages/blob/develop/repos/spack_repo/builtin/packages/py_geopmpy/package.py>`_
are available in the `spack-packages
<https://github.com/spack/spack-packages>`_ repository.  These recipes
currently allow for building the v3.2.2 release of GEOPM and our main
development branch.

For deploying GEOPM's layers to a compute image in an HPC system context (i.e.
PXE booted via warewulf or similar), a typical configuration would be to have a
system install of the service RPMs baked into the compute image, and use spack
to install ``py-geopmpy``.  This is required as the GEOPM Service will be
launched via systemd, and thus must run against the system installed Python
runtime.

For GEOPM v3.2.2, system install ``geopmd``, ``geopmd-cli``, ``libgeopmd2``,
and ``python3-geopmdpy``.

If building for a system with different system images for user-login nodes vs.
compute nodes, the following split is recommended:

* Compute image: ``geopmd``, ``geopmd-cli``, ``libgeopmd2``,
  ``python3-geopmdpy``
* User-login image: ``geopmd-doc``, ``libgeopmd-doc``, ``libgeopmd-devel``,
  ``libgeopmd2``, ``python3-geopmdpy``

In order to build with spack this way, ``libgeopmd`` must be configured as an
external package in ``~/.spack/packages.yaml``:

.. code-block:: yaml

    packages:
      libgeopmd:
        externals:
        - spec: "libgeopmd@3.2.2"
          prefix: /usr
        version:
        - 3.2.2
        buildable: False

Afterwards, ``py-geopmpy`` can be installed normally with ``spack install
py-geopmpy``.
