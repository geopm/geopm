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
            TensorOneD() = delete;
            ///@brief Constructor with size specified 
            ///
            ///@param [in] n Size of 1D tensor
            TensorOneD(std::size_t n);
            ///@brief Constructs a deep copy of the argument
            ///
            ///@param [in] TensorOneD Tensor to copy
            TensorOneD(const TensorOneD&);

            TensorOneD(TensorOneD &&other);
            ///@brief Constructor inputting from external JSON
            ///
            ///This function expects a Json array.
            ///
            ///@param [in] input json11::Json instance
            TensorOneD(json11::Json input);
            ///@brief Set length of 1D tensor
            ///
            ///If the instance has more than n elements, it will
            ///be truncated. If it has fewer than n elements, the
            ///tensor will be expanded to a total size of n, uninitialized.
            ///A geopm::Exception will be rasied if input is not an array,
            ///if input is empty, or if a non-numeric type is found in input.
            ///
            ///@param [in] n Resulting size of 1D tensor
            void set_dim(std::size_t n);
            ///@brief Get the length of the 1D tensor
            ///
            ///@return Returns the length of the 1D tensor
            std::size_t get_dim() const;
            ///@brief Add two 1D tensors, element-wise
            ///
            ///The tensors need to be the same length. A geopm::Exception
            ///will be raised if the lengths do not match.
            ///
            ///@param [in] other The summand
            ///
            ///@return Returns a 1D tensor, the sum of two 1D tensors
            TensorOneD operator+(const TensorOneD& other);
            ///@brief Subtract two 1D tensors, element-wise
            ///
            ///The tensors need to be the same length. A geopm::Exception
            ///will be raised if the lengths do not match.
            ///
            ///@param [in] other The subtrahend
            ///
            ///@return Returns a 1D tensor, the difference of two 1D tensors
            TensorOneD operator-(const TensorOneD& other);
            ///@brief Multiply two 1D tensors, element-wise
            ///
            ///The tensors need to be the same length. A geopm::Exception
            ///will be raised if the lengths do not match.
            ///
            ///@param [in] other The multiplicand
            ///
            ///@return Returns a 1D tensor, the product of two 1D tensors
            float operator*(const TensorOneD& other);
            ///@brief Overload = operator with an in-place deep copy
            ///
            ///@param [in] other The assignee (tensor to be copied)
            TensorOneD& operator=(const TensorOneD& other);

            TensorOneD& operator=(TensorOneD &&other);
            ///@brief Reference indexing of 1D tensor value at idx
            ///
            ///@param [in] idx The index at which to look for the value
            ///
            ///@return Returns a reference to the 1D tensor at idx
            float &operator[](std::size_t idx);
            ///@brief Value access of 1D Tensor value at idx.
            ///
            ///@param [in] idx The index at which to look for the value
            ///
            ///@return Returns the value of the 1D tensor at idx
            float operator[](std::size_t idx) const;
            ///@brief Compute logistic sigmoid function of 1D Tensor
            TensorOneD sigmoid() const;

        private:
            std::vector<float> m_vec;
    };
}
#endif
