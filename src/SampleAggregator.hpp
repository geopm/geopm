/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
            ///         epoch.
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
            /// push_signal_average aggregation if the region was not
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
            ///         region.
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
            ///         epoch.
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
            /// push_signal_average aggregation if a completed region
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
            ///         last execution of the region.
            virtual double sample_region_last(int signal_idx, uint64_t region_hash) = 0;
            /// @brief Set the time period for sample_period_last()
            ///
            /// The duration must be greater than zero and if this
            /// method is not called, sample_period_last() will
            /// return NAN.
            virtual void period_duration(double duration) = 0;
            /// @brief Get the index of the current time period.
            ///
            /// @returns The number of completed durations since
            /// the application start.
            virtual int get_period(void) = 0;
            /// @brief Get the aggregated value of a signal during the
            ///        last completed time interval.
            virtual double sample_period_last(int signal_idx) = 0;
        protected:
            SampleAggregator() = default;
    };
}

#endif
