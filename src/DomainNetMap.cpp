/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DomainNetMap.hpp"

#include <cmath>
#include <fstream>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "DenseLayerImp.hpp"
#include "LocalNeuralNetImp.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

namespace geopm
{
    DomainNetMap::DomainNetMap(geopm::PlatformIO &plat_io, const int count)
        : m_platform_io(plat_io)
          , m_count(count)
          , m_last_output(m_count)
          , m_signal_inputs(m_count)
          , m_delta_inputs(m_count)
    {
    }

    std::shared_ptr<DenseLayer> DomainNetMap::json_to_DenseLayer(const json11::Json &obj) const {
        if (!obj.is_array()) {
            throw geopm::Exception("Neural network weights contains non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (obj.array_items().size() != 2) {
            throw geopm::Exception("Dense Layer weights must be an array of length exactly two.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorTwoD weights = json_to_TensorTwoD(obj[0]);
        TensorOneD biases = json_to_TensorOneD(obj[1]);
        return std::make_shared<DenseLayerImp>(weights, biases);
    }

    TensorOneD DomainNetMap::json_to_TensorOneD(const json11::Json &obj) const {
        if (!obj.is_array()) {
            throw geopm::Exception("Neural network weights contains non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (obj.array_items().empty()) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::size_t vec_size = obj.array_items().size();
        std::vector<float> vals(vec_size);

        for (std::size_t idx = 0; idx < vec_size; ++idx) {
            if (!obj[idx].is_number()) {
                throw geopm::Exception("Non-numeric type found in neural network weights.\n",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            vals[idx] = obj[idx].number_value();
        }

        return TensorOneD(vals);
    }

    TensorTwoD DomainNetMap::json_to_TensorTwoD(const json11::Json &obj) const {
        if (!obj.is_array()){
            throw geopm::Exception("Neural network weights is non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (obj.array_items().size() == 0) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<std::vector<float> > vals;
        for (std::size_t ridx = 0; ridx < obj.array_items().size(); ++ridx) {
            if(!obj[ridx].is_array()) {
                throw geopm::Exception("Neural network weights is non-array-type.\n",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            std::size_t vec_size = obj[ridx].array_items().size();
            std::vector<float> row(vec_size);

            for (std::size_t cidx = 0; cidx < vec_size; ++cidx) {
                if (!obj[ridx][cidx].is_number()) {
                    throw geopm::Exception("Non-numeric type found in neural network weights.\n",
                                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                row[cidx] = obj[ridx][cidx].number_value();
            }

            vals.push_back(row);
        }

        return TensorTwoD(vals);
    }

    void DomainNetMap::load_neural_net(char* nn_path, int domain_type)
    {
        std::ifstream file(nn_path);

        if (!file) {
            throw geopm::Exception("Unable to open neural net file.\n",
                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        if (length >= m_max_nnet_size) {
            throw geopm::Exception("Neural net file exceeds maximum size.\n",
                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string buf, err;
        buf.reserve(length);

        std::getline(file, buf, '\0');

        const json11::Json nnet_json = json11::Json::parse(buf, err);

        if (! nnet_json["layers"].is_array()) {
            throw geopm::Exception(
                    "Neural net json must have a key \"layers\" whose value "
                    "is an array.\n",
                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (nnet_json["signal_inputs"].array_items().size() == 0 
            && nnet_json["delta_inputs"].array_items().size() == 0) {
            throw geopm::Exception(
                    "Neural net json must contain at least one of "
                    "\"signal_inputs\" and \"delta_inputs\" whose "
                    "value is a non-empty array.\n",
                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        
        std::vector<std::shared_ptr<DenseLayer> > layers;
        for (std::size_t layer_idx = 0; layer_idx < nnet_json["layers"].array_items().size(); ++layer_idx) {
            layers.push_back(json_to_DenseLayer(nnet_json["layers"][layer_idx]));
        }

        m_neural_net = geopm::make_unique<LocalNeuralNetImp>(layers);

        for (int domain_idx = 0; domain_idx < m_count; ++domain_idx) {
            for (std::size_t i=0; i<nnet_json["signal_inputs"].array_items().size(); i++) {
                m_signal_inputs[domain_idx].push_back({
                        m_platform_io.push_signal(
                                nnet_json["signal_inputs"][i][0].string_value(),
                                domain_type,
                                domain_idx),
                        NAN});
            }

            for (std::size_t i=0; i<nnet_json["delta_inputs"].array_items().size(); i++) {
                m_delta_inputs[domain_idx].push_back({
                        m_platform_io.push_signal(
                                nnet_json["delta_inputs"][i][0][0].string_value(),
                                domain_type,
                                domain_idx),
                        m_platform_io.push_signal(
                                nnet_json["delta_inputs"][i][1][0].string_value(),
                                domain_type,
                                domain_idx),
                        NAN, NAN, NAN, NAN});
            }
        }

        for (std::size_t i=0; i<nnet_json["trace_outputs"].array_items().size(); i++) {
            m_trace_outputs.push_back(nnet_json["trace_outputs"][i].string_value());
        }
    }

    void DomainNetMap::sample()
    {
        for (int idx=0; idx<m_count; ++idx) {
            sample_one(idx);
        }
    }

    void DomainNetMap::sample_one(int domain_index)
    {
        TensorOneD xs(m_signal_inputs[domain_index].size() + m_delta_inputs[domain_index].size());

        // Collect latest signal values
        for (std::size_t i=0; i<m_signal_inputs[domain_index].size(); i++) {
            m_signal_inputs[domain_index][i].signal = m_platform_io.sample(m_signal_inputs[domain_index][i].batch_idx);
            xs[i] = m_platform_io.sample(m_signal_inputs[domain_index][i].batch_idx);
        }
        for (std::size_t i=0; i<m_delta_inputs[domain_index].size(); i++) {
            m_delta_inputs[domain_index][i].signal_num_last = m_delta_inputs[domain_index][i].signal_num;
            m_delta_inputs[domain_index][i].signal_den_last = m_delta_inputs[domain_index][i].signal_den;
            m_delta_inputs[domain_index][i].signal_num = m_platform_io.sample(m_delta_inputs[domain_index][i].batch_idx_num);
            m_delta_inputs[domain_index][i].signal_den = m_platform_io.sample(m_delta_inputs[domain_index][i].batch_idx_den);
            xs[m_signal_inputs[domain_index].size() + i] =
                (m_delta_inputs[domain_index][i].signal_num - m_delta_inputs[domain_index][i].signal_num_last) /
                (m_delta_inputs[domain_index][i].signal_den - m_delta_inputs[domain_index][i].signal_den_last);
        }

        m_last_output[domain_index] = m_neural_net->forward(xs);
    }

    std::vector<std::string> DomainNetMap::trace_names(std::string domain) const
    {
        std::vector<std::string> tracelist;
        for (int domain_index=0; domain_index<m_count; ++domain_index) {
            for (std::string col: m_trace_outputs) {
                tracelist.push_back(col + "_" + domain + "_" + std::to_string(domain_index));
            }
        }
        return tracelist;
    }

    std::vector<double> DomainNetMap::trace_values() const
    {
        std::vector<double> rval;

        for (int domain_index=0; domain_index<m_count; ++domain_index) {
            for (std::size_t idx=0; idx<m_last_output[domain_index].get_dim(); ++idx) {
                rval.push_back(m_last_output[domain_index][idx]);
            }
        }

        return rval;
    }

    std::map<std::string, float> DomainNetMap::last_output(int domain_index) const
    {
        std::map<std::string, float> rval;

        for (std::size_t idx=0; idx<m_last_output[domain_index].get_dim(); ++idx) {
            rval[m_trace_outputs.at(idx)] = m_last_output.at(domain_index)[idx];
        }

        return rval;
    }
}
