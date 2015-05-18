/*
 * Copyright (c) 2015, Intel Corporation
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

#include <set>

#include "Platform.hpp"

namespace geopm
{

    Platform::Platform()
        : m_imp(NULL),
          m_level(-1) {}

    Platform::~Platform()
    {
        std::set <PowerModel *> power_model_set;
        for (auto it = m_power_model.begin(); it != m_power_model.end(); ++it) {
            power_model_set.insert(it->second);
        }
        for (auto it = power_model_set.begin(); it != power_model_set.end(); ++it) {
            delete *it;
        }
    }

    void Platform::set_implementation(PlatformImp* platform_imp)
    {
        PowerModel *power_model = new PowerModel();
        m_imp = platform_imp;
        m_imp->initialize_msrs();
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE_UNCORE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_BOARD_MEMORY, power_model));
    }

    std::string Platform::name(void) const
    {
        if (m_imp == NULL) {
            throw std::runtime_error("Platform implementation is missing");
        }
        return m_imp->get_platform_name();
    }

    void Platform::buffer_index(hwloc_obj_t domain,
                                const std::vector <std::string> &signal_names,
                                std::vector <int> &buffer_index) const
    {
        /* FIXME need to figure out how to implement this function */
        throw std::runtime_error("Platform does not support buffer_index() method\n");
    }

    int Platform::level(void) const
    {
        return m_level;
    }

    void Platform::phase_begin(Phase *phase)
    {
        m_cur_phase = phase;
    }

    void Platform::phase_end(void)
    {
        m_cur_phase = NULL;
    }

    int Platform::num_domain(void) const
    {
        return m_num_domains;
    }

    void Platform::domain_index(int domain_type, std::vector <int> &domain_index) const
    {
        // FIXME
        throw(std::runtime_error("Platform does not support domain_index() method"));
    }

    void Platform::observe(const std::vector <struct sample_message_s> &sample) const
    {
        //check if we are in unmarked code
        if (m_cur_phase == NULL) {
            return;
        }
    }

    Phase *Platform::cur_phase(void) const
    {
        return m_cur_phase;
    }

    const PlatformTopology Platform::topology(void) const
    {
        return m_imp->topology();
    }

    PowerModel *Platform::power_model(int domain_type) const
    {
        auto model =  m_power_model.find(domain_type);
        if (model == m_power_model.end()) {
            throw std::invalid_argument("No PowerModel found for given domain_type\n");
        }
        return model->second;
    }
}
