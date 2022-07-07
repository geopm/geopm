/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SAMPLEAGGREGATOR_HPP_INCLUDE
#define SAMPLEAGGREGATOR_HPP_INCLUDE

#include <cstdint>

#include <string>
#include <set>
#include <memory>

namespace geopm
{
    class SampleAggregator
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<SampleAggregator> make_unique(void);
            /// @brief Default destructor of pure virtual base.
            virtual ~SampleAggregator() = default;
            /// @brief Push a signal to be accumulated per-region.
            ///
            /// Check the signal behavior and call push_signal_total()
            /// or push_signal_average() accordingly.
            ///
            /// @param [in] signal_name Name of the signal to sample
            ///         and aggregate.
            ///
            /// @param [in] domain_type Domain type over which the
            ///        region hash and signal should be sampled.
            ///
            /// @param [in] domain_idx Domain over which the region hash
            ///        and signal should be sampled.
            ///
            /// @return Index of signal to be used with sample().
            ///         This index matches the return value of
            ///         PlatformIO::push_signal() for the same signal.
            virtual int push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Push a signal to be accumulated per-region as a
            ///        total.
            ///
            /// The signal name must be a valid signal available
            /// through PlatformIO.  Note that unlike other signals
            /// this is a total accumulated per region by subtracting
            /// the value of the signal at the region exit from the
            /// region entry.  Region entry and exit are not exact and
            /// are determined by the value of the REGION_HASH signal
            /// at the time of read_batch().  This aggregation should
            /// only be used for signals that are monotonically
            /// increasing, such as time.
            ///
            /// @param [in] signal_name Name of the signal to sample
            ///         and aggregate.
            ///
            /// @param [in] domain_type Domain type over which the
            ///        region hash and signal should be sampled.
            ///
            /// @param [in] domain_idx Domain over which the region hash
            ///        and signal should be sampled.
            ///
            /// @return Index of signal to be used with sample().
            ///         This index matches the return value of
            ///         PlatformIO::push_signal() for the same signal.
            virtual int push_signal_total(const std::string &signal_name,
                                          int domain_type,
                                          int domain_idx) = 0;
            /// @brief Push a signal to be accumulated per-region as
            ///        an average.
            ///
            /// The signal name must be a valid signal available
            /// through PlatformIO.  Note that unlike other signals
            /// this is an average value accumulated per region by a
            /// time weighted mean of the values sampled while in the
            /// region. Region entry and exit are not exact and are
            /// determined by the value of the REGION_HASH signal at
            /// the time of read_batch().  This aggregation should be
            /// used for signals that vary up and down over time such
            /// as the CPU frequency.
            ///
            /// @param [in] signal_name Name of the signal to sample
            ///        and aggregate.
            ///
            /// @param [in] domain_type Domain type over which the region
            ///        hash and signal should be sampled.
            ///
            /// @param [in] domain_idx Domain over which the region
            ///        hash and signal should be sampled.
            ///
            /// @return Index of signal to be used with sample().
            ///         This index matches the return value of
            ///         PlatformIO::push_signal() for the same signal.
            virtual int push_signal_average(const std::string &signal_name,
                                            int domain_type,
                                            int domain_idx) = 0;
            /// @brief Update stored totals for each signal.
            ///
            /// This method is to be called after each call to
            /// PlatformIO::read_batch().  This should be called with
            /// every PlatformIO update because sample_total() maybe
            /// not be called until the end of execution.
            virtual void update(void) = 0;
            /// @brief Get the aggregated value of a signal.
            ///
            /// The aggregation type is determined by which method was
            /// used to push the signal: push_signal_total() or
            /// push_signal_average().  The value returned is
            /// aggregated over all samples since the application
            /// start.
            ///
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total() or
            ///        push_signal_average().
            ///
            /// @return Aggregated value for the signal regardless of
            ///         region or epoch.
            virtual double sample_application(int signal_idx) = 0;
            /// @brief Get the aggregated value of a signal since the
            ///        first epoch.
            ///
            /// The aggregation type is determined by which method was
            /// used to push the signal: push_signal_total() or
            /// push_signal_average().  The value returned is
            /// aggregated over all samples since the first epoch
            /// observed over the domain specified when the signal was
            /// pushed.
            ///
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total() or
            ///        push_signal_average().
            ///
            /// @return Aggregated value for the signal since first
            ///         epoch, or NAN if called before first call to
            ///         update().
            virtual double sample_epoch(int signal_idx) = 0;
            /// @brief Get the aggregated value of a signal during the
            ///        execution of a particular region.
            ///
            /// The aggregation type is determined by which method was
            /// used to push the signal: push_signal_total() or
            /// push_signal_average().  The value returned is
            /// aggregated over all samples where the REGION_HASH
            /// signal matched the value specified for the domain
            /// pushed.  The returned value is zero for
            /// push_signal_total() aggregation, and NAN for
            /// push_signal_average() aggregation if the region was not
            /// observed for any samples.
            ///
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total() or
            ///        push_signal_average().
            ///
            /// @param [in] region_hash The region hash to look up
            ///        data for.
            ///
            /// @return Aggregated value for the signal during the
            ///         region, or NAN if called before first call to
            ///         update().
            virtual double sample_region(int signal_idx, uint64_t region_hash) = 0;
            /// @brief Get the aggregated value of a signal over the
            ///        last completed epoch interval.
            ///
            /// The aggregation type is determined by which method was
            /// used to push the signal: push_signal_total() or
            /// push_signal_average().  The value returned is
            /// aggregated over all samples between the last two
            /// samples when the epoch count changed.
            ///
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total() or
            ///        push_signal_average().
            ///
            /// @return Aggregated value for the signal over last
            ///         epoch, or NAN if called before first call to
            ///         update().
            virtual double sample_epoch_last(int signal_idx) = 0;
            /// @brief Get the aggregated value of a signal during the
            ///        the last completed execution of a particular
            ///        region.
            ///
            /// The aggregation type is determined by which method was
            /// used to push the signal: push_signal_total() or
            /// push_signal_average().  The value returned is
            /// aggregated over the last contiguous set of samples
            /// where the REGION_HASH signal matched the value
            /// specified for the domain pushed.  Note that if the
            /// region is currently executing, the value reported is
            /// aggregated over the last region interval, not the
            /// currently executing interval. The returned value is
            /// zero for push_signal_total() aggregation, and NAN for
            /// push_signal_average() aggregation if a completed region
            /// with the specified hash has not been observed.
            ///
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total() or
            ///        push_signal_average().
            ///
            /// @param [in] region_hash The region hash to look up
            ///        data for.
            ///
            /// @return Aggregated value for the signal during the
            ///         last execution of the region, or NAN if called
            ///         before first call to update() or if
            ///         period_duration() was not called.
            virtual double sample_region_last(int signal_idx, uint64_t region_hash) = 0;
            /// @brief Set the time period for sample_period_last()
            ///
            /// Calling this method prior to the first call to
            /// update() enables signals to be accumulated on a
            /// periodic basis.  The sample_period_last() method is
            /// used to sample an accumulated value over the last
            /// completed time interval, and the period of the
            /// interval is configured by calling this method.
            ///
            /// The sample_period_last() method will always return NAN
            /// If period_duration() is not called prior to the first
            /// call to update().  The period_duration() method will
            /// throw an Exception if it is called after the first
            /// call to update().
            ///
            /// @param [in] duration Time interval in seconds over
            ///        which the sample_region_last() method is
            ///        aggregated (must be greater than 0.0).
            virtual void period_duration(double duration) = 0;
            /// @brief Get the index of the current time period.
            ///
            /// Provides an index of completed durations.  Will return
            /// zero if periodic sampling is not enabled (when
            /// period_duration() was not called prior to update()).
            /// When periodic sampling is enabled, the
            /// sample_period_last() method will return 0.0 until a
            /// full period has elapsed, this corresponds to when
            /// get_period() returns a value greater than zero.
            ///
            /// @returns The number of completed durations since the
            ///          application start.
            virtual int get_period(void) = 0;
            /// @brief Get the aggregated value of a signal during the
            ///        last completed time interval.
            virtual double sample_period_last(int signal_idx) = 0;
        protected:
            SampleAggregator() = default;
    };
}

#endif
