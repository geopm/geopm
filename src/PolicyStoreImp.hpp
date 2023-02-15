/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POLICYSTOREIMP_HPP_INCLUDE
#define POLICYSTOREIMP_HPP_INCLUDE

#include <string>
#include <vector>

#include "PolicyStore.hpp"

struct sqlite3;

namespace geopm
{
    /// Manages a data store of best known policies for profiles used with
    /// agents. The data store includes records of best known policies and
    /// default policies to apply when a best run has not yet been recorded.
    class PolicyStoreImp : public PolicyStore
    {
        public:
            PolicyStoreImp(const std::string &database_path);

            PolicyStoreImp() = delete;
            PolicyStoreImp(const PolicyStoreImp &other) = delete;
            PolicyStoreImp &operator=(const PolicyStoreImp &other) = delete;

            virtual ~PolicyStoreImp();

            std::vector<double> get_best(const std::string &agent_name,
                                         const std::string &profile_name) const override;

            void set_best(const std::string &agent_name,
                          const std::string &profile_name,
                          const std::vector<double> &policy) override;

            void set_default(const std::string &agent_name, const std::vector<double> &policy) override;

        private:
            struct sqlite3 *m_database;
    };
}

#endif
