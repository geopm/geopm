.. role:: raw-html-m2r(raw)
   :format: html


geopm_hash.h(3) -- helper methods for encoding
==============================================






SYNOPSIS
--------

#include `<geopm_hash.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_hash.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``uint64_t geopm_crc32_u64(``\ :
  ``uint64_t`` *begin*\ , :raw-html-m2r:`<br>`
  ``uint64_t`` *key*\ ``);``

* 
  ``uint64_t geopm_crc32_str(``\ :
  ``uint64_t`` *key*\ ``);``

* 
  ``static inline uint64_t geopm_signal_to_field(``\ :
  ``double`` *signal*\ ``);``

* 
  ``static inline double geopm_field_to_signal(``\ :
  ``uint64_t`` *field*\ ``);``

DESCRIPTION
-----------

The _geopm\ *hash.h* header defines GEOPM interfaces for encoding region
names into 64-bit integers and working with 64-bit integer values
stored as doubles.


* 
  ``geopm_crc32_u64``\ ():
  Implements the CRC32 hashing algorithm, which starts with
  the value *begin* and hashes the value *key* to produce a 32-bit
  result.  The result is returned as a 64-bit integer.

* 
  ``geopm_crc32_str``\ ():
  Hashes the string *key* to produce a 64-bit value.  This function
  is used to produce unique region IDs for named regions.  An
  ``Agent`` implementation with specialized behavior for specific
  region names can use this function to figure out the region ID to
  expect for the desired region.  As this uses the CRC32 algorithm,
  only the bottom 32 bits will be filled in, reserving the top 32
  bits for hints and other information.

* 
  ``geopm_signal_to_field``\ ():
  Convert a double *signal* into a 64-bit field.  This function is
  especially useful for converting region IDs read as signals from
  `geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_ but may also be used to work with raw MSR
  values.

* 
  ``geopm_field_to_signal``\ ():
  Convert a 64-bit *field* into a double representation appropriate
  for a signal returned by an IOGroup.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_
