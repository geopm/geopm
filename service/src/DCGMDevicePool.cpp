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

#include "config.h"

#include <cmath>

#include <iostream>
#include <string>

#include "geopm/Exception.hpp"
#include "DCGMDevicePoolImp.hpp"

namespace geopm
{

    DCGMDevicePool &dcgm_device_pool()
    {
        static DCGMDevicePoolImp instance;
        return instance;
    }

    DCGMDevicePoolImp::DCGMDevicePoolImp()
        : m_update_freq(100000)    // 100 millisecond
        , m_max_keep_age(1.0)      // 1 second
        , m_max_keep_sample(100)   // 100 samples
    {
        m_dcgm_field_ids[M_FIELD_ID_SM_ACTIVE] = DCGM_FI_PROF_SM_ACTIVE;
        m_dcgm_field_ids[M_FIELD_ID_SM_OCCUPANCY] = DCGM_FI_PROF_SM_OCCUPANCY;
        m_dcgm_field_ids[M_FIELD_ID_DRAM_ACTIVE] = DCGM_FI_PROF_DRAM_ACTIVE;

        dcgmReturn_t result;

        //Initialize DCGM
        result = dcgmInit();
        check_result(result, "Error Initializing DCGM.", __LINE__);

        // We are assuming a local version of DCGM.  This could transition to a
        // dcgmStartEmbedded at a later date.
        result = dcgmConnect((char*)"127.0.0.1", &m_dcgm_handle);
        check_result(result, "Error connecting to standalone DCGM instance", __LINE__);

        //Check all devices are DCGM enabled
        unsigned int dcgm_dev_id_list[DCGM_MAX_NUM_DEVICES];
        result = dcgmGetAllSupportedDevices(m_dcgm_handle, dcgm_dev_id_list, &m_dcgm_dev_count);
        check_result(result, "Error fetching DCGM supported devices.", __LINE__);

        dcgmFieldValue_v1 init_val = {};
        init_val.value.dbl = NAN;
        init_val.status = DCGM_ST_UNINITIALIZED;
        std::vector<dcgmFieldValue_v1> field_values(M_NUM_FIELD_ID, init_val);
        m_dcgm_field_values.resize(m_dcgm_dev_count, field_values);

        //Setup Field Group

        char geopm_group[] = "geopm_field_group";
        result = dcgmFieldGroupCreate(m_dcgm_handle, M_NUM_FIELD_ID, m_dcgm_field_ids,
                                      geopm_group, &m_field_group_id);

        //Retry case
        if (result == DCGM_ST_DUPLICATE_KEY) {
#ifdef GEOPM_DEBUG
            std::cerr << "DCGMDevicePool::" << std::string(__func__)  <<
                         ": Duplicate field group found. " << std::endl;
#endif
            result = dcgmFieldGroupDestroy(m_dcgm_handle, m_field_group_id);
            check_result(result, "Error destroying DCGM geopm_fields group.", __LINE__);
            result = dcgmFieldGroupCreate(m_dcgm_handle, M_NUM_FIELD_ID, m_dcgm_field_ids,
                                          geopm_group, &m_field_group_id);
            check_result(result, "Error re-creating DCGM geopm_fields group.", __LINE__);
        }
        else {
            check_result(result, "Error creating DCGM geopm_fields group.", __LINE__);
        }

        // Note: Currently we are using dcgmWatchFields, but may transition to using
        //       dcgmProfWatchFields at a later date
        result = dcgmWatchFields(m_dcgm_handle, DCGM_GROUP_ALL_GPUS, m_field_group_id, m_update_freq,
                                 m_max_keep_age, m_max_keep_sample);

        check_result(result, "Error setting default watch field configuration.", __LINE__);

        for (auto dcgm_field_id : m_dcgm_field_ids) {
            DcgmFieldGetById(dcgm_field_id);
            if (DcgmFieldGetById(dcgm_field_id)->fieldType != DCGM_FT_DOUBLE) {
                throw Exception("DCGMDevicePool::" + std::string(__func__) + ": "
                                "DCGM Field ID " + std::to_string(dcgm_field_id) +
                                " field type is not double",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    DCGMDevicePoolImp::~DCGMDevicePoolImp()
    {
        //Shutdown DCGM
        dcgmFieldGroupDestroy(m_dcgm_handle, m_field_group_id);
        dcgmGroupDestroy(m_dcgm_handle, DCGM_GROUP_ALL_GPUS);
    }

    void DCGMDevicePoolImp::check_result(const dcgmReturn_t result, const std::string error, const int line)
    {
        if (result != DCGM_ST_OK) {
            throw Exception("DCGMDevicePool::" + std::string(__func__) + ": "
                            + error + ": " + errorString(result),
                            GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    int DCGMDevicePoolImp::num_device() const
    {
        return m_dcgm_dev_count;
    }

    double DCGMDevicePoolImp::sample(int accel_idx, int geopm_field_id) const
    {
        double result = NAN;

        if (m_dcgm_field_values.at(accel_idx).at(geopm_field_id).status == DCGM_ST_OK) {
            result = m_dcgm_field_values.at(accel_idx).at(geopm_field_id).value.dbl;
        }
        return result;
    }

    void DCGMDevicePoolImp::update(int accel_idx)
    {
        dcgmReturn_t result;
        result = dcgmGetLatestValuesForFields(m_dcgm_handle, accel_idx,
                        &m_dcgm_field_ids[0], M_NUM_FIELD_ID,
                        &(m_dcgm_field_values.at(accel_idx))[0]);
        check_result(result, "Error getting latest values for fields in read_batch", __LINE__);
    }

    void DCGMDevicePoolImp::update_rate(int field_update_rate)
    {
        dcgmReturn_t result;
        m_update_freq = field_update_rate;
        result = dcgmWatchFields(m_dcgm_handle, DCGM_GROUP_ALL_GPUS, m_field_group_id, m_update_freq,
                                 m_max_keep_age, m_max_keep_sample);
        check_result(result, "Error setting Field Update Rate", __LINE__);
    }

    void DCGMDevicePoolImp::max_storage_time(int max_storage_time)
    {
        dcgmReturn_t result;
        m_max_keep_age = max_storage_time;
        result = dcgmWatchFields(m_dcgm_handle, DCGM_GROUP_ALL_GPUS, m_field_group_id, m_update_freq,
                                 m_max_keep_age, m_max_keep_sample);
        check_result(result, "Error setting Max Storage Time", __LINE__);
    }

    void DCGMDevicePoolImp::max_samples(int max_samples)
    {
        dcgmReturn_t result;
        m_max_keep_sample = max_samples;
        result = dcgmWatchFields(m_dcgm_handle, DCGM_GROUP_ALL_GPUS, m_field_group_id, m_update_freq,
                                 m_max_keep_age, m_max_keep_sample);
        check_result(result, "Error setting Max Samples", __LINE__);
    }
}
