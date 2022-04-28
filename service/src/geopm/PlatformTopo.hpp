/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef PLATFORMTOPO_HPP_INCLUDE
#define PLATFORMTOPO_HPP_INCLUDE

#include <vector>
#include <set>
#include <map>
#include <string>

#include "geopm_topo.h"

extern "C"
{
    /// @brief Identify host CPU.
    int geopm_read_cpuid(void);
}

namespace geopm
{
    class PlatformTopo
    {
        public:
            PlatformTopo() = default;
            virtual ~PlatformTopo() = default;
            /// @brief Number of domains on the platform of a
            ///        particular geopm_domain_e type.
            virtual int num_domain(int domain_type) const = 0;
            /// @brief Get the domain index for a particular domain
            ///        type that contains the given Linux logical CPU
            ///        index
            virtual int domain_idx(int domain_type,
                                   int cpu_idx) const = 0;
            /// @brief Check if one domain type is contained in another.
            ///
            /// @param [in] inner_domain The contained domain type.
            ///
            /// @param [in] outer_domain The containing domain type.
            ///
            /// @return True if the inner_domain is contained within
            ///         the outer_domain.
            virtual bool is_nested_domain(int inner_domain, int outer_domain) const = 0;
            /// @brief Get the set of smaller domains contained in a larger one.
            ///        If the inner domain is not the same as or contained within
            ///        the outer domain, it throws an error.
            ///
            /// @param [in] inner_domain The contained domain type.
            ///
            /// @param [in] outer_domain The containing domain type.
            ///
            /// @param [in] outer_idx The containing domain index.
            ///
            /// @return The set of domain indices for the inner domain that are
            ///         within the indexed outer domain.
            virtual std::set<int> domain_nested(int inner_domain, int outer_domain, int outer_idx) const = 0;
            /// @brief Convert a domain type enum to a string.
            ///
            /// @details These strings are used by the geopmread and geopmwrite tools.
            ///
            /// @param [in] domain_type Domain type from the
            ///        geopm_domain_e enum.
            ///
            /// @return Domain name which is the enum name in
            ///         lowercase with GEOPM_DOMAIN_ prefix removed.
            static std::string domain_type_to_name(int domain_type);
            /// @brief Convert a domain name to its corresponding
            ///        enum.
            ///
            /// @param [in] name Domain name which is the enum
            ///        in lowercase with GEOPM_DOMAIN_ prefix removed.
            ///
            /// @return Domain type from the geopm_domain_e enum.
            static int domain_name_to_type(const std::string &domain_name);
            /// @brief Create cache file in tmpfs that can be read
            ///        instead of popen() call.
            static void create_cache(void);
        private:
            static std::vector<std::string> domain_names(void);
            static std::map<std::string, int> domain_types(void);
    };

    const PlatformTopo &platform_topo(void);
}
#endif
