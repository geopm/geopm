/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTROLRECOMMENDER_HPP_INCLUDE
#define CONTROLRECOMMENDER_HPP_INCLUDE

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;

    class ControlRecommender
    {
        public:
            /// @brief Construct object, query the system for signals at the domains
            /// required for the algorithm
            ControlRecommender() = default;
            virtual ~ControlRecommender() = default;

            /// @brief Provides calling classes a way to inform the recommender what controls it
            ///        would like to set
            /// @param [in] control_name Map of desired controls and domain of interest.
            /// @param [in] domain GEOPM Domain of desired controls and domain of interest.
            /// @returns Recommender control index.  -1 indicates control not supported
            virtual int control_supported(std::string control_name, int domain) = 0;

            /// @brief Queries all signals of interest and updates the recommendations for controls
            virtual void update_recommendation() = 0;

            /// @brief Provides recommendations for control index provided
            /// @param [in] control_idx Control index provided by controls_supported
            /// @returns recommended values for specified control at the domain of interest
            virtual std::vector<double> sample_recommendation(int control_idx) const = 0;

            /// @brief Validates the policy provided to the performance model.
            ///        Policy sender can request default value with 'NaN'.
            virtual void validate_policy(std::vector<double> &in_policy) const = 0;

            /// @brief Applies the policy provided to the performance model.
            virtual void apply_policy(std::vector<double> &in_policy) = 0;
    };
}

#endif
