/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RECORDFILTER_HPP_INCLUDE
#define RECORDFILTER_HPP_INCLUDE

#include <vector>
#include <memory>

namespace geopm
{
    struct record_s;
    /// @brief Base class for filters that can be applied to
    ///        ApplicationSampler record streams produced by a single
    ///        process.
    class RecordFilter
    {
        public:
            static std::unique_ptr<RecordFilter> make_unique(const std::string &name);
            /// @brief Default constructor for pure virtual interface.
            RecordFilter() = default;
            /// @brief Default destructor for pure virtual interface.
            virtual ~RecordFilter() = default;
            /// @brief Apply a filter to a stream of records.
            ///
            /// This method is called repeatedly by a user to update a
            /// filtered time stream with a new record.  The input
            /// record is used to update the state of the filter and
            /// the method returns a vector containing any filtered
            /// values resulting from the update.
            ///
            /// @param [in] record The update value to be filtered.
            ///
            /// @return Vector of zero or more records to update the
            ///         filtered stream.
            virtual std::vector<record_s> filter(const record_s &record) = 0;
    };
}

#endif
