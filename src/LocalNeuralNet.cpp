/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"
#include "DenseLayer.hpp"
#include "LocalNeuralNet.hpp"
#include "LocalNeuralNetImp.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    std::unique_ptr<LocalNeuralNet> LocalNeuralNet::make_unique(std::vector<std::shared_ptr<DenseLayer> > layers)
    {
        return geopm::make_unique<LocalNeuralNetImp>(layers);
    }

    LocalNeuralNetImp::LocalNeuralNetImp(std::vector<std::shared_ptr<DenseLayer> > layers)
    {
        for (std::size_t idx=1; idx < layers.size(); ++idx) {
            if (layers[idx]->get_input_dim() != layers[idx-1]->get_output_dim()) {
                throw geopm::Exception("Incompatible dimensions for consecutive layers.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        m_layers = layers;
    }

    TensorOneD LocalNeuralNetImp::forward(const TensorOneD &inp) const
    {
        if (inp.get_dim() != m_layers[0]->get_input_dim()) {
            throw geopm::Exception("Input vector dimension is incompatible with network.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorOneD tmp = inp;

        for (std::size_t idx = 0; idx < m_layers.size(); ++idx) {
            tmp = m_layers[idx]->forward(tmp);

            // Apply a sigmoid on all but the last layer
            if (idx != m_layers.size() - 1) {
                tmp = tmp.sigmoid();
            }
        }

        return tmp;
    }
}
