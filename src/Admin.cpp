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

#include "config.h"
#include <sstream>
#include <memory>
#include "Admin.hpp"
#include "Environment.hpp"
#include "Exception.hpp"
#include "MSRIOGroup.hpp"

namespace geopm
{
    Admin::Admin()
        : Admin(geopm::environment().default_config_path(),
                geopm::environment().override_config_path(),
                nullptr)
    {

    }

    Admin::Admin(const std::string &default_config_path,
                 const std::string &override_config_path,
                 std::shared_ptr<MSRIOGroup> msriog)
        : m_default_config_path(default_config_path)
        , m_override_config_path(override_config_path)
        , m_msriog(msriog)
    {

    }

    std::string Admin::main(bool do_default,
                            bool do_override,
                            bool do_whitelist,
                            int cpuid)
    {
        int action_count = 0;
        action_count += do_default;
        action_count += do_override;
        action_count += do_whitelist;
        if (action_count > 1) {
            throw geopm::Exception("geopmadmin: -d, -o and -w must be used exclusively",
                                   EINVAL, __FILE__, __LINE__);
        }

        std::string result;
        if (do_default) {
            result = default_config();
        }
        else if (do_override) {
            result = override_config();
        }
        else if (do_whitelist) {
            result = whitelist(cpuid);
        }
        else {
            result = check_node();
        }
        return result;
    }

    std::string Admin::default_config(void)
    {
        return m_default_config_path + "\n";
    }

    std::string Admin::override_config(void)
    {
        return m_override_config_path + "\n";
    }

    std::string Admin::whitelist(int cpuid)
    {
        std::string result;
        if (cpuid == -1) {
           if (m_msriog == nullptr) {
               m_msriog = std::make_shared<MSRIOGroup>();
           }
           result = m_msriog->msr_whitelist();
        }
        else {
            result = geopm::MSRIOGroup::msr_whitelist(cpuid);
        }
        return result;
    }

    std::string Admin::check_node(void)
    {
        throw geopm::Exception("Check has not been implemented yet",
                               GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return "";
    }
}
