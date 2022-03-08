/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef DCGMDEVICEPOOLIMP_HPP_INCLUDE
#define DCGMDEVICEPOOLIMP_HPP_INCLUDE

#include <vector>

#include "dcgm_agent.h"
#include "dcgm_structs.h"

#include "DCGMDevicePool.hpp"

namespace geopm
{
    class DCGMDevicePoolImp : public DCGMDevicePool
    {
        public:
            DCGMDevicePoolImp();
            virtual ~DCGMDevicePoolImp();

            virtual int num_device() const override;
            virtual double sample(int accel_idx, int field_id) const override;
            virtual void update(int accel_idx) override;
            virtual void update_rate(int field_update_rate) override;
            virtual void max_storage_time(int max_storage_time) override;
            virtual void max_samples(int max_samples) override;
            virtual void polling_enable(void) override;
            virtual void polling_disable(void) override;

        private:
            virtual void check_result(const dcgmReturn_t result, const std::string error, const int line);

            long long m_update_freq;
            double m_max_keep_age;
            int m_max_keep_sample;
            int m_dcgm_dev_count;
            bool m_dcgm_polling;

            dcgmHandle_t m_dcgm_handle;
            dcgmFieldGrp_t m_field_group_id;

            unsigned short m_dcgm_field_ids[M_NUM_FIELD_ID];

            // Accelerator indexed vector of vector of field values
            std::vector<std::vector<dcgmFieldValue_v1>> m_dcgm_field_values;
    };
}
#endif
