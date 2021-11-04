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
    /// An interface for the NVIDIA Data Center GPU Manager (DCGM).
    /// This class is a wrapper around all calls to the DCGM library
    /// and is intented to be called via the DCGMIOGroup.  Its primary
    /// function is provided an abstracted interface to DCGM metrics
    /// of interest.
    class DCGMDevicePool
    {
        public:
            enum geopm_dcgm_field_ids_e {
                /*!
                 * @brief Field ID associated with DCGM SM Active metrics
                 */
                M_SM_ACTIVE,
                /*!
                 * @brief Field ID associated with SM Occupancy metrics
                 */
                M_SM_OCCUPANCY,
                /*!
                 * @brief Field ID associated with DCGM DRAM Active metrics
                 */
                M_DRAM_ACTIVE,
                /*!
                 * @brief Number of valid field ids
                 */
                M_FIELD_IDS
            };

            DCGMDevicePool() = default;
            virtual ~DCGMDevicePool() = default;

            /// @brief Number of accelerators that support DCGM on the platform.
            /// @return Number of accelerators.
            virtual int dcgm_device() const = 0;
            /// @brief Get the value for the provided geopm_field_id.
            ///        This value should not change unless update_field_value
            //         has been called
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator.
            /// @return The value for the specified field
            virtual double sample_field_value(int accel_idx, int geopm_field_id) const = 0;

            /// @brief Query DCGM for the latest value for an accelerator.
            ///        Note that this is the last value DCGM cahced.  This
            //         updates the DCGM device pool stored value that is provided
            //         via the sample_field_value function
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator.
            virtual void update_field_value(int accel_idx) = 0;
            /// @brief Set field update rate for DCGM devices.
            /// @param [in] field_update_rate DCGM update rate in microseconds.
            virtual void field_update_rate(int field_update_rate) = 0;
            /// @brief Set maximum storage time for for DCGM devices.
            /// @param [in] max_storage_time maximum storage time in seconds
            virtual void max_storage_time(int max_storage_time) = 0;
            /// @brief Set maximum samples to store for for DCGM devices.
            /// @param [in] max_samples maximun number of samples to store
            virtual void max_samples(int max_samples) = 0;
    };

    DCGMDevicePool &dcgm_device_pool();
}
#endif
