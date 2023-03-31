/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "ApplicationIO.hpp"

#include <utility>
#include <unistd.h>
#include <ctime>

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
                           environment().report(),
                           environment().timeout(),
                           environment().num_proc())
    {

    }

    ApplicationIOImp::ApplicationIOImp(std::shared_ptr<ServiceProxy> service_proxy,
                                       const std::string &profile_name,
                                       const std::string &report_name,
                                       int timeout,
                                       int num_proc)
        : m_is_connected(false)
        , m_service_proxy(service_proxy)
        , m_profile_name(profile_name)
        , m_report_name(report_name)
        , m_timeout(timeout)
        , m_num_proc(num_proc)
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
        double timeout = m_timeout;
        if (timeout < 0.0) {
            timeout = 60.0;
        }
        struct geopm_time_s time_zero;
        struct geopm_time_s time_curr;
        geopm_time(&time_zero);
        timespec delay = {0, 1000000};
        do {
            m_profile_pids = get_profile_pids();
            auto ctl_it = m_profile_pids.find(getpid());
            if (ctl_it != m_profile_pids.end()) {
                m_profile_pids.erase(ctl_it);
            }
            if (m_profile_pids.size() >= (size_t)m_num_proc) {
                m_is_connected = true;
                break;
            }
            clock_nanosleep(CLOCK_REALTIME, 0, &delay, NULL);
            geopm_time(&time_curr);
        } while (!m_is_connected && geopm_time_diff(&time_zero, &time_curr) < timeout);
        m_slow_loop_last = time(nullptr);
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
        auto time_now = std::time(nullptr);
        if (m_profile_pids.size() == 0) {
            result = true;
        }
        // If the slow loop interval has expired, check to see if all
        // PIDs have requested an end to profiling.
        else if (std::difftime(time_now, m_slow_loop_last) > M_SLOW_LOOP_PERIOD) {
            m_slow_loop_last = time_now;
            m_profile_pids = get_profile_pids();
            if (m_profile_pids.size() == 0) {
                result = true;
            }
        }
        return result;
    }

    std::string ApplicationIOImp::report_name(void) const
    {
        // Get report name from controller's environment not the
        // application environment
        return m_report_name;
    }

    std::string ApplicationIOImp::profile_name(void) const
    {
        return m_profile_name;
    }

    std::set<std::string> ApplicationIOImp::region_name_set(void) const
    {
        std::set<std::string> result;
        auto profile_names = m_service_proxy->platform_get_profile_region_names(m_profile_name);
        for (const auto &name : profile_names) {
            result.insert(name);
        }
        return result;
    }
}
