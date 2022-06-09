.. role:: raw-html-m2r(raw)
   :format: html


geopm::SampleAggregator(3) -- per-region aggregated signal data
===============================================================






Synopsis
--------

#include `<geopm/SampleAggregator.hpp> <https://github.com/geopm/geopm/blob/dev/src/SampleAggregator.hpp>`_

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``void init(``\ :
  ``void);``

* 
  ``int push_signal_total(``\ :
  `const std::string &`_signal\ *idx*\ ``,``\ :raw-html-m2r:`<br>`
  `int `_domain\ *type*\ ``,``\ :raw-html-m2r:`<br>`
  `int `_domain\ *idx*\ ``);``

* 
  ``double sample_total(``\ :
  `int `_signal\ *idx*\ ``,``\ :raw-html-m2r:`<br>`
  `uint64_t `_region\ *hash*\ ``);``

* 
  ``void read_batch(``\ :
  ``void);``

* 
  ``std::set<uint64_t> tracked_region_hash(``\ :
  ``void) const;``

Description
-----------

The RegionAggregator is used to store running totals of various
signals per region.  Regions are automatically detected through
sampling the REGION_HASH signal.  The object also accumulates data for
the epoch.  The set of signals to be tracked is determined by pushing
signals similar to the ``push_signal()`` method of
:doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`.

Class Methods
-------------


* 
  ``init``\ ():
  Push required PlatformIO signals (EPOCH_COUNT).

* 
  ``push_signal_total``\ ():
  Push a signal to be accumulated per-region.  It must be a valid
  signal available through PlatformIO.  The signal to sample and
  aggregate is _signal\ *name* and it will be collected for the domain
  _domain\ *type* at _domain\ *idx*.  The return value is an index to be
  used with sample() to refer to this signal.  This index matches
  the index returned by :doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`\ ::push_signal() for
  the same signal name and domain.  Note that unlike other signals
  this is a total accumulated per region by subtracting the value of
  the signal at the region exit from the region entry.  Region entry
  and exit are not exact and are determined by the value of the
  REGION_HASH signal at the time of read_batch() for the same domain
  type and index as the signal of interest.  This aggregation should
  only be used for signals that are monotonically increasing, such
  as time.

* 
  ``sample_total``\ ():
  Returns the total accumulated value of a signal for one
  region. The signal must have been pushed to accumulate as
  per-region values.  The index returned from push_signal_total()
  should be passed to _signal\ *idx*.  The region of interest is
  passed in _region\ *hash*.  Note that unlike other signals this is a
  total accumulated per region by subtracting the value of the
  signal at the region exit from the region entry.  Region entry and
  exit are not exact and are determined by the value of the
  REGION_HASH signal at the time of read_batch().

* 
  ``read_batch``\ ():
  Updates stored totals for each signal after
  PlatformIO::read_batch() has been called.  This should be called
  with every PlatformIO update because sample_total() maybe not be
  called until the end of execution.  Agents that include an
  instance of the RegionAggregator can include this call in their
  implementation of sample_platform().

* 
  ``tracked_region_hash``\ ():
  Returns the set of region IDs tracked by this object.  Note that
  very short-running regions may not be detected through sampling
  the REGION_HASH signal.

See Also
--------

:doc:`geopm(7) <geopm.7>`
