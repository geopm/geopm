geopm_hash(3) -- helper methods for encoding
==============================================

Synopsis
--------

#include `<geopm_hash.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_hash.h>`_

Link with ``-lgeopmd``


.. code-block:: c

       uint64_t geopm_crc32_str(const char *key);

Description
-----------

The `geopm_hash.h <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_hash.h>`_
header defines GEOPM interfaces for encoding region
names into 64-bit integers and working with 64-bit integer values
stored as doubles.


``geopm_crc32_str()``
  Hashes the string *key* to produce a 64-bit value.  This function
  is used to produce unique region IDs for named regions.  An
  ``Agent`` implementation with specialized behavior for specific
  region names can use this function to figure out the region ID to
  expect for the desired region.  Only the bottom 32 bits will be
  filled in, reserving the top 32 bits for hints and other information.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`
