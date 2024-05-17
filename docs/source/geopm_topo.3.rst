geopm_topo(3) -- query platform component topology
====================================================

Synopsis
--------

#include `<geopm_topo.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_topo.h>`_

``Link with -lgeopm (MPI) or -lgeopm (non-MPI)``

.. code-block:: c

  int geopm_topo_num_domain(int domain_type);

  int geopm_topo_domain_idx(int domain_type,
                            int cpu_idx);

  int geopm_topo_num_domain_nested(int inner_domain,
                                   int outer_domain);

  int geopm_topo_domain_nested(int inner_domain,
                               int outer_domain,
                               int outer_idx,
                               size_t num_domain_nested,
                               int *domain_nested);


  int geopm_topo_domain_name(int domain_type,
                             size_t domain_name_max,
                             char *domain_name);

  int geopm_topo_domain_type(const char *domain_name);

  int geopm_topo_create_cache(void);

Domain Types
------------

The ``geopm_domain_e`` enum defines a set of values that correspond to
hardware components on the system:

``GEOPM_DOMAIN_INVALID = -1``
    Indicates an invalid domain.

``GEOPM_DOMAIN_BOARD = 0``
    All components on a user allocated compute node. There is only a
    single board per compute node, and every other domain is contained
    with the board domain.

``GEOPM_DOMAIN_PACKAGE = 1``
    A collection of all the hardware present on a single processor
    package installed on a distinct socket of a motherboard.

``GEOPM_DOMAIN_CORE = 2``
    Physical core, i.e. a group of associated hyper-threads

``GEOPM_DOMAIN_CPU = 3``
    Linux logical CPU.  In practice, there is one logical CPU per
    hyperthread visible to the operating system.

``GEOPM_DOMAIN_MEMORY = 4``
    Standard off-package DIMM (DRAM or NAND).

``GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY = 5``
    On-package memory (MCDRAM).

``GEOPM_DOMAIN_NIC = 6``
    Peripheral network interface controller not on the processor package.

``GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC = 7``
    Network interface controller on the processor package.

``GEOPM_DOMAIN_GPU = 8``
    Peripheral GPU card not on the processor package.

``GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU = 9``
    GPU unit on the package (e.g on-package graphics).

``GEOPM_DOMAIN_GPU_CHIP = 10``
    GPU card chips within a package on the PCI Bus (e.g Level Zero subdevices).

``GEOPM_NUM_DOMAIN = 11``
    The number of valid built-in domains.

Description
-----------

The interfaces described in this man page are the C language bindings for the
:doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>` C++ class.  Please
refer to the :doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>` man
page for a general description of the purpose, goals and use case for this
interface.

``geopm_topo_num_domain()``
  Returns the number of domains available on the system of type
  *domain_type*.  If the *domain_type* is valid, but there are no
  domains of that type on the system, the return value is zero.  If
  the domain is not a valid domain defined by the ``geopm_domain_e``
  enum then the function will return a negative error code:
  ``GEOPM_ERROR_INVALID``.

``geopm_topo_domain_idx()``
  Returns the index of the domain of type *domain_type* that is local to the
  Linux logical CPU *cpu_idx*.  The return value will be greater than or equal
  to zero and less than the value returned by ``geopm_topo_num_domain()`` for
  valid input parameters.  If *domain_type* or *cpu_idx* are out of range a
  negative error code is returned:  ``GEOPM_ERROR_INVALID``.

``geopm_topo_num_domain_nested()``
  Returns the number of domains of type *inner_domain* that are contained
  within each domain of *outer_domain* type.  The return value is one if
  *inner_domain* is equal to *outer_domain*.  If *inner_domain* is not
  contained within *outer_domain* a negative error code is returned:
  ``GEOPM_ERROR_INVALID``.  Any non-negative return value can be used to size
  the *domain_nested* array that is passed to ``geopm_topo_domain_nested()``
  with the same values for *inner_domain* and *outer_domain*.

``geopm_topo_domain_nested()``
  Fills the output array *domain_nested* with the domain indices of all of the
  *inner_domain* types nested within the specific *outer_domain* type indexed
  by *outer_idx*.  *num_domain_nested* defines the length of the
  *domain_nested* array and must match the positive return value from
  ``geopm_topo_num_domain_nested()``.  Zero is returned upon success.  A
  negative error code is returned if *inner_domain* is not within
  *outer_domain*, or if *outer_idx* is not between zero and
  ``geopm_topo_num_domain()``.

``geopm_topo_domain_name()``
  Sets the *domain_name* string to the name associated with the
  *domain_type* selected from the ``geopm_domain_e`` enum.  At most
  *result_max* bytes are written to the *domain_name* string.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``) will be
  sufficient for storing any result.  If *result_max* is too small
  to contain the domain name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

``geopm_topo_domain_type()``
  Returns the domain type that is associated with the provided
  *domain_name* string.  This is the inverse function to
  ``geopm_topo_domain_name()`` and the input *domain_name* must match
  the output from ``geopm_topo_domain_name()`` for a valid domain
  type.  If the string does not match any of the valid domain names,
  then ``GEOPM_DOMAIN_INVALID`` is returned.

``geopm_topo_create_cache()``:
  Create a cache file for the :doc:`geopm::PlatformTopo(3)
  <geopm::PlatformTopo.3>` object if one does not exist.  This cache
  file will be used by any calls to the other ``geopm_topo_*()`` functions
  documented here as well as any use of the GEOPM runtime.  If a privileged
  user is making this call (i.e. root or via sudo), the file path will be
  ``/run/geopm/geopm-topo-cache``. If a non-privileged user makes this call
  file path will be ``/tmp/geopm-topo-cache-<UID>``. In either case, the
  permissions will be ``-rw-------``, i.e. 600.  If the file exists from the
  current boot cycle and has the proper permissions no operation will be
  performed.  To force the creation of a new cache file, `unlink(3)
  <https://man7.org/linux/man-pages/man3/unlink.3p.html>`_ the existing cache
  file prior to calling this function.

Return Value
------------

If an error occurs in any call to an interface documented here, the
return value of the function will be a negative integer
corresponding to one of the error codes documented in
:doc:`geopm_error(3) <geopm_error.3>`.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_pio(3) <geopm_pio.3>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`\ ,
:doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>`\ ,
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`\ ,
`unlink(3) <https://man7.org/linux/man-pages/man3/unlink.3p.html>`_
