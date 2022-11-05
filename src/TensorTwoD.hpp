/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWOD_HPP_INCLUDE
#define TENSORTWOD_HPP_INCLUDE

#include <cstddef>

#include "geopm/json11.hpp"

#include "TensorOneD.hpp"

namespace geopm
{
    ///  @brief Class to manage data and operations related to 2D Tensors
    ///         required for neural net inference.  

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
            /// @brief Constructor input from external json, as
            ///        an array of arrays of values.
            /// 
            /// @param [in] input json11::Json instance
            /// 
            /// @throws geopm::Exception if input is not an array of arrays,
            /// if input is empty, or if a non-numeric type is found in input.
            TensorTwoD(json11::Json input);
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
            /// @brief Multipy a 2D tensor by a 1D tensor
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
            TensorOneD &operator[](int idx);
            /// @brief Value access of 1D Tensor value at idx
            /// 
            /// @pram [in] idx The index at which to look for the value
            /// 
            /// @return Returns the values of 1D Tensors at idx
            TensorOneD operator[](int idx) const;
            /// @brief Oerload = operator with an in-place deep copy
            /// 
            /// @param [in] TensorTwoD& Reference to the 2D Tensor to copy
            TensorTwoD& operator=(const TensorTwoD&);

        private:
            std::vector<TensorOneD> m_mat;
    };
}
#endif
