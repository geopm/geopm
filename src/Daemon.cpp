/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "Daemon.hpp"
#include "DaemonImp.hpp"

#include <chrono>
#include <thread>

#include "geopm_time.h"
#include "Exception.hpp"
#include "Endpoint.hpp"
#include "PolicyStore.hpp"
#include "Helper.hpp"

namespace geopm
{
    std::unique_ptr<Daemon> Daemon::make_unique(const std::string &endpoint_name,
                                                const std::string &db_path)
    {
        return geopm::make_unique<DaemonImp>(endpoint_name, db_path);
    }

    DaemonImp::DaemonImp(const std::string &endpoint_name,
                         const std::string &db_path)
        : DaemonImp(Endpoint::make_unique(endpoint_name),
                    PolicyStore::make_unique(db_path))
    {

    }

    DaemonImp::DaemonImp(std::shared_ptr<Endpoint> endpoint,
                         std::shared_ptr<PolicyStore> policystore)
        : m_endpoint(endpoint)
        , m_policystore(policystore)
    {
        m_endpoint->open();
    }

    DaemonImp::~DaemonImp()
    {
        m_endpoint->close();
    }

    void DaemonImp::update_endpoint_from_policystore(volatile bool &cancel,
                                                     double timeout)
    {
        std::string agent = "";
        geopm_time_s start;
        geopm_time(&start);
        while (!cancel && agent == "") {
            agent = m_endpoint->get_agent();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (geopm_time_since(&start) >= timeout) {
                throw Exception("Daemon: timed out waiting for controller.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        if (agent != "") {
            std::string profile_name = m_endpoint->get_profile_name();
            auto policy = m_policystore->get_best(profile_name, agent);
            m_endpoint->write_policy(policy);
        }
    }
}
