/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
            ///        particular m_domain_e type.
            virtual int num_domain(int domain_type) const = 0;
            /// @brief Get the domain index for a particular domain
            ///        type that contains the given Linux logical CPU
            ///        index
            virtual int domain_idx(int domain_type,
                                   int cpu_idx) const = 0;
            /// @brief Check if one domain type is contained in another.
            /// @param [in] inner_domain The contained domain type.
            /// @param [in] outer_domain The containing domain type.
            /// @return True if the inner_domain is contained within
            ///         the outer_domain.
            virtual bool is_nested_domain(int inner_domain, int outer_domain) const = 0;
            /// @brief Get the set of smaller domains contained in a larger one.
            ///        If the inner domain is not the same as or contained within
            ///        the outer domain, it throws an error.
            /// @param [in] inner_domain The contained domain type.
            /// @param [in] outer_domain The containing domain type.
            /// @param [in] outer_idx The containing domain index.
            /// @return The set of domain indices for the inner domain that are
            ///         within the indexed outer domain.
            virtual std::set<int> domain_nested(int inner_domain, int outer_domain, int outer_idx) const = 0;
            /// @brief  Get the domain index of domain type outer_domain
            //          that contains inner_domain_idx of type inner_domain
            //          if inner_domain is nested within outer_domain.
            /// @param [in] inner_domain The contained domain type.
            /// @param [in] innerr_idx The contained domain index.
            /// @param [in] outer_domain The containing domain type.
            /// @return The outer domain index that contains the inner_domain_idx
            //          or -1 when the inner_domain is not within outer_domain.
            virtual int get_outer_domain_idx(int inner_domain, int inner_domain_idx, int outer_domain) const = 0;
            /// @brief Convert a domain type enum to a string.
            /// @param [in] domain_type Domain type from the
            ///        m_domain_e enum.
            /// @return Domain name which is the enum name in
            ///         lowercase with M_DOMAIN_ prefix removed.
            static std::string domain_type_to_name(int domain_type);
            /// @brief Convert a domain name to its corresponding
            ///        enum.
            /// @param [in] name Domain name which is the enum
            ///        in lowercase with M_DOMAIN_ prefix removed.
            /// @return Domain type from the m_domain_e enum.
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
