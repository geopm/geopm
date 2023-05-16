/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "DomainNetMapImp.hpp"

#include <cmath>
#include <fstream>
#include <set>

#include "geopm/PlatformIO.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "NNFactoryImp.hpp"

namespace geopm
{
    const std::set<std::string> DomainNetMapImp::M_EXPECTED_KEYS = {
            "layers",
            "signal_inputs",
            "delta_inputs",
            "trace_outputs",
            "description"};

    std::unique_ptr<DomainNetMap> DomainNetMap::make_unique(const std::string &nn_path,
                                                            geopm_domain_e domain_type,
                                                            int domain_index)
    {
        return geopm::make_unique<DomainNetMapImp>(nn_path, domain_type, domain_index);
    }

    std::shared_ptr<DomainNetMap> DomainNetMap::make_shared(const std::string &nn_path,
                                                            geopm_domain_e domain_type,
                                                            int domain_index)
    {
        return std::make_shared<DomainNetMapImp>(nn_path, domain_type, domain_index);
    }

    DomainNetMapImp::DomainNetMapImp(const std::string &nn_path,
                                     geopm_domain_e domain_type,
                                     int domain_index)
        : DomainNetMapImp(nn_path, domain_type, domain_index, platform_io(),
                          NNFactory::make_shared())
    {
    }

    DomainNetMapImp::DomainNetMapImp(const std::string &nn_path,
                                     geopm_domain_e domain_type,
                                     int domain_index,
                                     PlatformIO &plat_io,
                                     std::shared_ptr<NNFactory> nn_factory)
        : m_platform_io(plat_io)
        , m_nn_factory(nn_factory)
    {
        std::ifstream file(nn_path);

        if (!file.is_open()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Unable to open neural net file: " + nn_path + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        if (length >= M_MAX_NNET_SIZE) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural net file exceeds maximum size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string buf, err;
        buf.reserve(length);

        std::getline(file, buf, '\0');

        const json11::Json nnet_json = json11::Json::parse(buf, err);

        if (!err.empty()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural net file format is incorrect: " + err + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (nnet_json.is_null() || !nnet_json.is_object()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural net file format is incorrect: object expected.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // make sure that there no unexpected keys in the json
        for (const auto &key : nnet_json.object_items()) {
            if (M_EXPECTED_KEYS.find(key.first) == M_EXPECTED_KEYS.end()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                                ": Unexpected key in neural net json: " + key.first,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        // will check key exists, it's an array, and is not empty
        if (nnet_json["layers"].array_items().size() == 0) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                            ": Neural net must contain valid json and must have "
                            "a key \"layers\" whose value is a non-empty array.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        const auto nnet_properties = nnet_json.object_items();
        const auto signal_it = nnet_properties.find("signal_inputs");
        const auto delta_it = nnet_properties.find("delta_inputs");
        size_t signal_inputs_size = 0;
        size_t delta_inputs_size = 0;

        if (signal_it != nnet_properties.end()) {
            if (!signal_it->second.is_array()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) +
                                 ": Neural net \"signal_inputs\" must be an array.",
                                 GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            signal_inputs_size = signal_it->second.array_items().size();
        }

        if (delta_it != nnet_properties.end()) {
            if (!delta_it->second.is_array()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) +
                                ": Neural net \"delta_inputs\" must be an array.",
                                 GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            delta_inputs_size = delta_it->second.array_items().size();
        }

        if (signal_inputs_size == 0 && delta_inputs_size == 0) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                            ": Neural net json must contain at least one of "
                            "\"signal_inputs\" and \"delta_inputs\" whose "
                            "value is a non-empty array.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        
        std::vector<std::shared_ptr<DenseLayer> > layers;
        for (const auto &layer : nnet_json["layers"].array_items()) {
            layers.push_back(json_to_DenseLayer(layer));
        }

        m_neural_net = m_nn_factory->createLocalNeuralNet(layers);

        if (signal_inputs_size + delta_inputs_size != m_neural_net->get_input_dim()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                            ": Neural net input dimension must match the number of "
                            "signal and delta inputs.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!nnet_json["trace_outputs"].is_array()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                            ": Neural net json must have a key \"trace_outputs\" "
                            "whose value is an array.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (nnet_json["trace_outputs"].array_items().size()
            != m_neural_net->get_output_dim()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                            ": Neural net output dimension must match the number of "
                            "trace outputs.",
                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (const auto &input : signal_it->second.array_items()) {
            if (!input.is_string()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                                ": Neural net signal inputs must be strings.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_signal_inputs.push_back({m_platform_io.push_signal(input.string_value(),
                                                                 domain_type,
                                                                 domain_index),
                                       NAN});
        }

        for (const auto &input : delta_it->second.array_items()) {
            if (!input.is_array() || input.array_items().size() != 2 ||
                !input[0].is_string() || !input[1].is_string()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                                ": Neural net delta inputs must be tuples of strings.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_delta_inputs.push_back({m_platform_io.push_signal(input[0].string_value(),
                                                                domain_type,
                                                                domain_index),
                                      m_platform_io.push_signal(input[1].string_value(),
                                                                domain_type,
                                                                domain_index),
                                      NAN, NAN, NAN, NAN});
        }

        for (const auto &output : nnet_json["trace_outputs"].array_items()) {
            if (!output.is_string()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) + 
                                ": Neural net trace outputs must be strings.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_trace_outputs.push_back(output.string_value());
        }
    }

    std::shared_ptr<DenseLayer> DomainNetMapImp::json_to_DenseLayer(const json11::Json &obj) const
    {
        if (!obj.is_array()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural network weights contains non-array-type.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (obj.array_items().size() != 2) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Dense Layer weights must be an array of length exactly two.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorTwoD weights = json_to_TensorTwoD(obj[0]);
        TensorOneD biases = json_to_TensorOneD(obj[1]);
        return m_nn_factory->createDenseLayer(weights, biases);
    }

    TensorOneD DomainNetMapImp::json_to_TensorOneD(const json11::Json &obj) const
    {
        if (!obj.is_array()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural network weights contains non-array-type.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (obj.array_items().empty()) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Empty array is invalid for neural network weights.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        size_t vec_size = obj.array_items().size();
        std::vector<double> vals(vec_size);

        for (size_t idx = 0; idx < vec_size; ++idx) {
            if (!obj[idx].is_number()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) +
                                ": Non-numeric type found in neural network weights.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            vals[idx] = obj[idx].number_value();
        }

        return m_nn_factory->createTensorOneD(vals);
    }

    TensorTwoD DomainNetMapImp::json_to_TensorTwoD(const json11::Json &obj) const
    {
        if (!obj.is_array()){
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Neural network weights is non-array-type.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (obj.array_items().size() == 0) {
            throw Exception("DomainNetMapImp::" + std::string(__func__) +
                            ": Empty array is invalid for neural network weights.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<std::vector<double> > vals;
        for (size_t ridx = 0; ridx < obj.array_items().size(); ++ridx) {
            if (!obj[ridx].is_array()) {
                throw Exception("DomainNetMapImp::" + std::string(__func__) +
                                ": Neural network weights is non-array-type.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            size_t vec_size = obj[ridx].array_items().size();
            std::vector<double> row(vec_size);

            for (size_t cidx = 0; cidx < vec_size; ++cidx) {
                if (!obj[ridx][cidx].is_number()) {
                    throw Exception("DomainNetMapImp::" + std::string(__func__) +
                                    ": Non-numeric type found in neural network weights.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                row[cidx] = obj[ridx][cidx].number_value();
            }

            vals.push_back(row);
        }

        return m_nn_factory->createTensorTwoD(vals);
    }

    void DomainNetMapImp::sample()
    {
        std::vector<double> xs;

        // Sample latest signal values
        for (auto &input : m_signal_inputs) {
            input.signal = m_platform_io.sample(input.batch_idx);
            xs.push_back(input.signal);
        }
        for (auto &input : m_delta_inputs) {
            input.signal_num_last = input.signal_num;
            input.signal_den_last = input.signal_den;
            input.signal_num = m_platform_io.sample(input.batch_idx_num);
            input.signal_den = m_platform_io.sample(input.batch_idx_den);
            xs.push_back((input.signal_num - input.signal_num_last) /
                         (input.signal_den - input.signal_den_last));
        }

        m_last_output = m_neural_net->forward(m_nn_factory->createTensorOneD(xs));
    }

    std::vector<std::string> DomainNetMapImp::trace_names() const
    {
        return m_trace_outputs;
    }

    std::vector<double> DomainNetMapImp::trace_values() const
    {
        std::vector<double> rval(m_last_output.get_data().begin(),
                                 m_last_output.get_data().end());
        return rval;
    }

    std::map<std::string, double> DomainNetMapImp::last_output() const
    {
        std::map<std::string, double> rval;

        for (size_t idx=0; idx<m_last_output.get_dim(); ++idx) {
            rval[m_trace_outputs.at(idx)] = m_last_output[idx];
        }

        return rval;
    }
}
