/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
