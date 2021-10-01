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

#ifndef DCGMDEVICEPOOL_HPP_INCLUDE
#define DCGMDEVICEPOOL_HPP_INCLUDE

namespace geopm
{
    class DCGMDevicePool
    {
        public:
            enum geopm_dcgm_field_ids_e {
                M_SM_ACTIVE,
                M_SM_OCCUPANCY,
                M_DRAM_ACTIVE,
                M_PCIE_TX_BYTES,
                M_PCIE_RX_BYTES,
                M_FIELD_IDS
            };

            DCGMDevicePool() = default;
            virtual ~DCGMDevicePool() = default;

            virtual int dcgm_device() const = 0;
            virtual double field_value(int accel_idx, int geopm_field_id) const = 0;
            virtual void update_field_value(int accel_idx) = 0;
            virtual void field_update_rate(int accel_idx, int field_update_rate) = 0;
            virtual void max_storage_time(int accel_idx, int max_storage_time) = 0;
            virtual void max_samples(int accel_idx, int max_samples) = 0;
    };

    DCGMDevicePool &dcgm_device_pool();
}
#endif
