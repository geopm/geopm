/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNET_HPP_INCLUDE
#define LOCALNEURALNET_HPP_INCLUDE

#include <memory>
#include <vector>

namespace geopm
{
    ///  @brief Class to manage data and operations of feed forward neural nets
    ///         required for neural net inference.

    class LocalNeuralNet
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            ///        from an array of doubles of weights and biases.
            /// 
            /// @param [in] vector of pairs of weight matrices (as
            ///             double vectors of floats) and bias vectors
            ///             as vectors of floats.
            /// 
            /// @throws geopm::Exception if dimensions are incompatible.
            static std::unique_ptr<LocalNeuralNet> make_unique(std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > input);
            /// @brief Default destructor of pure virtual base class.
            virtual ~LocalNeuralNet() = default;

            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] std::vector<float> of input signals.
            ///
            /// @return Returns a std::vector<float> of output values.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            /// with network.
            virtual std::vector<float> model(std::vector<float> inp) = 0;
        protected:
            LocalNeuralNet() = default;
    };
}

#endif  /* LOCALNEURALNET_HPP_INCLUDE */
