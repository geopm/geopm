/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTROLRECOMMENDER_HPP_INCLUDE
#define CONTROLRECOMMENDER_HPP_INCLUDE

#include <memory>
#include <vector>
#include <string>

struct geopm_request_s; // Defined in "geopm/PlatformIO.hpp"

namespace geopm
{
    /// @brief Abstract class that recommends control settings
    class ControlRecommender
    {
        public:
            ControlRecommender() = default;
            virtual ~ControlRecommender() = default;
            /// @brief Factory constructor
            /// @param [in] algorithm Name of algorithm to select
            /// @returns Unique pointer to a ControlRecommender objecct
            static std::unique_ptr<ControlRecommender> make_unique(const std::string &algorithm);
            /// @brief Set the performance bias
            /// @param [in] bias Abstract number between 0 and 1: a
            ///         value of 0 is most biased toward performance,
            ///         and 1 is most biased towards energy efficiency.
            virtual void performance_bias(double bias) = 0;
            /// @brief Submit a set of requests for recommendations
            ///
            /// Called once to configure the object and discover
            /// feature support.
            ///
            /// @param [in] Vector of control requests submitted for
            ///        recommendation.
            /// @returns The subset of the attempted_requests that are
            ///          supported.  The order of these requests
            ///          reflects ordering of the returned values from
            ///          the settings method.
            virtual std::vector<geopm_request_s> supported_requests(const std::vector<geopm_request_t> &attempted_requests);
            /// @brief Update recommended settings, method is called
            ///        once prior calling the settings() method one or
            ///        more times.
            virtual void update(void) = 0;
            /// @brief Get latest recommendation based determined on
            ///        last call to update.
            /// @returns Vector of settings for the requests returned
            ///          by supported_requests()
            virtual std::vector<double> settings(void) const = 0;
    };
}

#endif
