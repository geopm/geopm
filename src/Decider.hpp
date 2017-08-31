/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef DECIDER_HPP_INCLUDE
#define DECIDER_HPP_INCLUDE

#include <string>

#include "geopm_message.h"

namespace geopm
{
    class IRegion;
    class IPolicy;

    /// @brief Abstract base class for power/frequency steering algorithms.
    ///
    /// Defines the interfaces for any power/frequency steering algorithms. These
    /// can be implemented as part of the geopm library or as plugins to be loaded
    /// at runtime. There are two types of deciders. The first is a leaf decider
    /// which controls power/frequency within a single node. The second is a tree
    /// decider ehich controls power/frequency across a set of nodes that are direct
    /// decendants of it in the geopm tree hierarchy.
    class IDecider
    {
        public:
            /// @brief Decider default constructor.
            IDecider() {}
            IDecider(const IDecider &other) {}
            /// @brief Decider destructor, virtual.
            virtual ~IDecider() {}
            /// @brief return a pointer of the derived class, virtual.
            virtual IDecider *clone() const = 0;
            ///@brief Return the upper and lower control bounds.
            /// For a power based control, this will be the upper and lower
            /// power bounds of a single tree node below the current one. For
            /// a frequency based control this will be the p-state bounds of
            /// a single leaf node.
            ///
            /// @param [in] upper_bound The upper control bound.
            ///
            /// @param [in] lower_bound The lower control bound.
            ///
            virtual void bound(double upper_bound, double lower_bound) = 0;
            /// @brief Updates the power split among power control domains when
            /// recieving a new global budget, vitual.
            virtual bool update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy) = 0;
            /// @brief Calculate a new power policy for the region based on telemery data, virtual.
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy) = 0;
            /// @brief Return true if th edescription string matches capabilities of decider, virtual.
            virtual bool decider_supported(const std::string &descripton) = 0;
            /// @brief Return the name of the decider, virtual.
            virtual const std::string& name(void) const = 0;
    };

    class Decider : public IDecider
    {
        public:
            Decider();
            Decider(const Decider &other);
            virtual ~Decider();
            virtual IDecider *clone() const = 0;
            virtual void bound(double upper_bound, double lower_bound);
            virtual bool update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy);
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy) = 0;
            virtual bool decider_supported(const std::string &descripton) = 0;
            virtual const std::string& name(void) const = 0;
        protected:
            /// @brief Save the last known power budget
            double m_last_power_budget;
            /// @brief The upper control bound;
            double m_upper_bound;
            /// @brief The lower control bound;
            double m_lower_bound;
    };

}

#endif
