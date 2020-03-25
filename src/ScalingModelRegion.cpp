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

#include "config.h"
#include "ScalingModelRegion.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "geopm.h"
#include "geopm_time.h"
#include "Exception.hpp"
#include "Profile.hpp"

namespace geopm
{
    ScalingModelRegion::ScalingModelRegion(double big_o_in,
                                           int verbosity,
                                           bool do_imbalance,
                                           bool do_progress,
                                           bool do_unmarked)
        : ModelRegion(verbosity)
        , m_sysfs_cache_dir("/sys/devices/system/cpu/cpu0/cache")
        , m_llc_slop_size(320) // 5 cache lines
        , m_element_size(3 * 8)
        , m_array_len((llc_size() - m_llc_slop_size) / m_element_size) // Array is sized to fit 3 in LLC with slop
        , m_array_a(m_array_len, 0.0)
        , m_array_b(m_array_len, 1.0)
        , m_array_c(m_array_len, 2.0)
    {
        m_name = "scaling";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegion::region(GEOPM_REGION_HINT_MEMORY);
        if (err) {
            throw Exception("ScalingModelRegion::ScalingModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    size_t ScalingModelRegion::llc_size(void)
    {
        size_t result = 0;
        bool do_cont = true;
        std::string contents;
        for (int cache_idx = 0; do_cont; ++cache_idx) {
            std::ifstream fid(m_sysfs_cache_dir + "/index" + std::to_string(cache_idx) + "/size");
            if (fid.good()) {
                std::stringstream ss;
                ss << fid.rdbuf();
                contents = ss.str();
                size_t offset = 0;
                result = std::stoi(contents, &offset);
                if (contents.substr(offset).find('K') == 0) {
                    result *= 1024;
                }
                else if (contents.substr(offset).find('M') == 0) {
                    result *= 1048576;
                }
                else {
                    result = 0;
                }
            }
            else {
                do_cont = false;
            }
        }
        if (result == 0) {
            throw Exception("ScalingModelRegion::llc_size: Unable to parse cache size from sysfs: " + contents,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (result <= m_llc_slop_size + 8 * m_element_size) {
            throw Exception("ScalingModelRegion::llc_size: LLC cache size is too small: " + contents,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    void ScalingModelRegion::run_atom(void)
    {
        double scalar = 3.0;
#pragma omp parallel for
        for (size_t idx = 0; idx < m_array_a.size(); ++idx) {
            m_array_a[idx] += m_array_b[idx] + scalar * m_array_c[idx];
        }
    }

    void ScalingModelRegion::big_o(double big_o_in)
    {
        geopm::Profile &prof = geopm::Profile::default_profile();
        uint64_t start_rid = prof.region("geopm_scaling_model_region_startup", GEOPM_REGION_HINT_IGNORE);
        prof.enter(start_rid);
        m_big_o = big_o_in;
        size_t num_trial = 11;
        size_t median_idx = num_trial / 2;
        std::vector<double> atom_time(num_trial, 0.0);
        for (size_t trial_idx = 0; trial_idx != num_trial; ++trial_idx) {
            struct geopm_time_s time_0;
            geopm_time(&time_0);
            size_t repeat = 10;
            for (size_t it = 0; it != repeat; ++it) {
                run_atom();
            }
            atom_time[trial_idx] = geopm_time_since(&time_0) / repeat;
        }
        std::sort(atom_time.begin(), atom_time.end());
        double median_atom_time = atom_time[median_idx];
        m_num_atom = big_o_in / median_atom_time;
        m_num_atom = m_num_atom ? m_num_atom : 1;
        m_norm = 1.0 / m_num_atom;
        prof.exit(start_rid);
    }

    void ScalingModelRegion::run(void)
    {
        if (m_array_len != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing stream triad of length " << m_array_len << " elements " << m_num_atom << " times."  << std::endl;
            }
            ModelRegion::region_enter();
            for (uint64_t atom_idx = 0; atom_idx < m_num_atom; ++atom_idx) {
                ModelRegion::loop_enter(atom_idx);
                run_atom();
                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
