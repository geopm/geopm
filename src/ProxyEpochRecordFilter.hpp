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

#ifndef PROXYEPOCHRECORDFILTER_HPP_INCLUDE
#define PROXYEPOCHRECORDFILTER_HPP_INCLUDE

#include "RecordFilter.hpp"

namespace geopm
{
    /// @brief Filter that can be used to synthesize epoch events from
    ///        a sequence of region entry events.  The filter
    ///        suppresses received epoch events and passes through all
    ///        other events.
    ///
    /// This filter is used to insert synthetic epoch events into the
    /// stream received by an application process.  This provides
    /// users of the ApplicationSampler with epoch events even if the
    /// application does not provide them directly through calls to
    /// geopm_prof_epoch().  When this filter is selected, any epoch
    /// events that arrive though the application calls into
    /// geopm_prof_epoch() are removed from the record stream.  The
    /// output of this filter is a pass through of all non-epoch
    /// events and may include synthesized epoch events.  The epoch
    /// events are synthesized from region entry of a specified region
    /// that may be detected through runtimes like MPI function calls
    /// or OpenMP parallel regions.  This proxy-region is specified at
    /// filter construction time by the region hash.  Typically, this
    /// region hash value is determined by inspection of a report from
    /// a previous run.  There are two other constructor parameters
    /// that enable support for multiple proxy-region entries per
    /// outer loop, and for application calls into the proxy-region
    /// prior to the beginning of the outer loop.  The filter assumes
    /// that the provided records have been collected from a single
    /// process.
    class ProxyEpochRecordFilter : public RecordFilter
    {
        public:
            /// @brief Constructor for a process specific proxy-region
            ///        EpochIOGroup record filter.
            ///
            /// @param [in] region_hash The hash for the region that
            ///        will be used as a proxy for the epoch events.
            ///
            /// @param [in] calls_per_epoch Number of calls to the
            ///        proxy-region that are expected in each outer
            ///        loop of the application per process.
            ///
            /// @param [in] startup_count Number of calls to the proxy-
            ///        region that are to be ignored at application
            ///        startup.  These calls are expected prior to
            ///        entering the outer loop of the application.
            ProxyEpochRecordFilter(uint64_t region_hash,
                                   int calls_per_epoch,
                                   int startup_count);
            /// @brief Default destructor.
            virtual ~ProxyEpochRecordFilter() = default;
            /// @brief If input record matches the periodic entry into
            ///        the proxy-region matching the construction
            ///        arguments, then the output will be a vector of
            ///        length one containing the inferred
            ///        M_EVENT_EPOCH_COUNT event.  If the input record
            ///        is of type M_EVENT_HINT, then returned vector
            ///        will be a vector of length one containing this
            ///        record.  In all other cases the method returns
            ///        an empty vector.
            ///
            /// @return An empty vector or a vector of length one
            ///         containing a record of an epoch or hint event.
            std::vector<ApplicationSampler::m_record_s> filter(const ApplicationSampler::m_record_s &record);
        private:
            uint64_t m_proxy_hash;
            int m_num_per_epoch;
            int m_count;
    };
}

#endif
