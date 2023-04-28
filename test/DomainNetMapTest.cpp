/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "DomainNetMap.hpp"
#include "DomainNetMapImp.hpp"

#include "MockDenseLayer.hpp"
#include "MockLocalNeuralNet.hpp"
#include "MockNNFactory.hpp"
#include "MockPlatformIO.hpp"
#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"
#include "TensorTwoDMatcher.hpp"

using geopm::DomainNetMap;
using geopm::DomainNetMapImp;
using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class DomainNetMapTest : public ::testing::Test
{
    protected:
        void SetUp() override;
        void TearDown() override;

        const std::string M_FILENAME = "domain_net_map_test.json";
        std::shared_ptr<MockNNFactory> m_fake_nn_factory;
        MockPlatformIO m_fake_plat_io;
        std::shared_ptr<MockTensorMath> m_fake_math;
        std::shared_ptr<MockLocalNeuralNet> m_fake_nn;
        std::shared_ptr<MockDenseLayer> m_fake_layer;
        std::vector<std::vector<double>> m_weight_vals;
        TensorTwoD m_weights;
        TensorOneD m_biases;
        TensorOneD m_tmp1, m_tmp2;
};

void DomainNetMapTest::SetUp()
{
    m_fake_nn_factory = std::make_shared<MockNNFactory>();
    m_fake_math = std::make_shared<MockTensorMath>();
    m_fake_nn = std::make_shared<MockLocalNeuralNet>();

    m_weight_vals = {{1, 2, 3}, {4, 5, 6}};
    m_weights = TensorTwoD(m_weight_vals, m_fake_math);
    m_biases = TensorOneD({7, 8}, m_fake_math);
    m_tmp1 = TensorOneD({4, 3, -1, 0, 2}, m_fake_math);
    m_tmp2 = TensorOneD({0, 2, -4}, m_fake_math);

    m_fake_layer = std::make_shared<MockDenseLayer>();

    ON_CALL(*m_fake_nn_factory, createLocalNeuralNet(_))
        .WillByDefault(Return(m_fake_nn));

    ON_CALL(*m_fake_nn, get_input_dim()).WillByDefault(Return(1));
    ON_CALL(*m_fake_nn, get_output_dim()).WillByDefault(Return(1));
}

void DomainNetMapTest::TearDown()
{
    remove(M_FILENAME.c_str());
}

TEST_F(DomainNetMapTest, test_json_parsing)
{
    // malformed json
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{[\"test\"]" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Neural net file format is incorrect");
    }

    // empty file
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Neural net file format is incorrect");
    }
    
    // empty json
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{ }" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // layers missing
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // layers are not actual layers
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": 15,"
            "\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // extraneous keys
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"],"
            "\"horses\": \"edible\"}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Unexpected key");
    }

    // missing both signal_inputs and delta_inputs
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must contain at least one of \"signal_inputs\" and \"delta_inputs\"");
    }

    // valid signal_inputs, invalid delta_inputs 
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": \"B\","
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "\"delta_inputs\" must be an array");
    }

    // valid delta_inputs, invalid signal_inputs 
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": \"A\","
            "\"delta_inputs\": ["
            "[\"B\", \"C\"],"
            "[\"D\", \"E\"]"
            "],"
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "\"signal_inputs\" must be an array");
    }

    // missing trace_outputs
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*m_fake_nn, get_input_dim()).Times(1);
        EXPECT_CALL(*m_fake_nn, get_output_dim()).Times(0);

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"trace_outputs\" whose value is an array");

        // reset the mock
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }

    // mismatched input dimensions
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": [[\"B\", \"C\"]],"
            "\"trace_outputs\": [\"D\", \"E\"]}" << std::endl;
        bad_json.close();

	EXPECT_CALL(*m_fake_nn, get_input_dim()).Times(1);

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "input dimension must match the number of signal and delta inputs");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }

    // mismatched output dimensions
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": [[\"B\", \"C\"], [\"D\", \"E\"]],"
            "\"trace_outputs\": [\"F\", \"G\", \"H\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*m_fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*m_fake_nn, get_output_dim()).WillOnce(Return(2));

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "output dimension must match the number of trace outputs");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }

    // invalid signal_inputs values
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [1],"
            "\"delta_inputs\": ["
            "[\"B\", \"C\"],"
            "[\"D\", \"E\"]"
            "],"
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();
        
        EXPECT_CALL(*m_fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*m_fake_nn, get_output_dim()).WillOnce(Return(5));

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "signal inputs must be strings");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }

    // invalid delta_inputs values
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": ["
            "[1, 2],"
            "[\"D\", \"E\"]"
            "],"
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*m_fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*m_fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(m_fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "delta inputs must be tuples of strings");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }
    
    // invalid delta_inputs values
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": ["
            "\"A\","
            "[\"D\", \"E\"]"
            "],"
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*m_fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*m_fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(m_fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "delta inputs must be tuples of strings");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }

    // invalid trace_outputs values
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": ["
            "[\"B\", \"C\"],"
            "[\"D\", \"E\"]"
            "],"
            "\"trace_outputs\": [1, 2, 3, 4, 2023]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*m_fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*m_fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(m_fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));
        EXPECT_CALL(m_fake_plat_io, push_signal("B", _, _)).WillOnce(Return(1));
        EXPECT_CALL(m_fake_plat_io, push_signal("C", _, _)).WillOnce(Return(2));
        EXPECT_CALL(m_fake_plat_io, push_signal("D", _, _)).WillOnce(Return(3));
        EXPECT_CALL(m_fake_plat_io, push_signal("E", _, _)).WillOnce(Return(4));

        EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
            .WillOnce(Return(m_weights));
        EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
            .WillOnce(Return(m_biases));
        EXPECT_CALL(*m_fake_nn_factory,
                createDenseLayer(TensorTwoDEqualTo(m_weights),
                    TensorOneDEqualTo(m_biases)))
            .WillOnce(Return(m_fake_layer));
        EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
            .WillOnce(Return(m_fake_nn));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(M_FILENAME,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    m_fake_plat_io,
                    m_fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "trace outputs must be strings");
        Mock::VerifyAndClearExpectations(m_fake_nn_factory.get());
    }
}

TEST_F(DomainNetMapTest, test_plumbing)
{
    std::ofstream good_json(M_FILENAME);
    good_json << 
        "{\"layers\": ["
        "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
        "],"
        "\"signal_inputs\": [\"A\"],"
        "\"delta_inputs\": ["
        "[\"B\", \"C\"],"
        "[\"D\", \"E\"]"
        "],"
        "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
    good_json.close();

    EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(_))
        .WillOnce(Return(m_biases))
        .WillOnce(Return(m_tmp1));
    EXPECT_CALL(*m_fake_nn_factory, createTensorOneD(ElementsAre(0, 2, -4)))
        .WillOnce(Return(m_biases));
    EXPECT_CALL(*m_fake_nn_factory, createTensorTwoD(m_weight_vals))
        .WillOnce(Return(m_weights));
    EXPECT_CALL(*m_fake_nn_factory,
            createDenseLayer(TensorTwoDEqualTo(m_weights),
                TensorOneDEqualTo(m_biases)))
        .WillOnce(Return(m_fake_layer));
    EXPECT_CALL(*m_fake_nn_factory, createLocalNeuralNet(ElementsAre(m_fake_layer)))
        .WillOnce(Return(m_fake_nn));

    EXPECT_CALL(*m_fake_nn, get_input_dim()).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_fake_nn, get_output_dim()).WillRepeatedly(Return(5));

    EXPECT_CALL(m_fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));
    EXPECT_CALL(m_fake_plat_io, push_signal("B", _, _)).WillOnce(Return(1));
    EXPECT_CALL(m_fake_plat_io, push_signal("C", _, _)).WillOnce(Return(2));
    EXPECT_CALL(m_fake_plat_io, push_signal("D", _, _)).WillOnce(Return(3));
    EXPECT_CALL(m_fake_plat_io, push_signal("E", _, _)).WillOnce(Return(4));

    EXPECT_CALL(m_fake_plat_io, sample(0)).WillOnce(Return(1)).WillOnce(Return(0));
    EXPECT_CALL(m_fake_plat_io, sample(1)).WillOnce(Return(2)).WillOnce(Return(4));
    EXPECT_CALL(m_fake_plat_io, sample(2)).WillOnce(Return(3)).WillOnce(Return(4));
    EXPECT_CALL(m_fake_plat_io, sample(3)).WillOnce(Return(4)).WillOnce(Return(0));
    EXPECT_CALL(m_fake_plat_io, sample(4)).WillOnce(Return(5)).WillOnce(Return(6));

    DomainNetMapImp net_map(M_FILENAME,
            GEOPM_DOMAIN_PACKAGE,
            0,
            m_fake_plat_io,
            m_fake_nn_factory);

    EXPECT_CALL(*m_fake_nn, forward(_)).WillRepeatedly(Return(m_tmp1));

    net_map.sample();
    net_map.sample();
    EXPECT_EQ(std::vector<std::string>({"GEO", "PM", "@", "INTEL", "2023"}),
            net_map.trace_names());
    EXPECT_EQ(std::vector<double>({4, 3, -1, 0, 2}), net_map.trace_values());
    std::map<std::string, double> expected_output({{"GEO", 4}, {"PM", 3}, {"@", -1}, {"INTEL", 0}, {"2023", 2}});
    EXPECT_EQ(expected_output, net_map.last_output());
}
