/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWOD_HPP_INCLUDE
#define TENSORTWOD_HPP_INCLUDE

#include <cstdint>
#include <memory>
#include <vector>

#include "TensorOneD.hpp"

namespace geopm
{
    class TensorMath;

    ///  @brief Class to manage data and operations related to 2D Tensors
    ///         required for neural net inference.
    class TensorTwoD
    {
        public:
            TensorTwoD();
            /// @brief Constructor setting dimensions
            TensorTwoD(std::size_t rows, std::size_t cols);
            /// @brief Test constructor
            TensorTwoD(std::size_t rows, std::size_t cols, std::shared_ptr<TensorMath> math);
            /// @brief Copy constructor using a deep copy
            ///
            /// @param [in] TensorTwoD& 2D Tensor to copy
            TensorTwoD(const TensorTwoD&);

            TensorTwoD(TensorTwoD &&other);
            /// @brief Constructor input from a vector of vectors of values.
            ///
            /// @param [in] input std::vector<std::vector<float> > instance
            ///
            /// @throws geopm::Exception if input is not rectangular or
            /// if input is empty.
            TensorTwoD(std::vector<std::vector<float> > input);
            /// @brief Constructor input from a vector of vectors of values.
            ///
            /// @param [in] input std::vector<std::vector<float> > instance
            /// @param [in] math  TensorMath instance
            ///
            /// @throws geopm::Exception if input is not rectangular or
            /// if input is empty.
            TensorTwoD(std::vector<std::vector<float> > input, std::shared_ptr<TensorMath> math);
            /// @brief Constructor input from a vector of 1D tensors.
            ///
            /// @param [in] input std::vector<TensorOneD> instance
            ///
            /// @throws geopm::Exception if input is not rectangular or
            /// if input is empty.
            TensorTwoD(std::vector<TensorOneD> input);
            /// @brief Test constructor
            ///
            /// @param [in] input std::vector<TensorOneD> instance
            /// @param [in] math  TensorMath instance
            ///
            /// @throws geopm::Exception if input is not rectangular or
            /// if input is empty.
            TensorTwoD(std::vector<TensorOneD> input, std::shared_ptr<TensorMath> math);
            /// @brief Get number of rows in the 2D tensor
            ///
            /// @return Number of rows of the 2D tensor
            std::size_t get_rows() const;
            /// @brief get number of columns in 2D tensor
            ///
            /// @return Number of columns in the 2D tensor
            std::size_t get_cols() const;
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
            /// @brief Multiply a 2D tensor by a 1D tensor
            ///
            /// @param [in] TensorOneD& Reference to the multiplicand
            ///
            /// @throws geopm::Exception if the sizes are incompatible, i.e. if 2D tensor
            /// number of columns is unequal to 1D tensor number of rows
            TensorOneD operator*(const TensorOneD&) const;
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
            /// @brief Overload == operator to do comparison of the underlying
            //         data
            ///
            /// @param [in] other The tensor to compare against
            bool operator==(const TensorTwoD &other) const;

            /// @brief Return the tensor as a vector of tensors.
            ///
            /// @return Returns the contents of the tensor.
            const std::vector<TensorOneD> &get_data() const;

            /// @brief Set the contents as a vector of tensors.
            void set_data(const std::vector<TensorOneD>);

        private:
            std::vector<TensorOneD> m_mat;
            std::shared_ptr<TensorMath> m_math;
    };
}
#endif
