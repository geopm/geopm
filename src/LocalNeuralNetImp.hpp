/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
            LocalNeuralNetImp(std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > input);
            std::vector<float> model(std::vector<float> inp);

        private:
            /// @brief Class to store and perform operations on 1D Tensors,
            ///        aka vectors, suitable for use in feed-forward neural
            ///        networks.
            class TensorOneD
            {
                public:
                    TensorOneD() = default;
                    /// @brief Constructor with size specified 
                    ///
                    /// @param [in] n Size of 1D tensor
                    TensorOneD(std::size_t n);
                    /// @brief Constructs a deep copy of the argument
                    ///
                    /// @param [in] TensorOneD Tensor to copy
                    TensorOneD(const TensorOneD&);

                    TensorOneD(TensorOneD &&other);
                    /// @param [in] input std::vector<float> input the data
                    ///
                    /// @throws geopm::Exception if input is empty.
                    TensorOneD(std::vector<float> input);
                    /// @brief Set length of 1D tensor
                    ///
                    /// If the instance has more than n elements, it will
                    /// be truncated. If it has fewer than n elements, the
                    /// tensor will be expanded to a total size of n, uninitialized.
                    ///
                    /// @param [in] n Resulting size of 1D tensor
                    void set_dim(std::size_t dim);
                    /// @brief Get the length of the 1D tensor
                    ///
                    /// @return Returns the length of the 1D tensor
                    std::size_t get_dim() const;
                    /// @brief Add two 1D tensors, element-wise
                    ///
                    /// The tensors need to be the same length. 
                    ///
                    /// @throws geopm::Exception if the lengths do not match.
                    ///
                    /// @param [in] other The summand
                    ///
                    /// @return Returns a 1D tensor, the sum of two 1D tensors
                    TensorOneD operator+(const TensorOneD& other);
                    /// @brief Subtract two 1D tensors, element-wise
                    ///
                    /// @throws geopm::Exception if the lengths do not match.
                    ///
                    /// @param [in] other The subtrahend
                    ///
                    /// @return A 1D tensor, the difference of two 1D tensors.
                    TensorOneD operator-(const TensorOneD& other);
                    /// @brief Multiply two 1D tensors, element-wise
                    ///
                    /// @throws geopm::Exception if the lengths do not match.
                    ///
                    /// @param [in] other The multiplicand
                    ///
                    /// @return Returns a 1D tensor, the product of two 1D tensors
                    float operator*(const TensorOneD& other);
                    /// @brief Overload = operator with an in-place deep copy
                    ///
                    /// @param [in] other The assignee (tensor to be copied)
                    TensorOneD& operator=(const TensorOneD& other);

                    TensorOneD& operator=(TensorOneD &&other);
                    /// @brief Reference indexing of 1D tensor value at idx
                    ///
                    /// @param [in] idx The index at which to look for the value
                    ///
                    /// @return Returns a reference to the 1D tensor at idx
                    float &operator[](std::size_t idx);
                    /// @brief Value access of 1D Tensor value at idx.
                    ///
                    /// @param [in] idx The index at which to look for the value
                    ///
                    /// @return Returns the value of the 1D tensor at idx
                    float operator[](std::size_t idx) const;
                    /// @brief Compute logistic sigmoid function of 1D Tensor
                    TensorOneD sigmoid() const;
                    /// TODO
                    std::vector<float> to_vector() const;

                private:
                    std::vector<float> m_vec;
            };

            class TensorTwoD
            {
                public:
                    TensorTwoD() = default;
                    /// @brief Constructor setting dimensions
                    TensorTwoD(std::size_t rows, std::size_t cols);
                    /// @brief Copy constructor using a deep copy
                    /// 
                    /// @param [in] TensorTwoD& 2D Tensor to copy
                    TensorTwoD(const TensorTwoD&);
                    /// @brief Constructor input from a vector of vectors of values.
                    /// 
                    /// @param [in] input std::vector<std::vector<float> > instance
                    /// 
                    /// @throws geopm::Exception if input is not rectangular or
                    /// if input is empty.
                    TensorTwoD(std::vector<std::vector<float> > input);
                    /// @brief Set dimensions of 2D tensor
                    /// 
                    /// @param [in] rows The number of "rows" or 1D tensors
                    /// @param [in] cols The number of "cols" or the size of each 1D tensor
                    /// 
                    /// If the instance contains more than \p rows 1D tensors,
                    /// it will be truncated. If the instance contains fewer
                    /// than \p rows 1D tensors, it will be expanded to a total
                    /// number of rows, uninitialized. The individual 1D tensors
                    /// will be managed similarly. 
                    /// 
                    /// @throws geopm::Exception if \p rows = 0 and \p cols > 0
                    void set_dim(std::size_t rows, std::size_t cols);
                    /// @brief Get number of rows in the 2D tensor
                    /// 
                    /// @return Number of rows of the 2D tensor
                    std::size_t get_rows() const;
                    /// @brief get number of columns in 2D tensor
                    /// 
                    /// @return Number of columns in the 2D tensor
                    std::size_t get_cols() const;
                    /// @brief Multiply a 2D tensor by a 1D tensor
                    /// 
                    /// @param [in] TensorOneD& Reference to the multiplicand
                    /// 
                    /// @throws geopm::Exception if the sizes are incompatible, i.e. if 2D tensor
                    /// number of columns is unequal to 1D tensor number of rows
                    TensorOneD operator*(const TensorOneD&);
                    /// @brief Reference indexing of 1D Tensor at idx of the 2D Tensor
                    /// 
                    /// @param [in] idx The index at which to look for the value
                    /// 
                    /// @return Returns a reference to the 1D Tensor at idx
                    TensorOneD &operator[](size_t idx);
                    /// @brief Value access of 1D Tensor value at idx
                    /// 
                    /// @pram [in] idx The index at which to look for the value
                    /// 
                    /// @return Returns the values of 1D Tensors at idx
                    TensorOneD operator[](size_t idx) const;
                    /// @brief Oerload = operator with an in-place deep copy
                    /// 
                    /// @param [in] TensorTwoD& Reference to the 2D Tensor to copy
                    TensorTwoD& operator=(const TensorTwoD&);

                private:
                    std::vector<TensorOneD> m_mat;
            };

            std::vector<std::pair<TensorTwoD, LocalNeuralNetImp::TensorOneD> > m_layers;
    };
}

#endif  /* LOCALNEURALNETIMP_HPP_INCLUDE */
