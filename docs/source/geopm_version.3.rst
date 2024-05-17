
geopm_version(3) -- GEOPM library version
=========================================






Synopsis
--------

#include `<geopm_version.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_version.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c

       const char *geopm_version(void);

Description
-----------

``geopm_version()``
  Returns a human readable string describing the version of the GEOPM library
  linked to the calling application.  The form is ``<MAJOR>.<MINOR>.<HOTFIX>`` for
  released versions and ``<MAJOR>.<MINOR>.<HOTFIX>+dev<COUNT>g<HASH>`` for
  development snapshots.  ``MAJOR`` is the major revision number, ``MINOR`` is the minor
  revision number, ``HOTFIX`` is the hotfix iteration, ``COUNT`` is the number of git
  commits since the tagged release and ``HASH`` is the **git SHA-1** hash of the last
  commit.  For example ``"0.0.1"`` is a release version, ``"0.0.1+dev21g986987f"`` is a
  development snapshot ``21`` commits away from the ``0.0.1`` release with the **git SHA-1**
  that begins with ``"986987f"``.

See Also
--------

:doc:`geopm(7) <geopm.7>`
