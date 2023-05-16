/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOMAINNETMAPIMP_HPP_INCLUDE
#define DOMAINNETMAPIMP_HPP_INCLUDE

#include "DomainNetMap.hpp"

#include <memory>
#include <set>

#include "geopm/json11.hpp"

#include "DenseLayer.hpp"
#include "LocalNeuralNet.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"
#include "NNFactory.hpp"

namespace geopm
{
    class PlatformIO;

    class DomainNetMapImp : public DomainNetMap
    {
        public:
            DomainNetMapImp(const std::string &nn_path, geopm_domain_e domain_type,
                            int domain_index);

            DomainNetMapImp(const std::string &nn_path, geopm_domain_e domain_type,
                            int domain_index, PlatformIO &plat_io,
                            std::shared_ptr<NNFactory> nn_factory);

            void sample() override;
            /// @brief Generates the names for trace columns from the appropriate field in the neural net.
            //         In this case, region classification names annotated with domain type and index. 
            std::vector<std::string> trace_names() const override;
            /// @brief Populates trace values from last_output for each index within each domain type.
            ///        In this case, region classification logits. 
            /// @return Returns a vector of doubles containing trace values
            std::vector<double> trace_values() const override;
            /// @brief Populates a map of trace names to the latest output from the neural net.
            ///        In this case, region classification names to their respective logits.
            ///
            /// @return A map of string, double containing region class and logits.
            std::map<std::string, double> last_output() const override;

        private:
            std::shared_ptr<DenseLayer> json_to_DenseLayer(const json11::Json &obj) const;
            TensorOneD json_to_TensorOneD(const json11::Json &obj) const;
            TensorTwoD json_to_TensorTwoD(const json11::Json &obj) const;

            PlatformIO &m_platform_io;
            std::shared_ptr<NNFactory> m_nn_factory;

            struct m_signal_s
            {
                int batch_idx;
                double signal;
            };

            struct m_delta_signal_s
            {
                int batch_idx_num;
                int batch_idx_den;
                double signal_num;
                double signal_den;
                double signal_num_last;
                double signal_den_last;
            };

            static const std::set<std::string> M_EXPECTED_KEYS;
            // Size in bytes
            static constexpr int M_MAX_NNET_SIZE = 1024 * 1024;
            std::shared_ptr<LocalNeuralNet> m_neural_net;

            TensorOneD m_last_output;
            std::vector<m_signal_s> m_signal_inputs;
            std::vector<m_delta_signal_s> m_delta_inputs;
            std::vector<std::string> m_trace_outputs;
    };
}

#endif /* DOMAINNETMAPIMP_HPP_INCLUDE */
