/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "MSRSignalImp.hpp"

#include <cmath>

#include "geopm_hash.h"
#include "MSR.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    MSRSignalImp::MSRSignalImp(const MSR &msr_obj,
                               int domain_type,
                               int cpu_idx,
                               int signal_idx)
        : m_name(msr_obj.name() + ":" + msr_obj.signal_name(signal_idx))
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_signal_idx(signal_idx)
        , m_field_ptr(nullptr)
        , m_field_last(0)
        , m_num_overflow(0)
        , m_is_field_mapped(false)
        , m_is_raw(false)
    {

    }

    MSRSignalImp::MSRSignalImp(const MSR &msr_obj,
                               int domain_type,
                               int cpu_idx)
        : m_name(msr_obj.name() + "#")
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_signal_idx(0)
        , m_field_ptr(nullptr)
        , m_field_last(0)
        , m_num_overflow(0)
        , m_is_field_mapped(false)
        , m_is_raw(true)
    {

    }

    MSRSignalImp::MSRSignalImp(const MSRSignalImp &other)
        : m_name(other.m_name)
        , m_msr_obj(other.m_msr_obj)
        , m_domain_type(other.m_domain_type)
        , m_cpu_idx(other.m_cpu_idx)
        , m_signal_idx(other.m_signal_idx)
        , m_field_ptr(nullptr)
        , m_field_last(other.m_field_last)
        , m_num_overflow(other.m_num_overflow)
        , m_is_field_mapped(false)
        , m_is_raw(other.m_is_raw)
    {

    }

    std::unique_ptr<MSRSignal> MSRSignalImp::copy_and_remap(const uint64_t *field) const
    {
        std::unique_ptr<MSRSignal> result {new MSRSignalImp(*this)};
        result->map_field(field);
        return result;
    }

    std::string MSRSignalImp::name(void) const
    {
        return m_name;
    }

    int MSRSignalImp::domain_type(void) const
    {
        return m_domain_type;
    }

    int MSRSignalImp::cpu_idx(void) const
    {
        return m_cpu_idx;
    }

    double MSRSignalImp::sample(void)
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRSignalImp::sample(): must call map() method before sample() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        double result = NAN;
        if (!m_is_raw) {
            result = m_msr_obj.signal(m_signal_idx, *m_field_ptr, m_field_last, m_num_overflow);
        }
        else {
            result = geopm_field_to_signal(*m_field_ptr);
        }
        return result;
    }

    uint64_t MSRSignalImp::offset(void) const
    {
        return m_msr_obj.offset();
    }

    void MSRSignalImp::map_field(const uint64_t *field)
    {
        m_field_ptr = field;
        m_is_field_mapped = true;
    }
}
