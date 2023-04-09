/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNETIMP_HPP_INCLUDE
#define LOCALNEURALNETIMP_HPP_INCLUDE

#include "LocalNeuralNet.hpp"

namespace geopm
{
    class LocalNeuralNetImp : public LocalNeuralNet
    {
        public:
            /// @brief Constructor input from a vector of DenseLayers
            ///
            /// @param [in] input layers
            ///
            /// @throws geopm::Exception if consecutive layer sizes are
            /// incompatible
            LocalNeuralNetImp(const std::vector<std::shared_ptr<DenseLayer> > layers);
            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] TensorOneD vector of input signals.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            /// with network.
            ///
            /// @return Returns a TensorOneD vector of output values.
            TensorOneD forward(const TensorOneD &inp) const override;

        private:
            std::vector<std::shared_ptr<DenseLayer> > m_layers;
    };
}

#endif  /* LOCALNEURALNETIMP_HPP_INCLUDE */
