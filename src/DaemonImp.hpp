/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DAEMONIMP_HPP_INCLUDE
#define DAEMONIMP_HPP_INCLUDE

#include <memory>

#include "Daemon.hpp"

namespace geopm
{
    class Endpoint;
    class PolicyStore;

    class DaemonImp : public Daemon
    {
        public:
            DaemonImp(const std::string &endpoint_name,
                      const std::string &db_path);
            DaemonImp(std::shared_ptr<Endpoint> endpoint,
                      std::shared_ptr<const PolicyStore> policystore);
            DaemonImp(const DaemonImp &other) = delete;
            DaemonImp &operator=(const DaemonImp &other) = delete;
            virtual ~DaemonImp();

            void update_endpoint_from_policystore(double timeout) override;
            void stop_wait_loop() override;
            void reset_wait_loop() override;
        private:
            std::shared_ptr<Endpoint> m_endpoint;
            std::shared_ptr<const PolicyStore> m_policystore;
    };
}

#endif
