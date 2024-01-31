/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DCGMDEVICEPOOL_HPP_INCLUDE
#define DCGMDEVICEPOOL_HPP_INCLUDE

namespace geopm
{
    /// An interface for the NVIDIA Data Center GPU Manager (DCGM).
    /// This class is a wrapper around all calls to the DCGM library
    /// and is intended to be called via the DCGMIOGroup.  Its primary
    /// function is provided an abstracted interface to DCGM metrics
    /// of interest.
    class DCGMDevicePool
    {
        public:
            enum m_field_id_e {
                /*!
                 * @brief Field ID associated with DCGM SM Active metrics
                 */
                M_FIELD_ID_SM_ACTIVE,
                /*!
                 * @brief Field ID associated with SM Occupancy metrics
                 */
                M_FIELD_ID_SM_OCCUPANCY,
                /*!
                 * @brief Field ID associated with DCGM DRAM Active metrics
                 */
                M_FIELD_ID_DRAM_ACTIVE,
                /*!
                 * @brief Number of valid field ids
                 */
                M_NUM_FIELD_ID
            };

            DCGMDevicePool() = default;
            virtual ~DCGMDevicePool() = default;

            /// @brief Number of GPUs that support DCGM on the platform.
            /// @return Number of GPUs supported by DCGM.
            virtual int num_device() const = 0;
            /// @brief Get the value for the provided geopm_field_id.
            ///
            /// This value should not change unless update_field_value
            /// has been called
            ///
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            ///
            /// @param [in] field_id One of the m_field_id_e values
            ///
            /// @return The value for the specified field
            virtual double sample(int gpu_idx, int field_id) const = 0;

            /// @brief Query DCGM for the latest value for an GPU.
            ///        Note that this is the last value DCGM cached.  This
            ///        updates the DCGM device pool stored value that is provided
            ///        via the sample_field_value function
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            virtual void update(int gpu_idx) = 0;
            /// @brief Set field update rate for DCGM devices.  This is the rate
            //         at which the DCGM engine will poll for metrics
            /// @param [in] field_update_rate DCGM update rate in microseconds.
            virtual void update_rate(int field_update_rate) = 0;
            /// @brief Set maximum storage time for for DCGM devices.  This is
            ///        the maximum time a DCGM sample will be kept.
            /// @param [in] max_storage_time maximum storage time in seconds
            virtual void max_storage_time(int max_storage_time) = 0;
            /// @brief Set maximum samples to store for for DCGM devices. This is
            ///       the maximum number of DCGM samples that will be kept.
            ///       0 indicates no limit
            /// @param [in] max_samples maximum number of samples to store
            virtual void max_samples(int max_samples) = 0;

            /// @brief Enable DCGM data polling through setting the watch fields
            //         This function may be called repeatedly with updated polling
            //         rate or storage settings.
            virtual void polling_enable(void) = 0;

            /// @brief Disable DCGM data polling through calling unwatchfields
            virtual void polling_disable(void) = 0;
    };

    DCGMDevicePool &dcgm_device_pool();
}
#endif
