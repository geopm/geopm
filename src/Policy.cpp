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

#include <float.h>

#include "geopm_error.h"
#include "Exception.hpp"
#include "Policy.hpp"

const double GEOPM_POLICY_CONST_INVALID_TARGET = DBL_MIN;

namespace geopm
{
    Policy::Policy() {}

    Policy::Policy(int num_domain)
        :   m_num_domain(num_domain)
    {
        insert_region(0); //Add the default unmarked region
    }

    Policy::~Policy() {}

    void Policy::insert_region(uint64_t region_id)
    {
        std::vector<double> unknown_region_target(m_num_domain);
        std::vector<bool> unknown_region_updated(m_num_domain);
        std::fill(unknown_region_target.begin(), unknown_region_target.end(), GEOPM_POLICY_CONST_INVALID_TARGET);
        std::fill(unknown_region_updated.begin(), unknown_region_updated.end(), true);
        m_target.insert(std::pair<uint64_t, std::vector<double> >(region_id, unknown_region_target));
        m_updated.insert(std::pair<uint64_t, std::vector<bool> >(region_id, unknown_region_updated));
        m_is_converged.insert(std::pair<uint64_t, bool>(region_id, false));

    }

    void Policy::update(uint64_t region_id, int domain, double target)
    {
        auto target_it = m_target.find(region_id);
        auto updated_it = m_updated.find(region_id);
        if (target_it == m_target.end() || updated_it == m_updated.end()) {
            insert_region(region_id);
            target_it = m_target.find(region_id);
            updated_it = m_updated.find(region_id);
        }
        std::vector<double> *region_target = &(*target_it).second;
        std::vector<bool> *region_updated = &(*updated_it).second;
        if (domain < (int)region_target->size()) {
            (*region_target)[domain] = target;
            (*region_updated)[domain] = true;
        }
        else {
            size_t in_size = region_target->size();
            region_target->resize(domain + 1);
            region_updated->resize(domain + 1);
            std::fill(region_target->begin() + in_size, region_target->end(), GEOPM_POLICY_CONST_INVALID_TARGET);
            std::fill(region_updated->begin() + in_size, region_updated->end(), false);
            (*region_target)[domain] = target;
            (*region_updated)[domain] = true;
        }
    }

    void Policy::update(uint64_t region_id, const std::vector <double> &target)
    {
        auto target_it = m_target.find(region_id);
        auto updated_it = m_updated.find(region_id);
        if (target_it == m_target.end() || updated_it == m_updated.end()) {
            insert_region(region_id);
            target_it = m_target.find(region_id);
            updated_it = m_updated.find(region_id);
        }
        std::vector<double> *region_target = &(*target_it).second;
        std::vector<bool> *region_updated = &(*updated_it).second;
        (*region_target) = target;
        region_updated->resize(target.size());
        std::fill(region_updated->begin(), region_updated->end(), true);
    }

    void Policy::updated_target(uint64_t region_id, std::map <int, double> &target)
    {
        auto target_it = m_target.find(region_id);
        auto updated_it = m_updated.find(region_id);
        if (target_it == m_target.end() || updated_it == m_updated.end()) {
            throw Exception("Policy::updated_target() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<double> *region_target = &(*target_it).second;
        std::vector<bool> *region_updated = &(*updated_it).second;
        target.clear();
        for (int i = 0; i < (int)region_target->size(); ++i) {
            if ((*region_updated)[i] == true && (*region_target)[i] != GEOPM_POLICY_CONST_INVALID_TARGET) {
                target.insert(std::pair <int, double>(i, (*region_target)[i]));
                (*region_updated)[i] = false;
            }
        }
    }

    void Policy::target(uint64_t region_id, std::vector <double> &target)
    {
        auto target_it = m_target.find(region_id);
        auto updated_it = m_updated.find(region_id);
        if (target_it == m_target.end() || updated_it == m_updated.end()) {
            throw Exception("Policy::target() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<double> *region_target = &(*target_it).second;
        std::vector<bool> *region_updated = &(*updated_it).second;
        target = (*region_target);
        std::fill(region_updated->begin(), region_updated->end(), false);
    }

    void Policy::target(uint64_t region_id, int domain, double &target)
    {
        auto target_it = m_target.find(region_id);
        auto updated_it = m_updated.find(region_id);
        if (target_it == m_target.end() || updated_it == m_updated.end()) {
            throw Exception("Policy::target() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<double> *region_target = &(*target_it).second;
        std::vector<bool> *region_updated = &(*updated_it).second;
        if (domain >= (int)region_target->size()) {
            throw Exception("Policy::target() domain index out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        target = (*region_target)[domain];
        (*region_updated)[domain] = false;
    }

    void Policy::valid_target(uint64_t region_id, std::map <int, double> &target) const
    {
        auto target_it = m_target.find(region_id);
        if (target_it == m_target.end()) {
            throw Exception("Policy::valid_target() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const std::vector<double> *region_target = &(*target_it).second;
        target.clear();
        for (int i = 0; i < (int)region_target->size(); ++i) {
            if ((*region_target)[i] != GEOPM_POLICY_CONST_INVALID_TARGET) {
                target.insert(std::pair <int, double>(i, (*region_target)[i]));
            }
        }
    }

    void Policy::policy_message(uint64_t region_id, std::vector<geopm_policy_message_s> message) const
    {
        auto target_it = m_target.find(region_id);
        if (target_it == m_target.end()) {
            throw Exception("Policy::policy_message() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const std::vector<double> *region_target = &(*target_it).second;
        int i = 0;
        int sz = region_target->size();
        message.clear();
        message.resize(sz);

        for (i = 0; i < sz; i++) {
            message[i].region_id = region_id;
            message[i].mode = m_mode;
            message[i].flags = m_flags;
            message[i].num_sample = m_num_sample;
            message[i].power_budget = (*region_target)[i];
        }
    }

    void Policy::is_converged(uint64_t region_id, bool converged_state)
    {
        auto converged_it = m_is_converged.find(region_id);
        if (converged_it == m_is_converged.end()) {
            throw Exception("Policy::is_converged() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        (*converged_it).second = converged_state;
    }

    bool Policy::is_converged(uint64_t region_id) const
    {
        auto converged_it = m_is_converged.find(region_id);
        if (converged_it == m_is_converged.end()) {
            throw Exception("Policy::is_converged() Invalid region id", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return (*converged_it).second;
    }
}
