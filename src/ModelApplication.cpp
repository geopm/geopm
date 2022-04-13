/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ModelApplication.hpp"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <unistd.h>
#include <limits.h>

#include "geopm_prof.h"
#include "ModelRegion.hpp"
#include "geopm/Exception.hpp"
#include "config.h"

namespace geopm
{
    ModelApplication::ModelApplication(uint64_t repeat, const std::vector<std::string> &region_name, const std::vector<double> &big_o, int verbosity, int rank)
        : m_repeat(repeat)
        , m_rank(rank)
    {
        if (region_name.size() != big_o.size()) {
            throw Exception("ModelApplication: Length of region names is different than the length of big_o",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto name_it = region_name.begin();
        for (auto big_o_it = big_o.begin(); big_o_it != big_o.end(); ++big_o_it, ++name_it) {
            m_region.emplace_back(ModelRegion::model_region(*name_it, *big_o_it, verbosity));
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
