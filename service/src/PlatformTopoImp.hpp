/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef PLATFORMTOPOIMP_HPP_INCLUDE
#define PLATFORMTOPOIMP_HPP_INCLUDE

#include "geopm/PlatformTopo.hpp"
#include <vector>
#include <map>
#include <memory>

namespace geopm
{
    class GPUTopo;
    class ServiceProxy;

    class PlatformTopoImp : public PlatformTopo
    {
        public:
            PlatformTopoImp();
            PlatformTopoImp(const std::string &test_cache_file_name,
                            std::shared_ptr<ServiceProxy> service_proxy);
            virtual ~PlatformTopoImp() = default;
            int num_domain(int domain_type) const override;
            int domain_idx(int domain_type,
                           int cpu_idx) const override;
            bool is_nested_domain(int inner_domain, int outer_domain) const override;
            std::set<int> domain_nested(int inner_domain, int outer_domain, int outer_idx) const override;
            static void create_cache();
            static void create_cache(const std::string &cache_file_name);
            static void create_cache(const std::string &cache_file_name, const GPUTopo &gtopo);
        private:
            static const std::string M_CACHE_FILE_NAME;
            static const std::string M_SERVICE_CACHE_FILE_NAME;
            /// @brief Get the set of Linux logical CPUs associated
            ///        with the indexed domain.
            std::set<int> domain_cpus(int domain_type,
                                      int domain_idx) const;

            void lscpu(std::map<std::string, std::string> &lscpu_map);
            void parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                             int &num_package,
                             int &core_per_package,
                             int &thread_per_core);
            std::vector<std::set<int> > parse_lscpu_numa(const std::map<std::string, std::string> &lscpu_map);
            std::vector<std::set<int> > parse_lscpu_gpu(const std::map<std::string, std::string> &lscpu_map, int domain_type);
            std::string read_lscpu(void);
            static bool check_file(const std::string &file_name);
            static std::string gpu_short_name(int domain_type);
            static std::unique_ptr<ServiceProxy> try_service_proxy(void);
            const std::string M_TEST_CACHE_FILE_NAME;
            int m_num_package;
            int m_core_per_package;
            int m_thread_per_core;
            std::vector<std::set<int> > m_numa_map;
            std::map<int, std::vector<std::set<int> > > m_gpu_info;
            std::shared_ptr<ServiceProxy> m_service_proxy;
    };
}
#endif
