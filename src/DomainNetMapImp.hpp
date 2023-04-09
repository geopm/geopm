/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOMAINNETMAPIMP_HPP_INCLUDE
#define DOMAINNETMAPIMP_HPP_INCLUDE

#include "DomainNetMap.hpp"

#include <memory>

#include "geopm/json11.hpp"
#include "geopm/PlatformIO.hpp"

#include "DenseLayer.hpp"
#include "LocalNeuralNet.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"
#include "NNFactory.hpp"

namespace geopm
{
    class DomainNetMapImp : public DomainNetMap
    {
        public:
            DomainNetMapImp(const std::string nn_path, geopm_domain_e domain_type, int domain_index);

            DomainNetMapImp(const std::string nn_path, geopm_domain_e domain_type, int domain_index, PlatformIO &plat_io, std::shared_ptr<NNFactory> nn_factory);

            void sample() override;
            std::vector<std::string> trace_names() const override;
            std::vector<double> trace_values() const override;
            std::map<std::string, double> last_output() const override;

        private:
            std::shared_ptr<DenseLayer> json_to_DenseLayer(const json11::Json &obj) const;
            TensorOneD json_to_TensorOneD(const json11::Json &obj) const;
            TensorTwoD json_to_TensorTwoD(const json11::Json &obj) const;

            PlatformIO &m_platform_io;
            std::shared_ptr<NNFactory> m_nn_factory;

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

            static constexpr int m_max_nnet_size = 1024;
            std::shared_ptr<LocalNeuralNet> m_neural_net;

            TensorOneD m_last_output;
            std::vector<signal> m_signal_inputs;
            std::vector<delta_signal> m_delta_inputs;
            std::vector<std::string> m_trace_outputs;
    };
}

#endif /* DOMAINNETMAPIMP_HPP_INCLUDE */
