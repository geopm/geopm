/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "ApplicationIO.hpp"

#include <utility>
#include <unistd.h>
#include <ctime>
#include <iostream>

#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "Environment.hpp"
#include "geopm/ServiceProxy.hpp"

namespace geopm
{
    constexpr size_t ApplicationIOImp::M_SHMEM_REGION_SIZE;

    ApplicationIOImp::ApplicationIOImp()
        : ApplicationIOImp(ServiceProxy::make_unique(),
                           environment().profile(),
                           environment().timeout(),
                           environment().num_proc(),
                           environment().pmpi_ctl())
    {

    }

    ApplicationIOImp::ApplicationIOImp(std::shared_ptr<ServiceProxy> service_proxy,
                                       const std::string &profile_name,
                                       int timeout,
                                       int num_proc,
                                       int ctl_mode)
        : m_is_connected(false)
        , m_service_proxy(std::move(service_proxy))
        , m_profile_name(profile_name)
        , m_timeout(timeout)
        , m_num_proc(num_proc)
        , m_ctl_mode(ctl_mode)
    {

    }

    ApplicationIOImp::~ApplicationIOImp()
    {

    }

    std::vector<int> ApplicationIOImp::connect(void)
    {
        std::vector<int> result(m_profile_pids.begin(), m_profile_pids.end());
        if (m_is_connected) {
            return result;
        }
        struct geopm_time_s time_zero;
        struct geopm_time_s time_curr;
        geopm_time(&time_zero);
        timespec delay = {0, 1000000};
        do {
            m_profile_pids = get_profile_pids();
            if (m_ctl_mode != Environment::M_CTL_PTHREAD) {
                auto ctl_it = m_profile_pids.find(getpid());
                if (ctl_it != m_profile_pids.end()) {
                    m_profile_pids.erase(ctl_it);
                }
            }
            if (m_profile_pids.size() >= (size_t)m_num_proc) {
                m_is_connected = true;
                break;
            }
            clock_nanosleep(CLOCK_REALTIME, 0, &delay, NULL);
            geopm_time(&time_curr);
        } while (!m_is_connected && geopm_time_diff(&time_zero, &time_curr) < m_timeout);

        if (!m_is_connected) {
            std::cerr << "Warning: <geopm> Timeout while trying to detect the application. Possible causes:\n"
                      << "         1. Application processes have a very short duration\n"
                      << "         2. GEOPM_PROGRAM_FILTER is not set correctly in the application environment (does not match program invocation name)\n"
                      << "         3. GEOPM_NUM_PROC is set to more processes than are created with matching program invocation names." << std::endl;
        }
#ifdef GEOPM_DEBUG
        std::cout << "Info: <geopm> Controller will profile PIDs: ";
        for (auto pid : m_profile_pids) {
            std::cout << pid << " ";
        }
        std::cout << std::endl;
#endif
        result.assign(m_profile_pids.begin(), m_profile_pids.end());
        return result;
    }

    // Private helper function
    std::set<int> ApplicationIOImp::get_profile_pids(void)
    {
        auto profile_pids = m_service_proxy->platform_get_profile_pids(m_profile_name);
        return std::set<int>(profile_pids.begin(), profile_pids.end());
    }

    bool ApplicationIOImp::do_shutdown(void)
    {
        bool result = false;
        // Remove the first elements of the set which are not running
        // or zombied
        for (auto pid_it = m_profile_pids.begin(); pid_it != m_profile_pids.end();)
        {
            if (getpgid(*pid_it) == -1) {
                // PID is no longer valid
                errno = 0;
                pid_it = m_profile_pids.erase(pid_it);
            }
            else {
                // We don't need to prune all values if we find one
                // valid PID
                break;
            }
        }
        // If we have removed all of the PIDs that were previously
        // discovered then the application has ended
        geopm_time_s time_now;
        geopm_time(&time_now);
        if (m_profile_pids.size() == 0) {
            result = true;
        }
        return result;
    }

    std::set<std::string> ApplicationIOImp::region_name_set(void) const
    {
        std::set<std::string> result;
        auto profile_names = m_service_proxy->platform_pop_profile_region_names(m_profile_name);
        for (const auto &name : profile_names) {
            result.insert(name);
        }
        return result;
    }
}
