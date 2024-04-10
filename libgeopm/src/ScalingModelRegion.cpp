/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "ScalingModelRegion.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Profile.hpp"
#include "Comm.hpp"

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
        , m_rank_per_node(Comm::make_unique()->split("", Comm::M_COMM_SPLIT_TYPE_SHARED)->num_rank())
        , m_array_len((llc_size() / m_rank_per_node - m_llc_slop_size) / m_element_size) // Array is sized to fit 3 in the LLC with slop assuming one LLC per node
        , m_arrays(3, nullptr)
    {
        int err = 0;
        size_t align = 4096;
        size_t array_size = m_array_len * sizeof(double);
        for (auto &it : m_arrays) {
            err = posix_memalign((void **)&it, align, array_size);
            if (err) {
                throw Exception("ScalingModelRegion: posix_memalign error",
                                err, __FILE__, __LINE__);
            }
        }
        std::fill(m_arrays[0], m_arrays[0] + m_array_len, 0.0);
        std::fill(m_arrays[1], m_arrays[1] + m_array_len, 1.0);
        std::fill(m_arrays[2], m_arrays[2] + m_array_len, 2.0);

        m_name = "scaling";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;

        ModelRegion::region(GEOPM_REGION_HINT_MEMORY);
        big_o(big_o_in);
    }

    ScalingModelRegion::~ScalingModelRegion()
    {
        for (auto &it : m_arrays) {
            free(it);
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
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
        for (size_t idx = 0; idx < m_array_len; ++idx) {
            m_arrays[0][idx] += m_arrays[1][idx] + scalar * m_arrays[2][idx];
        }
    }

    void ScalingModelRegion::big_o(double big_o_in)
    {
        // run_atom is called 2000 times prior to calibration to
        // resolve issues with low IPC during calibration that lead to
        // a small num_atom value and short duration scaling model regions.
        for (size_t prep_idx = 0; prep_idx < 2000; ++prep_idx) {
            run_atom();
        }

        uint64_t start_rid = 0;
        geopm_prof_region("geopm_scaling_model_region_startup", GEOPM_REGION_HINT_IGNORE, &start_rid);
        geopm_prof_enter(start_rid);
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
        geopm_prof_exit(start_rid);
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
