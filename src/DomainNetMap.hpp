/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOMAINNETMAP_HPP_INCLUDE
#define DOMAINNETMAP_HPP_INCLUDE

#include <memory>
#include <vector>
#include <map>

#include "geopm_topo.h"

namespace geopm
{

    /// @brief Class to load neural net from file, sample signals specified in 
    ///        that file, feed those signals into the neural net and manage 
    ///        the output from the neural nets.
    class DomainNetMap
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object constructed
            ///        using the underlying implementation which loads neural
            ///        net for a specified domain from a json file.
            ///
            /// @param [in] nn_path Path to neural net json
            ///
            /// @param [in] domain_type Domain type, defined by geopm_domain_e enum 
            ///
            /// @param [in] domain_index Index of the domain to be measured
            ///
            /// @throws geopm::Exception if unable to open neural net file
            ///
            /// @throws geopm::Exception if neural net file exceeds max size  (currently 1024)
            ///
            /// @throws geopm::Exception if neural net file does not contain a key "layers"
            ///         or if "layers" does not contain an array
            ///
            /// @throws geopm::Exception if neural net file does not contain either a non-empty 
            ///         array "signal_inputs" or a non-empty array "delta_inputs"
            static std::unique_ptr<DomainNetMap> make_unique(const std::string &nn_path,
                                                             geopm_domain_e domain_type,
                                                             int domain_index);
            /// @brief Returns a shared_ptr to a concrete object constructed
            ///        using the underlying implementation which loads neural
            ///        net for a specified domain from a json file.
            ///
            /// @param [in] nn_path Path to neural net json
            ///
            /// @param [in] domain_type Domain type, defined by geopm_domain_e enum 
            ///
            /// @param [in] domain_index Index of the domain to be measured
            ///
            /// @throws geopm::Exception if unable to open neural net file
            ///
            /// @throws geopm::Exception if neural net file exceeds max size  (currently 1024)
            ///
            /// @throws geopm::Exception if neural net file does not contain a key "layers"
            ///         or if "layers" does not contain an array
            ///
            /// @throws geopm::Exception if neural net file does not contain either a non-empty 
            ///         array "signal_inputs" or a non-empty array "delta_inputs"
            static std::shared_ptr<DomainNetMap> make_shared(const std::string &nn_path,
                                                             geopm_domain_e domain_type,
                                                             int domain_index);

            virtual ~DomainNetMap() = default;
            /// @brief Samples latest signals for a specific domain and applies the 
            ///        resulting TensorOneD state to the neural net.
            virtual void sample() = 0;
            /// @brief generates the names for trace columns from the appropriate field in the neural net
            virtual std::vector<std::string> trace_names() const = 0;
            /// @brief Populates trace values from last_output for each index within each domain type
            ///
            /// @return Returns a vector of doubles containing trace values
            virtual std::vector<double> trace_values() const = 0;
            /// @brief Populates a map of trace names to the latest output from the neural net.
            ///
            /// @return A map of string, double containing trace names and latest nn output
            virtual std::map<std::string, double> last_output() const = 0;
    };
}

#endif  /* DOMAINNETMAP_HPP_INCLUDE */
