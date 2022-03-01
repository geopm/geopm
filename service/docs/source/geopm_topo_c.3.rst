.. role:: raw-html-m2r(raw)
   :format: html


geopm_topo_c(3) -- query platform component topology
====================================================






SYNOPSIS
--------

#include `<geopm_topo.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_topo.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``int geopm_topo_num_domain(``\ :
  ``int`` _domain\ *type*\ ``);``

* 
  ``int geopm_topo_domain_idx(``\ :
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``int`` _cpu\ *idx*\ ``);``

* 
  ``int geopm_topo_num_domain_nested(``\ :
  ``int`` _inner\ *domain*\ , :raw-html-m2r:`<br>`
  ``int`` _outer\ *domain*\ ``);``

* 
  ``int geopm_topo_domain_nested(``\ :
  ``int`` _inner\ *domain*\ , :raw-html-m2r:`<br>`
  ``int`` _outer\ *domain*\ , :raw-html-m2r:`<br>`
  ``int`` _outer\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _num_domain\ *nested*\ , :raw-html-m2r:`<br>`
  `int *`_domain\ *nested*\ ``);``

* 
  ``int geopm_topo_domain_name(``\ :
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``size_t`` _domain_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_domain\ *name*\ ``);``

* 
  ``int geopm_topo_domain_type(``\ :
  `const char *`_domain\ *name*\ ``);``

* 
  ``int geopm_topo_create_cache(``\ :
  ``void);``

DOMAIN TYPES
------------

The ``geopm_domain_e`` enum defines a set of values that correspond to
hardware components on the system:


* 
  ``GEOPM_DOMAIN_INVALID`` = -1:
  Indicates an invalid domain.

* 
  ``GEOPM_DOMAIN_BOARD`` = 0:
  All components on a user allocated compute node. There is only a
  single board per compute node, and every other domain is contained
  with the board domain.

* 
  ``GEOPM_DOMAIN_PACKAGE`` = 1:
  A collection of all the hardware present on a single processor
  package installed on a distinct socket of a motherboard.

* 
  ``GEOPM_DOMAIN_CORE`` = 2:
  Physical core, i.e. a group of associated hyper-threads

* 
  ``GEOPM_DOMAIN_CPU`` = 3:
  Linux logical CPU.  In practice, there is one logical CPU per
  hyperthread visible to the operating system.

* 
  ``GEOPM_DOMAIN_BOARD_MEMORY`` = 4:
  Standard off-package DIMM (DRAM or NAND).

* 
  ``GEOPM_DOMAIN_PACKAGE_MEMORY`` = 5:
  On-package memory (MCDRAM).

* 
  ``GEOPM_DOMAIN_BOARD_NIC`` = 6:
  Peripheral network interface controller not on the processor package.

* 
  ``GEOPM_DOMAIN_PACKAGE_NIC`` = 7:
  Network interface controller on the processor package.

* 
  ``GEOPM_DOMAIN_BOARD_ACCELERATOR`` = 8:
  Peripheral accelerator card not on the processor package.

* 
  ``GEOPM_DOMAIN_PACKAGE_ACCELERATOR`` = 9:
  Accelerator unit on the package (e.g on-package graphics).

* 
  ``GEOPM_NUM_DOMAIN`` = 10:
  The number of valid built-in domains.

DESCRIPTION
-----------

The interfaces described in this man page are the C language bindings
for the `geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ C++ class.  Please refer to the
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ man page for a general description of the purpose,
goals and use case for this interface.


* 
  ``geopm_topo_num_domain``\ ():
   Returns the number of domains available on the system of type
   _domain\ *type*.  If the _domain\ *type* is valid, but there are no
   domains of that type on the system, the return value is zero.  If
   the domain is not a valid domain defined by the ``geopm_domain_e``
   enum then the function will return a negative error code:
   GEOPM_ERROR_INVALID.

* 
  ``geopm_topo_domain_idx``\ ():
  Returns the index of the domain of type _domain\ *type* that is
  local to the Linux logical CPU _cpu\ *idx*.  The return value will
  be greater than or equal to zero and less than the value returned by
  ``geopm_topo_num_domain``\ (_domain\ *type*\ ) for valid input parameters.
  A negative error code is returned if _domain\ *type* or _cpu\ *idx*
  are out of range: GEOPM_ERROR_INVALID.

* 
  ``geopm_topo_num_domain_nested``\ ():
  Returns the number of domains of type _inter\ *domain* that are
  contained within each domain of _outer\ *domain* type.  The return
  value is one if _inner\ *domain* is equal to _outer\ *domain*.  A
  negative error code is returned if _inner\ *domain* is not contained
  within _outer\ *domain*\ : GEOPM_ERROR_INVALID.  Any non-negative
  return value can be used to size the _domain\ *nested* array that is
  passed to ``geopm_topo_domain_nested``\ () with the same values for
  _inner\ *domain* and _outer\ *domain*.

* 
  ``geopm_topo_domain_nested``\ ():
  Fills the output array _domain\ *nested* with the domain indices of
  all of the _inner\ *domain* types nested within the specific
  _outer\ *domain* type indexed by _outer\ *idx*.  The
  _num_domain\ *nested* defines the length of the _domain\ *nested*
  array must match the positive return value from
  ``geopm_topo_num_domain_nested``\ (_inner\ *domain*\ , _outer\ *domain*\ ).
  Zero is returned upon success.  A negative error code is returned
  if _inner\ *domain* is not within _outer\ *domain*\ , or if _outer\ *idx*
  is not between zero and ``geopm_topo_num_domain``\ (_outer\ *domain*\ ).

* 
  ``geopm_topo_domain_name``\ ():
  Sets the _domain\ *name* string to the name associated with the
  _domain\ *type* selected from the ``geopm_domain_e`` enum.  At most
  _result\ *max* bytes are written to the _domain\ *name* string.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be
  sufficient for storing any result.  If _result\ *max* is too small
  to contain the domain name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

* 
  ``geopm_topo_domain_type``\ ():
  Returns the domain type that is associated with the provided
  _domain\ *name* string.  This is the inverse function to
  ``geopm_topo_domain_name``\ () and the input _domain\ *name* must match
  the output from ``geopm_topo_domain_name``\ () for a valid domain
  type.  If the string does not match any of the valid domain names,
  then GEOPM_DOMAIN_INVALID is returned.

* 
  ``geopm_topo_create_cache``\ ():
  Create a cache file for the `geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ object if
  one does not exist.  This cache file will be used by any calls to
  the other ``geopm_topo_*()`` functions documented here as well as
  any use of the GEOPM runtime.  File permissions of the cache file
  are set to "-rw-rw-rw-", i.e. 666. The path for the cache file is
  ``/tmp/geopm-topo-cache``.  If the file exists no operation will be
  performed.  To force the creation of a new cache file,
  `unlink(3) <http://man7.org/linux/man-pages/man3/unlink.3p.html>`_ the existing cache file prior to calling this
  function.

RETURN VALUE
------------

If an error occurs in any call to an interface documented here, the
return value of the function will be a negative integer
corresponding to one of the error codes documented in
`geopm_error(3) <geopm_error.3.html>`_.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_pio_c(3) <geopm_pio_c.3.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_\ ,
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_\ ,
`unlink(3) <http://man7.org/linux/man-pages/man3/unlink.3p.html>`_
