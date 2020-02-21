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
#include "ModelRegion.hpp"

#include "geopm.h"
#include "geopm_imbalancer.h"
#include "Exception.hpp"
#include "Helper.hpp"

#include "NestedModelRegion.hpp"
#include "SleepModelRegion.hpp"
#include "All2allModelRegion.hpp"
#include "DGEMMModelRegion.hpp"
#include "StreamModelRegion.hpp"
#include "SpinModelRegion.hpp"
#include "IgnoreModelRegion.hpp"
#include "ScalingModelRegion.hpp"
#include "BarrierModelRegion.hpp"
#include "ReduceModelRegion.hpp"
#include "TimedScalingModelRegion.hpp"

namespace geopm
{
    bool ModelRegion::name_check(const std::string &name, const std::string &key)
    {
        bool result = false;
        size_t key_size = key.size();
        if (name.find(key) == 0 &&
            (name[key_size] == '\0' ||
             name[key_size] == '-')) {
            result = true;
        }
        return result;
    }

    std::unique_ptr<ModelRegion> ModelRegion::model_region(const std::string &name,
                                                           double big_o,
                                                           int verbosity)
    {
        bool do_imbalance = (name.find("-imbalance") != std::string::npos);
        bool do_progress = (name.find("-progress") != std::string::npos);
        bool do_unmarked = (name.find("-unmarked") != std::string::npos);

        if (do_unmarked) {
            do_progress = false;
        }

        if (name_check(name, "sleep")) {
            return geopm::make_unique<SleepModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "spin")) {
            return geopm::make_unique<SpinModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "dgemm")) {
            return geopm::make_unique<DGEMMModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "stream")) {
            return geopm::make_unique<StreamModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "all2all")) {
            return geopm::make_unique<All2allModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "nested")) {
            return geopm::make_unique<NestedModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "ignore")) {
            return geopm::make_unique<IgnoreModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "scaling")) {
            return geopm::make_unique<ScalingModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "barrier")) {
            return geopm::make_unique<BarrierModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "reduce")) {
            return geopm::make_unique<ReduceModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name_check(name, "timed_scaling")) {
            return geopm::make_unique<TimedScalingModelRegion>(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else {
            throw Exception("model_region_factory: unknown name: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    ModelRegion::ModelRegion(int verbosity)
        : m_big_o(0.0)
        , m_verbosity(verbosity)
        , m_do_imbalance(false)
        , m_do_progress(false)
        , m_do_unmarked(false)
        , m_num_progress_updates(1)
        , m_norm(1.0)
    {

    }

    ModelRegion::~ModelRegion()
    {

    }

    std::string ModelRegion::name(void)
    {
        return m_name;
    }

    double ModelRegion::big_o(void)
    {
        return m_big_o;
    }

    void ModelRegion::num_progress_updates(double big_o_in)
    {
        if (!m_do_progress) {
            m_num_progress_updates = 1;
        }
        else if (big_o_in > 1.0) {
            m_num_progress_updates = (uint64_t)(100.0 * big_o_in);
        }
        else {
            m_num_progress_updates = 100;
        }
        m_norm = 1.0 / m_num_progress_updates;
    }

    int ModelRegion::region(uint64_t hint)
    {
        int err = 0;
        if (!m_do_unmarked) {
            err = geopm_prof_region(m_name.c_str(), hint, &m_region_id);
        }
        return err;
    }

    int ModelRegion::region(void)
    {
        return region(GEOPM_REGION_HINT_UNKNOWN);
    }

    void ModelRegion::region_enter(void)
    {
        if (!m_do_unmarked) {
            (void)geopm_prof_enter(m_region_id);
        }
    }

    void ModelRegion::region_exit(void)
    {
        if (!m_do_unmarked) {
            (void)geopm_prof_exit(m_region_id);
        }
    }

    void ModelRegion::loop_enter(uint64_t iteration)
    {
        if (m_do_progress) {
            (void)geopm_prof_progress(m_region_id, iteration * m_norm);
        }
        if (m_do_imbalance) {
            (void)geopm_imbalancer_enter();
        }
    }

    void ModelRegion::loop_exit(void)
    {
        if (m_do_imbalance) {
            (void)geopm_imbalancer_exit();
        }
    }
}
