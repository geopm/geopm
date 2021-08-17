/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef LEVELZERODEVICEPOOLIMP_HPP_INCLUDE
#define LEVELZERODEVICEPOOLIMP_HPP_INCLUDE

#include <string>
#include <cstdint>

#include "LevelZeroDevicePool.hpp"
#include "LevelZero.hpp"

#include "geopm_time.h"

namespace geopm
{
    class LevelZeroDevicePoolImp : public LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePoolImp();
            LevelZeroDevicePoolImp(const LevelZero &levelzero);
            virtual ~LevelZeroDevicePoolImp() = default;
            virtual int num_accelerator(int domain_type) const override;

            virtual double frequency_status(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            virtual double frequency_min(int domain, unsigned int domain_idx,
                                         int l0_domain) const override;
            virtual double frequency_max(int domain, unsigned int domain_idx,
                                         int l0_domain) const override;
            virtual std::pair<uint64_t, uint64_t> active_time_pair(int domain,
                                                                   unsigned int device_idx,
                                                                   int l0_domain) const override;
            virtual uint64_t active_time(int domain, unsigned int device_idx,
                                         int l0_domain) const override;
            virtual uint64_t active_time_timestamp(int domain, unsigned int device_idx,
                                                   int l0_domain) const override;
            virtual int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            virtual int32_t power_limit_min(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            virtual int32_t power_limit_max(int domain, unsigned int domain_idx,
                                            int l0_domain) const override;
            virtual std::pair<uint64_t, uint64_t> energy_pair(int domain,
                                                              unsigned int domain_idx,
                                                              int l0_domain) const override;
            virtual uint64_t energy(int domain, unsigned int domain_idx, int l0_domain) const override;
            virtual uint64_t energy_timestamp(int domain, unsigned int domain_idx,
                                              int l0_domain) const override;

            virtual void frequency_control(int domain, unsigned int domain_idx,
                                           int l0_domain, double setting) const override;

        private:
            const LevelZero &m_levelzero;

            virtual void check_idx_range(int domain, unsigned int domain_idx) const;
            virtual void check_domain_exists(int size, const char *func, int line) const;
            virtual std::pair<unsigned int, unsigned int> subdevice_device_conversion(unsigned int idx) const;
    };
}
#endif
