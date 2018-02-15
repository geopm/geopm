/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "StaticPolicyDecider.hpp"

#include "config.h"

#define GEOPM_STATIC_POLICY_DECIDER_PLUGIN_NAME "static_policy"

namespace geopm
{
    StaticPolicyDecider::StaticPolicyDecider()
        : m_name(GEOPM_STATIC_POLICY_DECIDER_PLUGIN_NAME)
    {

    }

    StaticPolicyDecider::~StaticPolicyDecider()
    {

    }

    bool StaticPolicyDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& StaticPolicyDecider::name(void) const
    {
        return m_name;
    }

    std::string StaticPolicyDecider::plugin_name(void)
    {
        return GEOPM_STATIC_POLICY_DECIDER_PLUGIN_NAME;
    }

    std::unique_ptr<IDecider> StaticPolicyDecider::make_plugin(void)
    {
        return std::unique_ptr<IDecider>(new StaticPolicyDecider);
    }

    bool StaticPolicyDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        return false;
    }
}
