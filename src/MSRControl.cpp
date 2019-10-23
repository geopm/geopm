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

#include "MSRControlImp.hpp"

#include <cmath>

#include "MSR.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    std::unique_ptr<MSRControl> MSRControl::make_unique(const MSR &msr_obj,
                                                        int domain_type,
                                                        int cpu_idx,
                                                        int control_idx)
    {
        return geopm::make_unique<MSRControlImp>(msr_obj, domain_type, cpu_idx, control_idx);
    }

    MSRControlImp::MSRControlImp(const MSR &msr_obj,
                                 int domain_type,
                                 int cpu_idx,
                                 int control_idx)
        : m_name(msr_obj.name() + ":" + msr_obj.control_name(control_idx))
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_control_idx(control_idx)
        , m_field_ptr(nullptr)
        , m_mask_ptr(nullptr)
        , m_is_field_mapped(false)
    {

    }

    MSRControlImp::MSRControlImp(const MSRControlImp &other)
        : m_name(other.m_name)
        , m_msr_obj(other.m_msr_obj)
        , m_domain_type(other.m_domain_type)
        , m_cpu_idx(other.m_cpu_idx)
        , m_control_idx(other.m_control_idx)
        , m_field_ptr(nullptr)
        , m_mask_ptr(nullptr)
        , m_is_field_mapped(false)
    {

    }

    MSRControlImp::~MSRControlImp()
    {

    }

    std::unique_ptr<MSRControl> MSRControlImp::copy_and_remap(uint64_t *field,
                                                              uint64_t *mask) const
    {
        std::unique_ptr<MSRControl> result {new MSRControlImp(*this)};
        result->map_field(field, mask);
        return result;
    }

    std::string MSRControlImp::name() const
    {
        return m_name;
    }

    int MSRControlImp::domain_type(void) const
    {
        return m_domain_type;
    }

    int MSRControlImp::cpu_idx(void) const
    {
        return m_cpu_idx;
    }

    void MSRControlImp::adjust(double setting)
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRControlImp::adjust(): must call map() method before adjust() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_msr_obj.control(m_control_idx, setting, *m_field_ptr, *m_mask_ptr);
    }

    uint64_t MSRControlImp::offset(void) const
    {
        return m_msr_obj.offset();
    }

    uint64_t MSRControlImp::mask(void) const
    {
        return m_msr_obj.mask(m_control_idx);
    }

    void MSRControlImp::map_field(uint64_t *field, uint64_t *mask)
    {
        m_field_ptr = field;
        m_mask_ptr = mask;
        m_is_field_mapped = true;
    }
}
