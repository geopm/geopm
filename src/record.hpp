/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RECORD_HPP_INCLUDE
#define RECORD_HPP_INCLUDE

#include <cstdint>
#include <string>
#include "geopm_time.h"

namespace geopm
{
    /// @brief Enumeration of event types that can be stored
    ///        in a record.
    ///
    /// This enum is stored in the "event" field of an
    /// m_record_s.  For each event type the "signal" field of
    /// a record will represent different data.  The
    /// description of the "signal" field in the m_record_s is
    /// given for each enum value.
    enum event_e {
        EVENT_REGION_ENTRY = 0,  /// EVENT: The application has entered a region.
                                 /// SIGNAL: The hash of the entered region.
        EVENT_REGION_EXIT = 1,   /// EVENT: The application has exited a region.
                                 /// SIGNAL: The hash of the exited region.
        EVENT_EPOCH_COUNT = 2,   /// EVENT: An epoch call was made by the application.
                                 /// SIGNAL: The number of epochs signaled by process.
        EVENT_SHORT_REGION = 3,  /// EVENT: The application entered and exited a
                                 ///        region at least once since last update.
                                 /// SIGNAL: Handle to pass to AppliationSampler::get_short_region()
        EVENT_AFFINITY = 9,      /// EVENT: The application process affinity has changed
                                 /// SIGNAL: PID or TID of thread with the modified cpuset
        EVENT_START_PROFILE = 10,/// EVENT: An application PID requested profiling
                                 /// SIGNAL: Hash of the profile name
        EVENT_STOP_PROFILE = 11, /// EVENT: An application PID requested profiling
                                 /// SIGNAL: Hash of the profile name
        EVENT_OVERHEAD = 12,     /// EVENT: Report overhead time due to GEOPM startup
                                 /// SIGNAL: Time in seconds stored in double precision format
    };

    /// @brief Format an event_e type as a string.
    std::string event_name(int event_type);
    /// @brief Convert a human-readable event type string to
    ///        an event_e
    int event_type(const std::string &event_name);
    /// @brief Format a string to represent a hint enum from the
    ///        geopm_region_hint_e.
    /// @param [in] hint One of the hint enum values.
    /// @return A shortened string representation of the hint enum:
    ///         e.g. GEOPM_REGION_HINT_MEMORY => "MEMORY".
    std::string hint_name(uint64_t hint);
    /// @brief Parse a string representing the hint name.
    /// @param [in] hint_name A string representing the hint as would
    ///        be returned by hint_type_to_name().
    /// @return One of the geopm_region_hint_e enum values.
    uint64_t hint_type(const std::string &hint_name);

    /// @brief Record of an application event.
    struct record_s {
        /// @brief System time when event was recorded.
        geopm_time_s time;
        /// @brief The process identifier where event occurred.
        int process;
        /// @brief One of the m_event_e event types.
        int event;
        /// @brief The signal associated with the event type.
        uint64_t signal;
    };

    struct short_region_s {
        uint64_t hash;
        int num_complete;
        double total_time;
    };
}

#endif
