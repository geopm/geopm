
geopm::SampleAggregator(3) -- per-region aggregated signal data
===============================================================






Synopsis
--------

#include `<geopm/SampleAggregator.hpp> <https://github.com/geopm/geopm/blob/dev/src/SampleAggregator.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       static unique_ptr<SampleAggregator> SampleAggregator::make_unique(void);

       int SampleAggregator::push_signal(const string &signal_name,
                                         int domain_type,
                                         int domain_idx);

       int SampleAggregator::push_signal_total(const string &signal_name,
                                               int domain_type,
                                               int domain_idx);

       int SampleAggregator::push_signal_average(const string &signal_name,
                                                 int domain_type,
                                                 int domain_idx);

       void SampleAggregator::update(void);

       double SampleAggregator::sample_application(int signal_idx);

       double SampleAggregator::sample_epoch(int signal_idx);

       double SampleAggregator::sample_region(int signal_idx, uint64_t region_hash);

       double SampleAggregator::sample_epoch_last(int signal_idx);

       double SampleAggregator::sample_region_last(int signal_idx, uint64_t region_hash);

       void SampleAggregator::period_duration(double duration);

       int SampleAggregator::get_period(void);

       double SampleAggregator::sample_period_last(int signal_idx);

Description
-----------

The ``RegionAggregator`` is used to store running totals of various
signals per region.  Regions are automatically detected through
sampling the ``REGION_HASH`` signal.  The object also accumulates data for
the epoch.  The set of signals to be tracked is determined by pushing
signals similar to the ``push_signal()`` method of
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`.

Class Methods
-------------


``make_unique()``
  Returns a ``unique_ptr`` to a concrete object
  constructed using the underlying implementation

``push_signal()``
  Push a signal to be accumulated per-region.
  Check the signal behavior and call ``push_signal_total()``
  or ``push_signal_average()`` accordingly.
  The signal to sample and aggregate is *signal_name* and
  it will be collected for the domain *domain_type* at *domain_idx*
  over which the region hash and signal should be sampled.
  The return value is an index to be
  used with ``sample()`` to refer to this signal.
  This index matches the return value of
  :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`\ ``::push_signal()`` for the same signal.

``push_signal_total()``
  Push a signal to be accumulated per-region as a total.
  The *signal_name* must be a valid signal available
  through ``PlatformIO``.  Note that unlike other signals
  this is a total accumulated per region by subtracting
  the value of the signal at the region exit from the
  region entry.  Region entry and exit are not exact and
  are determined by the value of the ``REGION_HASH`` signal
  at the time of ``read_batch()``.  This aggregation should
  only be used for signals that are monotonically
  increasing, such as time.
  The signal to sample and aggregate is *signal_name* and
  it will be collected for the domain *domain_type* at *domain_idx*
  over which the region hash and signal should be sampled.
  The return value is an index to be
  used with ``sample()`` to refer to this signal.
  This index matches the return value of
  :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`\ ``::push_signal()`` for the same signal.

``push_signal_average()``
  Push a signal to be accumulated per-region as an average.
  The *signal_name* must be a valid signal available
  through ``PlatformIO``.  Note that unlike other signals
  this is an average value accumulated per region by a
  time weighted mean of the values sampled while in the
  region. Region entry and exit are not exact and are
  determined by the value of the ``REGION_HASH`` signal at
  the time of ``read_batch()``.  This aggregation should be
  used for signals that vary up and down over time such
  as the CPU frequency.
  The signal to sample and aggregate is *signal_name* and
  it will be collected for the domain *domain_type* at *domain_idx*
  over which the region hash and signal should be sampled.
  The return value is an index to be
  used with ``sample()`` to refer to this signal.
  This index matches the return value of
  :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`\ ``::push_signal()`` for the same signal.

``update()``
  Update stored totals for each signal.
  This method is to be called after each call to
  ``PlatformIO::read_batch()``.  This should be called with
  every ``PlatformIO`` update because ``sample_total()`` maybe
  not be called until the end of execution.

``sample_application()``
  Get the aggregated value of a signal.
  The aggregation type is determined by which method was
  used to push the signal: ``push_signal_total()`` or
  ``push_signal_average()``. The *signal_idx* parameter is the
  Index returned by a previous call to ``push_signal_total()`` or
  ``push_signal_average()``.  The value returned is
  aggregated over all samples since the application
  start, regardless of region or epoch.

``sample_epoch()``
  Get the aggregated value of a signal since the first epoch.
  The aggregation type is determined by which method was
  used to push the signal: ``push_signal_total()`` or
  ``push_signal_average()``. The *signal_idx* parameter is the
  Index returned by a previous call to ``push_signal_total()`` or
  ``push_signal_average()``.  The value returned is
  aggregated over all samples since the first epoch
  observed over the domain specified when the signal was
  pushed, or ``NAN`` if called before first call to ``update()``.

``sample_region()``
  Get the aggregated value of a signal during the
  execution of a particular region.
  The aggregation type is determined by which method was
  used to push the signal: ``push_signal_total()`` or
  ``push_signal_average()``. The *signal_idx* parameter is the
  Index returned by a previous call to ``push_signal_total()`` or
  ``push_signal_average()``.  The value returned is
  aggregated over all samples where the *region_hash*
  signal matched the value specified for the domain
  pushed.  The returned value is zero for
  ``push_signal_total()`` aggregation, and ``NAN`` for
  ``push_signal_average()`` aggregation if the region was not
  observed for any samples,
  or ``NAN`` if called before first call to ``update()``.

``sample_epoch_last()``
  Get the aggregated value of a signal over the
  last completed epoch interval.
  The aggregation type is determined by which method was
  used to push the signal: ``push_signal_total()`` or
  ``push_signal_average()``. The *signal_idx* parameter is the
  Index returned by a previous call to ``push_signal_total()`` or
  ``push_signal_average()``.  The value returned is
  aggregated over all samples between the last two
  samples when the epoch count changed,
  or ``NAN`` if called before first call to ``update()``.

``sample_region_last()``
  Get the aggregated value of a signal during the
  the last completed execution of a particular region.
  The aggregation type is determined by which method was
  used to push the signal: ``push_signal_total()`` or
  ``push_signal_average()``. The *signal_idx* parameter is the
  Index returned by a previous call to ``push_signal_total()`` or
  ``push_signal_average()``.  The value returned is
  aggregated over the last contiguous set of samples
  where the *region_hash* signal matched the value
  specified for the domain pushed.  Note that if the
  region is currently executing, the value reported is
  aggregated over the last region interval, not the
  currently executing interval. The returned value is
  zero for ``push_signal_total()`` aggregation, and ``NAN`` for
  ``push_signal_average()`` aggregation if a completed region
  with the specified hash has not been observed, or ``NAN`` if called
  before first call to ``update()`` or if
  ``period_duration()`` was not called.

``period_duration()``
  Set the time period for ``sample_period_last()``
  Calling this method prior to the first call to
  ``update()`` enables signals to be accumulated on a
  periodic basis.  The ``sample_period_last()`` method is
  used to sample an accumulated value over the last
  completed time interval, and the period of the
  interval is configured by calling this method.
  The *duration* is the time interval in seconds over
  which the ``sample_region_last()`` method is
  aggregated (must be greater than ``0.0``).

``get_period()``
  Get the index of the current time period.
  Provides an index of completed durations.
  Returns the number of completed durations since the
  application start. Will return
  zero if periodic sampling is not enabled (when
  ``period_duration()`` was not called prior to ``update()``).
  When periodic sampling is enabled, the
  ``sample_period_last()`` method will return ``0.0`` until a
  full period has elapsed, this corresponds to when
  ``get_period()`` returns a value greater than zero.

``sample_period_last()``
  Get the aggregated value of a signal during the
  last completed time interval.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`
