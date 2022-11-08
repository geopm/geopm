/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNET_HPP_INCLUDE
#define LOCALNEURALNET_HPP_INCLUDE

#include "geopm/json11.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"


namespace geopm
{
    ///  @brief Class to manage data and operations of feed forward neural nets
    ///         required for neural net inference.

    class LocalNeuralNet
    {
        public:
            LocalNeuralNet() = default;
            /// @brief Copy constructor using a deep copy
            /// 
            /// @param [in] LocalNeuralNet& Network to copy
            LocalNeuralNet(const LocalNeuralNet&);
            /// @brief Constructor input from external json, as
            ///        an array of doubles of weights and biases.
            /// 
            /// @param [in] input json11::Json instance
            /// 
            /// @throws geopm::Exception if input structure is incorrect,
            /// if dimensions are incompatible, or if included structures
            /// cannot be parsed.
            LocalNeuralNet(const json11::Json input);
            /// @brief Overload = operator with an in-place deep copy
            /// 
            /// @param [in] LocalNeuralNet& Reference to the network to copy
            LocalNeuralNet& operator=(const LocalNeuralNet&);
            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] TensorOneD vector of input signals.
            ///
            /// @return Returns a TensorOneD vector of output values.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            /// with network.
            TensorOneD model(TensorOneD inp);

        private:
            std::vector<std::pair<TensorTwoD, TensorOneD> > m_layers;
    };
}

#endif  /* LOCALNEURALNET_HPP_INCLUDE */
