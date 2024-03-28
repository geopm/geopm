/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROXYEPOCHRECORDFILTER_HPP_INCLUDE
#define PROXYEPOCHRECORDFILTER_HPP_INCLUDE

#include "RecordFilter.hpp"

namespace geopm
{
    struct record_s;

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
            ProxyEpochRecordFilter(const std::string &filter_name);
            /// @brief Default destructor.
            virtual ~ProxyEpochRecordFilter() = default;
            /// @brief If input record matches the periodic entry into
            ///        the proxy-region matching the construction
            ///        arguments, then the output will be a vector of
            ///        length one containing the inferred
            ///        M_EVENT_EPOCH_COUNT event.  In all other cases
            ///        the method returns an empty vector.
            ///
            /// @return An empty vector or a vector of length one
            ///         containing a record of an epoch event.
            std::vector<record_s> filter(const record_s &record);
            /// @brief Static function that will parse the filter
            ///        string for the proxy_epoch into the constructor
            ///        arguments for a ProxyEpochRecordFilter.
            ///        Failure to parse will result in a thrown
            ///        Exception with GEOPM_ERROR_INVALID type.
            ///
            /// @param [in] name The filter name which is of the form
            ///        "proxy_epoch,<HASH>[,<CALLS>[,<STARTUP>]]" The
            ///        region hash is always parsed (i.e. required).
            ///        If the calls per epoch is provided or if both
            ///        the call per epoch and startup count are
            ///        provided they are also parsed.  The default
            ///        value for calls_per_epoch is 1 and for
            ///        startup_count is 0.
            ///
            /// @param [out] region_hash The hash of the region that will
            ///        serve as the proxy for the epoch.
            ///
            /// @param [out] calls_per_epoch Number of entries into
            ///        the proxy-region expected in each outer loop of
            ///        the application.
            ///
            /// @param [out] startup_count Number of entries into the
            ///        proxy-region expected prior to the beginning of
            ///        the outer loop of the application.
            static void parse_name(const std::string &name,
                                   uint64_t &region_hash,
                                   int &calls_per_epoch,
                                   int &startup_count);

        private:
            uint64_t m_proxy_hash;
            int m_num_per_epoch;
            int m_count;
    };
}

#endif
