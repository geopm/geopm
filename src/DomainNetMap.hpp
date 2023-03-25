/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOMAINNETMAP_HPP_INCLUDE
#define DOMAINNETMAP_HPP_INCLUDE

#include "geopm/json11.hpp"
#include "geopm/PlatformTopo.hpp"

#include "DenseLayer.hpp"
#include "LocalNeuralNet.hpp"

#include <memory>

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
    /// @brief Class to load neural net from file, sample signals specified in 
    ///        that file, feed those signals into the neural net, and manage 
    ///        the output from the neural nets, i.e. the probabilities of each 
    ///        region class for each domain.
    class DomainNetMap
    {
        public:
            DomainNetMap(geopm::PlatformIO &plat_io);
            /// @brief Constructor with size specified 
            ///
            /// @param [in] n Size of 1D tensor
            virtual ~DomainNetMap() = default;
            /// @brief Loads neural net for a specified domain from a json11 instance.
            ///
            /// @throws geopm::Exception if unable to open neural net file
            ///
            /// @throws geopm::Exception if neural net file exceeds max size  (currently 1024)
            ///
            /// @throws geopm::Exception if neural net file does not contain 
            ///         expected keys or arrays
            ///
            /// @param [in] nn_path Path to neural net
            /// @param [in] domain_type Domain type, defined by geopm_domain_e enum 
            /// @param [in] domain_index Index of the domain to be measured
            void load_neural_net(char* nn_path, geopm_domain_e domain_type, int domain_index);
            /// @brief Collects latest signals for a specific domain and applies the 
            ///        resulting TensorOneD state to the neural net.
            void sample();
            /// @brief generates the names for trace columns from the appropriate field in the neural net
            std::vector<std::string> trace_names() const;
            /// @brief Populates trace values from last_output for each index within each domain type
            ///
            /// @return A vector of doubles containing trace values
            std::vector<double> trace_values() const;
            /// @brief Populates a map of region class name to probability for a given domain, 
            ///        index from the latest evaluation of the neural net.
            ///
            /// @param [in] domain_index
            ///
            /// @return A map of string, float containing region class and probabilities
            std::map<std::string, float> last_output() const;
        private:
            std::shared_ptr<DenseLayer> json_to_DenseLayer(const json11::Json &obj) const;
            TensorOneD json_to_TensorOneD(const json11::Json &obj) const;
            TensorTwoD json_to_TensorTwoD(const json11::Json &obj) const;

            geopm::PlatformIO &m_platform_io;

            struct signal
            {
                int batch_idx;
                double signal;
            };

            struct delta_signal
            {
                int batch_idx_num;
                int batch_idx_den;
                double signal_num;
                double signal_den;
                double signal_num_last;
                double signal_den_last;
            };

            struct trace_output
            {
                std::string trace_name;
                double value;
            };

            int m_count;
            static constexpr int m_max_nnet_size = 1024;
            std::unique_ptr<LocalNeuralNet> m_neural_net;

            TensorOneD m_last_output;
            std::vector<signal> m_signal_inputs;
            std::vector<delta_signal> m_delta_inputs;
            std::vector<std::string> m_trace_outputs;
    };
}

#endif  /* DOMAINNETMAP_HPP_INCLUDE */
