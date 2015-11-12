/*
 * Copyright (c) 2015, Intel Corporation
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

#include <vector>

#include "Platform.hpp"
#include "Policy.hpp"

namespace geopm
{
    class Decider
    {
        public:
            Decider();
            virtual ~Decider();
            virtual void update_policy(const struct geopm_policy_message_s &policy_msg, Region* curr_region);
            virtual bool is_converged(void);
            virtual void get_policy(Platform const *platform, Policy &policy) = 0;
            virtual bool decider_supported(const std::string &descripton) = 0;
            virtual const std::string& name(void) const = 0;

        protected:
            int m_is_converged;
            std::map <long, geopm_policy_message_s> m_region_policy_msg_map;
    };

    class LeafDecider : public Decider
    {
        public:
            LeafDecider();
            virtual ~LeafDecider();
            virtual bool decider_supported(const std::string &descripton) = 0;
            virtual const std::string& name(void) const = 0;
    };

    class TreeDecider : public Decider
    {
        public:
            TreeDecider();
            virtual ~TreeDecider();
            virtual void get_policy(Platform const *platform, Policy &policy);
            virtual void split_policy(const struct geopm_policy_message_s &policy, Region* curr_region);
            virtual bool decider_supported(const std::string &descripton) = 0;
            virtual const std::string& name(void) const = 0;
    };
}

#endif
