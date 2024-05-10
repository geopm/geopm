/*
 * Copyright (c) 2015 - 2024 Intel Corporation
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
                             int timeout,
                             int num_proc,
                             int ctl_mode);
            virtual ~ApplicationIOImp();
            std::vector<int> connect(void) override;
            bool do_shutdown(void) override;
            std::set<std::string> region_name_set(void) const override;
        private:
            std::set<int> get_profile_pids(void);
            static constexpr size_t M_SHMEM_REGION_SIZE = 2*1024*1024;

            bool m_is_connected;
            std::shared_ptr<ServiceProxy> m_service_proxy;
            const std::string m_profile_name;
            const int m_timeout;
            std::set<int> m_profile_pids;
            int m_num_proc;
            int m_ctl_mode;
    };
}

#endif
