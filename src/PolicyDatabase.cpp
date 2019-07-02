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
#include "PolicyDatabase.hpp"
#include "geopm_policy_database.h"
#include "geopm_error.h"

namespace geopm
{
    PolicyDatabase::PolicyDatabase(const std::string& database_path)
        : m_database(0)
    {
    }

    PolicyDatabase::~PolicyDatabase()
    {
    }

    std::string PolicyDatabase::GetBestPolicy(const std::string& profile_name,
                                              const std::string& agent_name) const
    {
        return std::string();
    }

    std::string PolicyDatabase::GetBestReport(const std::string& profile_name,
                                              const std::string& agent_name) const
    {
        return std::string();
    }

    void PolicyDatabase::SetBestPolicy(const std::string& profile_name,
                                       const std::string& agent_name,
                                       const std::string& policy,
                                       const std::string& report)
    {
    }

    void PolicyDatabase::SetDefaultPolicy(const std::string& agent_name,
                                          const std::string& policy)
    {
    }

extern "C" {
    struct geopm_policy_database* geopm_policy_database_connect(const char *database_path)
    {
        return nullptr;
    }

    void geopm_policy_database_disconnect(struct geopm_policy_database *database)
    {
    }

    int geopm_policy_database_get_best_policy(const struct geopm_policy_database *database,
                                              const char* profile_name, const char* agent_name,
                                              size_t policy_size, char* policy)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policy_database_get_best_report(const struct geopm_policy_database *database,
                                              const char* profile_name, const char* agent_name,
                                              size_t report_size, char* report)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policy_database_set_best_policy(struct geopm_policy_database *database, const char* profile_name,
                                              const char* agent_name, const char* policy, const char* report)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policy_database_set_default_policy(struct geopm_policy_database *database,
                                                 const char* agent_name, const char* policy)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }
}
}
