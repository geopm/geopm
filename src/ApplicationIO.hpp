/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APPLICATIONIO_HPP_INCLUDE
#define APPLICATIONIO_HPP_INCLUDE

#include <cstdint>
#include <set>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <ctime>

#include "geopm_hint.h"
#include "geopm_time.h"

namespace geopm
{
    class Comm;
    class ServiceProxy;

    class ApplicationIO
    {
        public:
            ApplicationIO() = default;
            virtual ~ApplicationIO() = default;
            /// @brief Connect to the application via
            ///        shared memory.
            virtual std::vector<int> connect(void) = 0;
            /// @brief Returns true if the application has indicated
            ///        it is shutting down.
            virtual bool do_shutdown(void) = 0;
            /// @brief Returns the path to the report file.
            virtual std::string report_name(void) const = 0;
            /// @brief Returns the profile name to be used in the
            ///        report.
            virtual std::string profile_name(void) const = 0;
            /// @brief Returns the set of region names recorded by the
            ///        application.
            virtual std::set<std::string> region_name_set(void) const = 0;
    };

    class ApplicationIOImp : public ApplicationIO
    {
        public:
            ApplicationIOImp();
            ApplicationIOImp(std::shared_ptr<ServiceProxy> service_proxy,
                             const std::string &profile_name,
                             const std::string &report_name,
                             int timeout,
                             int num_proc);
            virtual ~ApplicationIOImp();
            std::vector<int> connect(void) override;
            bool do_shutdown(void) override;
            std::string report_name(void) const override;
            std::string profile_name(void) const override;
            std::set<std::string> region_name_set(void) const override;
        private:
            std::set<int> get_profile_pids(void);
            static constexpr size_t M_SHMEM_REGION_SIZE = 2*1024*1024;

            bool m_is_connected;
            std::shared_ptr<ServiceProxy> m_service_proxy;
            const std::string m_profile_name;
            const std::string m_report_name;
            const int m_timeout;
            std::set<int> m_profile_pids;
            int m_num_proc;
    };
}

#endif
