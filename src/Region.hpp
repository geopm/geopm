/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#ifndef REGION_HPP_INCLUDE
#define REGION_HPP_INCLUDE

#include <stdint.h>
#include <string>

#include "geopm_message.h"
#include "Observation.hpp"
#include "Policy.hpp"

namespace geopm
{
    /// @brief This class encapsulates all state data for a specific
    /// application execution region.
    class Region
    {
        public:
            /// @brief Default constructor.
            /// @param [in] identifier Unique 64 bit region identifier.
            /// @param [in] hint geopm_policy_hint_e describing the compute
            ///             characteristics of this region
            /// @param [in] size Number of control domains.
            Region(uint64_t identifier, int hint, int size);
            /// @brief Default destructor.
            virtual ~Region();
            /// @brief Retrieve the unique region identifier.
            /// @return 64 bit region identifier.
            uint64_t identifier(void) const;
            /// @brief Insert a single observation sample for this region.
            /// @param [in] buffer_index Index of the buffer to insert into.
            /// @param [in] value The sample value to insert.
            void observation_insert(int buffer_index, double value);
            /// @brief Retrieve the compute characteristic hint for this region.
            /// @return geopm_policy_hint_e describing the compute characteristics
            /// of this region.
            int hint(void) const;
            /// @brief Set the power policy for this region.
            /// @param [in] policy Policy object for this region.
            void policy(Policy* policy);
            /// @brief Retrieve the power policy for this region.
            /// @return Policy object for this region.
            Policy* policy(void);
            /// @brief Retrieve The last policy message sent down for this region.
            /// Used to compare with new incoming message to see if the policy
            /// has changed.
            /// @return Saved policy message from last time one was sent.
            struct geopm_policy_message_s* last_policy();
            /// @brief Split this regions power policy budget and create new
            /// policy messages for each child control domain.
            /// @return Vector of policy messages for each child
            /// control domain.
            std::vector <struct geopm_policy_message_s>* split_policy(void);
            /// @brief Retrieve a set of sample message for each child
            /// control domain.
            /// @return Vector of sample messages from each child
            /// control domain.
            std::vector <struct geopm_sample_message_s>* child_sample(void);
            /// @brief Set our saved copy of the last updated policy message
            /// for this region.
            /// @param [in] policy Policy message to save as last update.
            void last_policy(const struct geopm_policy_message_s &policy);
            /// @brief Retrieve the mean of all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_mean(int buffer_index) const;
            /// @brief Retrieve the mean of all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_median(int buffer_index) const;
            /// @brief Retrieve the standard deviation of all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_stddev(int buffer_index) const;
            /// @brief Retrieve the maximum value of all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_max(int buffer_index) const;
            /// @brief Retrieve the minimum value of all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_min(int buffer_index) const;
            /// @brief Retrieve the integrated time over all values in the
            /// requested buffer.
            /// @param [in] buffer_index Index of the requested buffer.
            double observation_integrate_time(int buffer_index) const;
        protected:
            /// @brief Hold the Observation object which contains samples
            /// for this region.
            Observation m_obs;
            /// @brief Hold the current power policy for this region.
            Policy m_policy;
            /// @brief Hold a policy message for each child control domain.
            std::vector <struct geopm_policy_message_s> m_split_policy;
            /// @brief Hold a sample message from each child control domain.
            std::vector <struct geopm_sample_message_s> m_child_sample;
            /// @brief Hold Saved policy message from last time one was sent.
            struct geopm_policy_message_s m_last_policy;
            /// @brief Hold unique 64 bit region identifier.
            uint64_t m_identifier;
            /// @brief Hold the compute characteristic hint for this region.
            int m_hint;
    };
}

#endif
