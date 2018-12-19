/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <unistd.h>
#include <limits.h>

#include "geopm.h"
#include "ModelRegion.hpp"
#include "Exception.hpp"
#include "ModelApplication.hpp"
#include "config.h"

namespace geopm
{
    ModelApplication::ModelApplication(uint64_t repeat, std::vector<std::string> region_name, std::vector<double> big_o, int verbosity, int rank)
        : m_repeat(repeat)
        , m_rank(rank)
    {
        if (region_name.size() != big_o.size()) {
            throw Exception("ModelApplication: Length of region names is different than the length of big_o",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto name_it = region_name.begin();
        for (auto big_o_it = big_o.begin(); big_o_it != big_o.end(); ++big_o_it, ++name_it) {
            m_region.push_back(model_region_factory(*name_it, *big_o_it, verbosity));
        }
    }

    ModelApplication::~ModelApplication()
    {
        for (auto it = m_region.rbegin(); it != m_region.rend(); ++it) {
            delete *it;
        }
    }

    void ModelApplication::run(void)
    {
        if (!m_rank) {
            std::cout << "Beginning loop of " << m_repeat << " iterations." << std::endl << std::flush;
        }
        for (uint64_t i = 0; i < m_repeat; ++i) {
            (void)geopm_prof_epoch();
            for (auto it = m_region.begin(); it != m_region.end(); ++it) {
                (*it)->run();
            }
            if (!m_rank) {
                std::cout << "Iteration: " << i << "\r" << std::flush;
            }
        }
        if (!m_rank) {
            std::cout << "Completed loop.                    " << std::endl << std::flush;
        }
    }
}
