/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

#include "LevelZeroDevicePool.hpp"
#include "LevelZeroShim.hpp"

#include "geopm_time.h"

namespace geopm
{

    class LevelZeroDevicePoolImp : public LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePoolImp(const int num_cpu);
            virtual ~LevelZeroDevicePoolImp();
            virtual int num_accelerator(void) const override;
            virtual double frequency_status(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual double frequency_min(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual double frequency_max(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual uint64_t active_time(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual uint64_t active_time_timestamp(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual int32_t power_limit_tdp(unsigned int accel_idx) const override;
            virtual int32_t power_limit_min(unsigned int accel_idx) const override;
            virtual int32_t power_limit_max(unsigned int accel_idx) const override;
            virtual uint64_t energy(unsigned int accel_idx) const override;
            virtual uint64_t energy_timestamp(unsigned int accel_idx) const override;

            virtual void frequency_control(unsigned int accel_idx, geopm_levelzero_domain_e domain, double setting) const override;

        private:
            const unsigned int M_NUM_CPU;
            const LevelZeroShim &m_shim;

            virtual void check_accel_range(unsigned int accel_idx) const;
            virtual void check_domain_range(int size, const char *func, int line) const;
    };
}
#endif
