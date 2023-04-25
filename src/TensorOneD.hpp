/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORONED_HPP_INCLUDE
#define TENSORONED_HPP_INCLUDE

#include <cstdio>
#include <memory>
#include <vector>

namespace geopm
{
    class TensorMath;

    /// @brief Class to store and perform operations on 1D Tensors,
    ///        aka vectors, suitable for use in feed-forward neural
    ///        networks.
    class TensorOneD
    {
        public:
            TensorOneD();
            /// @brief Constructor with size specified 
            ///
            /// @param [in] n Size of 1D tensor
            TensorOneD(size_t n);
            /// @brief Constructs a deep copy of the argument
            ///
            /// @param [in] TensorOneD Tensor to copy
            TensorOneD(const TensorOneD &);

            TensorOneD(TensorOneD &&other);
            /// @brief Constructor from vector of values
            ///
            /// @param [in] input  The values to store in the 1D tensor.
            ///
            /// @throws geopm::Exception if input is empty.
            TensorOneD(const std::vector<double> &input);

            /// @brief Test constructor
            ///
            /// @param [in] input  The values to store in the 1D tensor.
            /// @param [in] math  TensorMath instance
            ///
            /// @throws geopm::Exception if input is empty.
            TensorOneD(const std::vector<double> &input, std::shared_ptr<TensorMath> math);

            /// @brief Set length of 1D tensor
            ///
            /// If the instance has more than n elements, it will
            /// be truncated. If it has fewer than n elements, the
            /// tensor will be expanded to a total size of n, uninitialized.
            ///
            /// @param [in] n Resulting size of 1D tensor
            void set_dim(size_t dim);
            /// @brief Get the length of the 1D tensor
            ///
            /// @return Returns the length of the 1D tensor
            size_t get_dim() const;

            TensorOneD operator+(const TensorOneD &other) const;
            TensorOneD operator-(const TensorOneD &other) const;
            double operator*(const TensorOneD &other) const;
            /// @brief Overload = operator with an in-place deep copy
            ///
            /// @param [in] other The assignee (tensor to be copied)
            TensorOneD& operator=(const TensorOneD &other);

            TensorOneD& operator=(TensorOneD &&other);

            /// @brief Overload == operator to do comparison of the underlying
            //         data
            ///
            /// @param [in] other The tensor to compare against
            bool operator==(const TensorOneD &other) const;

            /// @brief Reference indexing of 1D tensor value at idx
            ///
            /// @param [in] idx The index at which to look for the value
            ///
            /// @return Returns a reference to the 1D tensor at idx
            double &operator[](size_t idx);
            /// @brief Value access of 1D Tensor value at idx.
            ///
            /// @param [in] idx The index at which to look for the value
            ///
            /// @return Returns the value of the 1D tensor at idx
            double operator[](size_t idx) const;

            TensorOneD sigmoid() const;

            /// @brief Return the tensor as a vector of doubles.
            ///
            /// @return Returns the contents of the tensor.
            const std::vector<double> &get_data() const;

	    virtual ~TensorOneD() = default;
        private:
            std::vector<double> m_vec;
            std::shared_ptr<TensorMath> m_math;
    };
}
#endif /* TENSORONED_HPP_INCLUDE */
