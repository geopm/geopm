/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
using geopm::MockDenseLayer;
using geopm::MockLocalNeuralNet;
using geopm::MockNNFactory;
using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

class DomainNetMapTest : public ::testing::Test
{
    protected:
        void SetUp() override;
        void TearDown() override;

        std::string m_filename = "domain_net_map_test.json";
        std::shared_ptr<NiceMock<MockNNFactory>> fake_nn_factory;
        MockPlatformIO fake_plat_io;
        std::shared_ptr<MockTensorMath> fake_math;
        std::shared_ptr<MockLocalNeuralNet> fake_nn;
        std::shared_ptr<MockDenseLayer> fake_layer;
        std::vector<std::vector<double>> weight_vals;
        TensorTwoD weights;
        TensorOneD biases;
        TensorOneD tmp1, tmp2;
};

void DomainNetMapTest::SetUp()
{
    fake_nn_factory = std::make_shared<NiceMock<MockNNFactory>>();
    fake_math = std::make_shared<MockTensorMath>();
    fake_nn = std::make_shared<MockLocalNeuralNet>();

    weight_vals = {{1, 2, 3}, {4, 5, 6}};
    weights = TensorTwoD(weight_vals, fake_math);
    biases = TensorOneD({7, 8}, fake_math);
    tmp1 = TensorOneD({4, 3, -1, 0, 2}, fake_math);
    tmp2 = TensorOneD({0, 2, -4}, fake_math);

    fake_layer = std::make_shared<MockDenseLayer>();

    ON_CALL(*fake_nn_factory, createLocalNeuralNet(_))
        .WillByDefault(Return(fake_nn));

    ON_CALL(*fake_nn, get_input_dim()).WillByDefault(Return(1));
    ON_CALL(*fake_nn, get_output_dim()).WillByDefault(Return(1));
}

void DomainNetMapTest::TearDown()
{
    remove(m_filename.c_str());
}

TEST_F(DomainNetMapTest, test_json_parsing)
{
    // malformed json
    {
        std::ofstream bad_json(m_filename);
        bad_json << "{[\"test\"]" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Neural net file format is incorrect");
    }

    // empty file
    {
        std::ofstream bad_json(m_filename);
        bad_json << "" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Neural net file format is incorrect");
    }
    
    // empty json
    {
        std::ofstream bad_json(m_filename);
        bad_json << "{ }" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // layers missing
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // layers are not actual layers
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": 15,"
            "\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"layers\" whose value is a non-empty array");
    }

    // extraneous keys
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"trace_outputs\": [\"B\"],"
            "\"horses\": \"edible\"}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "Unexpected key");
    }

    // missing both signal_inputs and delta_inputs
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"trace_outputs\": [\"B\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must contain at least one of \"signal_inputs\" and \"delta_inputs\"");
    }

    // valid signal_inputs, invalid delta_inputs 
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": ["
            "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
            "],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": \"B\","
            "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "\"delta_inputs\" must be an array");
    }

    // valid delta_inputs, invalid signal_inputs 
    {
        std::ofstream bad_json(m_filename);
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
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "\"signal_inputs\" must be an array");
    }

    // missing trace_outputs
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*fake_nn, get_input_dim()).Times(1);
        EXPECT_CALL(*fake_nn, get_output_dim()).Times(0);

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "must have a key \"trace_outputs\" whose value is an array");
    }

    // mismatched input dimensions
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": [[\"B\", \"C\"]],"
            "\"trace_outputs\": [\"D\", \"E\"]}" << std::endl;
        bad_json.close();

	EXPECT_CALL(*fake_nn, get_input_dim()).Times(1);

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "input dimension must match the number of signal and delta inputs");
    }

    // mismatched output dimensions
    {
        std::ofstream bad_json(m_filename);
        bad_json << 
            "{\"layers\": [[[[1, 2, 3], [4, 5, 6]], [7, 8]]],"
            "\"signal_inputs\": [\"A\"],"
            "\"delta_inputs\": [[\"B\", \"C\"], [\"D\", \"E\"]],"
            "\"trace_outputs\": [\"F\", \"G\", \"H\"]}" << std::endl;
        bad_json.close();

        EXPECT_CALL(*fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*fake_nn, get_output_dim()).WillOnce(Return(2));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "output dimension must match the number of trace outputs");
    }

    // invalid signal_inputs values
    {
        std::ofstream bad_json(m_filename);
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
        
        EXPECT_CALL(*fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*fake_nn, get_output_dim()).WillOnce(Return(5));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "signal inputs must be strings");
    }

    // invalid delta_inputs values
    {
        std::ofstream bad_json(m_filename);
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

        EXPECT_CALL(*fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "delta inputs must be tuples of strings");
    }
    
    // invalid delta_inputs values
    {
        std::ofstream bad_json(m_filename);
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

        EXPECT_CALL(*fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "delta inputs must be tuples of strings");
    }

    // invalid trace_outputs values
    {
        std::ofstream bad_json(m_filename);
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

        EXPECT_CALL(*fake_nn, get_input_dim()).WillOnce(Return(3));
        EXPECT_CALL(*fake_nn, get_output_dim()).WillOnce(Return(5));
        EXPECT_CALL(fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));
        EXPECT_CALL(fake_plat_io, push_signal("B", _, _)).WillOnce(Return(1));
        EXPECT_CALL(fake_plat_io, push_signal("C", _, _)).WillOnce(Return(2));
        EXPECT_CALL(fake_plat_io, push_signal("D", _, _)).WillOnce(Return(3));
        EXPECT_CALL(fake_plat_io, push_signal("E", _, _)).WillOnce(Return(4));

        GEOPM_EXPECT_THROW_MESSAGE(
                DomainNetMapImp(m_filename,
                    GEOPM_DOMAIN_PACKAGE,
                    0,
                    fake_plat_io,
                    fake_nn_factory),
                GEOPM_ERROR_INVALID,
                "trace outputs must be strings");
    }
}

TEST_F(DomainNetMapTest, test_plumbing)
{
    std::ofstream good_json(m_filename);
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

    EXPECT_CALL(*fake_nn_factory, createTensorOneD(_))
        .WillOnce(Return(biases))
        .WillOnce(Return(tmp1));
    EXPECT_CALL(*fake_nn_factory, createTensorOneD(ElementsAre(0, 2, -4)))
        .WillOnce(Return(biases));
    EXPECT_CALL(*fake_nn_factory, createTensorTwoD(weight_vals))
        .WillOnce(Return(weights));
    EXPECT_CALL(*fake_nn_factory,
            createDenseLayer(TensorTwoDEqualTo(weights),
                TensorOneDEqualTo(biases)))
        .WillOnce(Return(fake_layer));
    EXPECT_CALL(*fake_nn_factory, createLocalNeuralNet(ElementsAre(fake_layer)))
        .WillOnce(Return(fake_nn));

    EXPECT_CALL(*fake_nn, get_input_dim()).WillRepeatedly(Return(3));
    EXPECT_CALL(*fake_nn, get_output_dim()).WillRepeatedly(Return(5));

    EXPECT_CALL(fake_plat_io, push_signal("A", _, _)).WillOnce(Return(0));
    EXPECT_CALL(fake_plat_io, push_signal("B", _, _)).WillOnce(Return(1));
    EXPECT_CALL(fake_plat_io, push_signal("C", _, _)).WillOnce(Return(2));
    EXPECT_CALL(fake_plat_io, push_signal("D", _, _)).WillOnce(Return(3));
    EXPECT_CALL(fake_plat_io, push_signal("E", _, _)).WillOnce(Return(4));

    EXPECT_CALL(fake_plat_io, sample(0)).WillOnce(Return(1)).WillOnce(Return(0));
    EXPECT_CALL(fake_plat_io, sample(1)).WillOnce(Return(2)).WillOnce(Return(4));
    EXPECT_CALL(fake_plat_io, sample(2)).WillOnce(Return(3)).WillOnce(Return(4));
    EXPECT_CALL(fake_plat_io, sample(3)).WillOnce(Return(4)).WillOnce(Return(0));
    EXPECT_CALL(fake_plat_io, sample(4)).WillOnce(Return(5)).WillOnce(Return(6));

    DomainNetMapImp net_map(m_filename,
            GEOPM_DOMAIN_PACKAGE,
            0,
            fake_plat_io,
            fake_nn_factory);

    EXPECT_CALL(*fake_nn, forward(_)).WillRepeatedly(Return(tmp1));

    net_map.sample();
    net_map.sample();
    EXPECT_EQ(std::vector<std::string>({"GEO", "PM", "@", "INTEL", "2023"}),
            net_map.trace_names());
    EXPECT_EQ(std::vector<double>({4, 3, -1, 0, 2}), net_map.trace_values());
    std::map<std::string, double> expected_output({{"GEO", 4}, {"PM", 3}, {"@", -1}, {"INTEL", 0}, {"2023", 2}});
    EXPECT_EQ(expected_output, net_map.last_output());
}
