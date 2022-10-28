/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORONED_HPP_INCLUDE
#define TENSORONED_HPP_INCLUDE

#include "geopm/json11.hpp"
#include <cstddef>

namespace geopm
{
    /// @brief Class to store and perform operations on 1D Tensors,
    ///        aka vectors, suitable for use in feed-forward neural
    ///        networks.
    class TensorOneD
    {
        public:
            ///@brief Constructor
            TensorOneD();
            ///@brief Constructor with size specified 
            TensorOneD(std::size_t n);
            ///@brief Constructs a deep copy of the argument
            TensorOneD(const TensorOneD&);
            ///@brief Constructor inputting from external JSON
            TensorOneD(json11::Json input);
            ///@brief Set length of 1D tensor
            ///
            ///If the instance has more than n elements, it will
            ///be truncated. If it has fewer than n elements, the
            ///tensor will be expanded to a total size of n, uninitialized.
            void set_dim(std::size_t n);
            ///@brief Get the length of the 1D tensor
            std::size_t get_dim() const;
            ///@brief Add two 1D tensors
            TensorOneD operator+(const TensorOneD&);
            ///@brief Subtract two 1D tensors
            TensorOneD operator-(const TensorOneD&);
            ///@brief Multiply two 1D tensors
            float operator*(const TensorOneD&);
            ///@brief Overload = operator with a deep copy
            TensorOneD& operator=(const TensorOneD&);
            ///@brief Reference indexing of 1D Tensor value at idx
            float &operator[](std::size_t idx);
            ///@brief Value access of 1D Tensor value at idx.
            float operator[](std::size_t idx) const;
            ///@brief Compute logistic sigmoid function of 1D Tensor
            TensorOneD sigmoid() const;

        private:
            std::vector<float> m_vec;
    };
}
#endif
