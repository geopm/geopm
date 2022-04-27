/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef PLATFORMTOPOIMP_HPP_INCLUDE
#define PLATFORMTOPOIMP_HPP_INCLUDE

#include "geopm/PlatformTopo.hpp"

namespace geopm
{
    class AcceleratorTopo;

    class PlatformTopoImp : public PlatformTopo
    {
        public:
            PlatformTopoImp();
            PlatformTopoImp(const std::string &test_cache_file_name);
            PlatformTopoImp(const std::string &test_cache_file_name, const AcceleratorTopo &accelerator_topo);
            virtual ~PlatformTopoImp() = default;
            int num_domain(int domain_type) const override;
            int domain_idx(int domain_type,
                           int cpu_idx) const override;
            bool is_nested_domain(int inner_domain, int outer_domain) const override;
            std::set<int> domain_nested(int inner_domain, int outer_domain, int outer_idx) const override;
            static void create_cache();
            static void create_cache_service();
            static void create_cache(const std::string &cache_file_name);
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
            void parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
                                  std::vector<std::set<int> > &numa_map);
            std::string read_lscpu(void);
            static bool check_file(const std::string &file_name);

            const std::string M_TEST_CACHE_FILE_NAME;
            int m_num_package;
            int m_core_per_package;
            int m_thread_per_core;
            std::vector<std::set<int> > m_numa_map;
            const AcceleratorTopo &m_accelerator_topo;
    };
}
#endif
