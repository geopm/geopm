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

#include <float.h>

#include "geopm.h"
#include "Policy.hpp"

const double GEOPM_POLICY_CONST_INVALID_TARGET = DBL_MIN;

namespace geopm
{
    Policy::Policy() {}

    Policy::Policy(int num_domain)
        : m_target(num_domain),
          m_updated(num_domain)
    {
        std::fill(m_target.begin(), m_target.end(), GEOPM_POLICY_CONST_INVALID_TARGET);
        std::fill(m_updated.begin(), m_updated.end(), false);
    }

    Policy::Policy(const std::vector <double> &target)
        : m_target(target),
          m_updated(target.size())
    {
        std::fill(m_updated.begin(), m_updated.end(), true);
    }

    Policy::~Policy() {}

    void Policy::clear(void)
    {
        std::fill(m_target.begin(), m_target.end(), GEOPM_POLICY_CONST_INVALID_TARGET);
        std::fill(m_updated.begin(), m_updated.end(), true);
    }

    void Policy::update(int domain, double target)
    {
        if (domain < (int)m_target.size()) {
            m_target[domain] = target;
            m_updated[domain] = true;
        }
        else {
            size_t in_size = m_target.size();
            m_target.resize(domain + 1);
            m_updated.resize(domain + 1);
            std::fill(m_target.begin() + in_size, m_target.end(), GEOPM_POLICY_CONST_INVALID_TARGET);
            std::fill(m_updated.begin() + in_size, m_updated.end(), false);
            m_target[domain] = target;
            m_updated[domain] = true;
        }
    }

    void Policy::update(const std::vector <double> &target)
    {
        m_target = target;
        m_updated.resize(target.size());
        std::fill(m_updated.begin(), m_updated.end(), true);
    }

    void Policy::updated_target(std::map <int, double> &target)
    {
        target.clear();
        for (int i = 0; i < (int)m_target.size(); ++i) {
            if (m_updated[i] == true && m_target[i] != GEOPM_POLICY_CONST_INVALID_TARGET) {
                target.insert(std::pair <int, double>(i, m_target[i]));
                m_updated[i] = false;
            }
        }
    }

    void Policy::target(std::vector <double> &target)
    {
        target = m_target;
        std::fill(m_updated.begin(), m_updated.end(), false);
    }

    void Policy::target(int domain, double &target)
    {
        if (domain >= (int)m_target.size()) {
            throw std::invalid_argument("Domain index out of range\n");
        }
        target = m_target[domain];
        m_updated[domain] = false;
    }

    int Policy::num_domain(void) const
    {
        return m_target.size();
    }

    void Policy::valid_target(std::map <int, double> &target) const
    {
        target.clear();
        for (int i = 0; i < (int)m_target.size(); ++i) {
            if (m_target[i] != GEOPM_POLICY_CONST_INVALID_TARGET) {
                target.insert(std::pair <int, double>(i, m_target[i]));
            }
        }
    }
}
