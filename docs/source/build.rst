Source Builds
=============

This documentation provides build instructions for administrators who are
interested in installing the GEOPM packages based on a checkout from the GEOPM
git repository. Following these instructions will assist users in creating RPM
or Debian packages compatible with various Linux distributions.

We have tested packages created through this process on SLES 15 SP3 and SP4,
as well as Ubuntu jammy 22.04.  Feedback from installations on other
distributions such as CentOS, openSUSE Leap, and Fedora would be greatly
appreciated.

GEOPM Service installation is available via both RPM and Debian packages. These
packages provide cross-Linux-distribution compatibility for the installation of
the SystemD service and facilitate a clean uninstall process.


Building Packages
-----------------

Use the following bash commands to build all of the GEOPM packages. Be advised
that the build may require package dependencies provided by the operating
system, but not installed by default.

.. code-block:: bash

   git clone git@github.com:geopm/geopm.git
   cd geopm
   ./package.sh

These commands create the .rpm files or .deb files that package the GEOPM
software.

The ``package.sh`` script will pass any command line options to the
configure commands used to create source archive.

The ``package.sh`` script can be used to build only the GEOPM Access Service
packages by setting the ``GEOPM_PACKAGE_SKIP_RUNTIME`` environment variable to
any value.  This may be required to bootstrap the GEOPM Runtime Service build
requirements.

In the example below the the ``package.sh`` script is used to build the GEOPM
Access service packages with LevelZero enabled.  These packages are installed
and then the ``package.sh`` script is rerun to generate the GEOPM Runtime
Service packages, and then these are installed.

.. code-block:: bash

   # Build libgeopmd with LevelZero support
   GEOPM_PACKAGE_SKIP_RUNTIME=1 \
   ENABLE_LEVELZERO=1 \
   ./package.sh
   sudo apt install -y $(find -name *.deb)
   # Build GEOPM Access Service
   GEOPM_PACKAGE_SKIP_RUNTIME=1 \
   ENABLE_LEVELZERO=1 \
   ./package.sh
   # Install packages
   sudo apt install -y $(find -name *.deb)
   # Build the GEOPM Runtime packages
   ENABLE_LEVELZERO=1 \
   ./package.sh --disable-mpi \
                --disable-openmp \
                --disable-fortran \
                --disable-geopmd-local
   sudo apt install -y $(find -name *.deb)

On Ubuntu, these packages are build in the tree:

.. code-block:: bash

   ./docs/geopmd-doc_<VERSION>-1_all.deb
   ./docs/libgeopmd-doc_<VERSION>-1_all.deb
   ./docs/libgeopm-doc_<VERSION>-1_all.deb
   ./docs/python3-geopmpy-doc_<VERSION>-1_all.deb
   ./geopmdpy/geopmd_<VERSION>-1_all.deb
   ./geopmdpy/python3-geopmdpy_<VERSION>-1_all.deb
   ./geopmdrs/target/debian/geopmd-proxy_<VERSION>-1_amd64.deb
   ./geopmpy/python3-geopmpy_<VERSION>-1_all.deb
   ./libgeopmd/geopmd-cli_<VERSION>-1_amd64.deb
   ./libgeopmd/libgeopmd2_<VERSION>-1_amd64.deb
   ./libgeopmd/libgeopmd-dev_<VERSION>-1_amd64.deb
   ./libgeopm/geopm-cli_<VERSION>-1_amd64.deb
   ./libgeopm/libgeopm2_<VERSION>-1_amd64.deb
   ./libgeopm/libgeopm-dev_<VERSION>-1_amd64.deb

In RPM based operating systems the RPMs are created in the rpmbuild root
(default is ~/rpmbuild).

.. code-block:: bash

   ./rpmbuild/RPMS/x86_64/geopm-cli-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/geopmd-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/geopmd-cli-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/geopmd-doc-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/geopm-doc-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopm2-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopmd2-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopmd-devel-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopmd-doc-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopm-devel-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/libgeopm-doc-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/python3-geopmdpy-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/python3-geopmpy-<VERSION>-1.x86_64.rpm
   ./rpmbuild/RPMS/x86_64/python3-geopmpy-doc-<VERSION>-1.x86_64.rpm


Building Containers
-------------------

Please refer to the `GEOPM Kubernetes documentation
<https://github.com/geopm/geopm/tree/dev/integration/k8#building-docker-containers>`_
for how to build containers using Docker.  The README describes how to use the
Docker files and associated scripting in the directory to create container
images with the GEOPM software installed.  These containers may be used with
Kubernetes to provide the GEOPM Access Service as a Kubernetes Daemonset.  This
configuration runs the GEOPM Access Service in a privileged container to enable
unprivileged containers to Access low level telemetry and control knobs from the
OS based on a restricted access list.
