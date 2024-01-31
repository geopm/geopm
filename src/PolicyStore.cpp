/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "PolicyStore.hpp"

#include <cmath>
#include <memory>

#include "geopm/Exception.hpp"
#include "PolicyStoreImp.hpp"
#include "geopm/Helper.hpp"
#include "geopm_error.h"
#include "geopm_policystore.h"
#include "config.h"

namespace geopm
{
    std::unique_ptr<PolicyStore> PolicyStore::make_unique(const std::string &data_path)
    {
        return geopm::make_unique<PolicyStoreImp>(data_path);
    }

    std::shared_ptr<PolicyStore> PolicyStore::make_shared(const std::string &data_path)
    {
        return std::make_shared<PolicyStoreImp>(data_path);
    }
}

extern "C"
{
    static std::unique_ptr<geopm::PolicyStore> connected_store(nullptr);

    int geopm_policystore_connect(const char *data_path)
    {
        int err = 0;
        if (connected_store) {
            err = GEOPM_ERROR_INVALID;
        }
        else {
            try {
                connected_store.reset(new geopm::PolicyStoreImp(data_path));
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
                err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
            }
        }
        return err;
    }

    int geopm_policystore_disconnect()
    {
        int err = 0;
        try {
            connected_store.reset();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_policystore_get_best(const char *agent_name, const char *profile_name,
                                   size_t max_policy_vals, double *policy_vals)
    {
        int err = 0;
        if (!connected_store) {
            err = GEOPM_ERROR_INVALID;
        }
        else {
            try {
                auto best = connected_store->get_best(agent_name, profile_name);
                if (!policy_vals || best.size() > max_policy_vals) {
                    err = GEOPM_ERROR_INVALID;
                }
                else {
                    std::copy(best.begin(), best.end(), policy_vals);
                    // Policies treat NaN as a default value
                    std::fill_n(policy_vals + best.size(),
                                max_policy_vals - best.size(), NAN);
                }
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
                err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
            }
        }
        return err;
    }

    int geopm_policystore_set_best(const char *agent_name, const char *profile_name,
                                   size_t num_policy_vals, const double *policy_vals)
    {
        int err = 0;
        if (!connected_store) {
            err = GEOPM_ERROR_INVALID;
        }
        else {
            try {
                std::vector<double> policy(policy_vals, policy_vals + num_policy_vals);
                connected_store->set_best(agent_name, profile_name, policy);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
                err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
            }
        }
        return err;
    }

    int geopm_policystore_set_default(const char *agent_name, size_t num_policy_vals,
                                      const double *policy_vals)
    {
        int err = 0;
        if (!connected_store) {
            err = GEOPM_ERROR_INVALID;
        }
        else {
            try {
                std::vector<double> policy(policy_vals, policy_vals + num_policy_vals);
                connected_store->set_default(agent_name, policy);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
                err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
            }
        }
        return err;
    }
}
