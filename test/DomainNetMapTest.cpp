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
using ::testing::Return;
using ::testing::_;

class DomainNetMapTest : public ::testing::Test
{
    protected:
        std::string m_filename = "domain_net_map_test.json";
        void TearDown() override
        {
            std::remove(m_filename.c_str());
        }
};

TEST_F(DomainNetMapTest, test_json_parsing)
{
    {
        std::ofstream bad_json(m_filename);
        bad_json << "{[\"test\"]" << std::endl;
        bad_json.close();

        MockPlatformIO fake_plat_io;
        auto fake_nn_factory = std::make_shared<MockNNFactory>();
        GEOPM_EXPECT_THROW_MESSAGE(
                        DomainNetMapImp(m_filename,
                                        GEOPM_DOMAIN_PACKAGE,
                                        0,
                                        fake_plat_io,
                                        fake_nn_factory),
                        GEOPM_ERROR_INVALID,
                        "Neural net must contain valid json");
    }

    {
        std::ofstream bad_json(m_filename);
        bad_json << "{\"layers\": 15}" << std::endl;
        bad_json.close();

        MockPlatformIO fake_plat_io;
        auto fake_nn_factory = std::make_shared<MockNNFactory>();
        GEOPM_EXPECT_THROW_MESSAGE(
                        DomainNetMapImp(m_filename,
                                        GEOPM_DOMAIN_PACKAGE,
                                        0,
                                        fake_plat_io,
                                        fake_nn_factory),
                        GEOPM_ERROR_INVALID,
                        "must have a key \"layers\" whose value is an array");
    }
}

TEST_F(DomainNetMapTest, test_plumbing)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::ofstream good_json(m_filename);
    good_json << 
        "{\"layers\": ["
        "[[[1, 2, 3], [4, 5, 6]], [7, 8]]"
        "],"
        "\"signal_inputs\": [[\"A\", 1, 0]],"
        "\"delta_inputs\": ["
            "[[\"B\", 1, 0], [\"C\", 1, 0]],"
            "[[\"D\", 1, 0], [\"E\", 1, 0]]"
        "],"
        "\"trace_outputs\": [\"GEO\", \"PM\", \"@\", \"INTEL\", \"2023\"]}" << std::endl;
    good_json.close();

    MockPlatformIO fake_plat_io;
    auto fake_nn_factory = std::make_shared<MockNNFactory>();
    std::shared_ptr<MockLocalNeuralNet> fake_nn = std::make_shared<MockLocalNeuralNet>();
    std::vector<std::vector<double> > weight_vals(
                {{1, 2, 3}, {4, 5, 6}});
    TensorTwoD weights(weight_vals, fake_math);

    TensorOneD biases(
            std::vector<double>(
                {7, 8}
                ),
            fake_math
            );

    TensorOneD tmp1(
            std::vector<double>(
                {4, 3, -1, 0, 2}
                ),
            fake_math
            );

    TensorOneD tmp2(
            std::vector<double>(
                {0, 2, -4}
                ),
            fake_math
            );


    std::shared_ptr<MockDenseLayer> fake_layer = std::make_shared<MockDenseLayer>();

    ON_CALL(*fake_nn_factory, createTensorOneD(_))
        .WillByDefault(Return(biases));
    ON_CALL(*fake_nn_factory, createTensorTwoD(_))
        .WillByDefault(Return(weights));
    ON_CALL(*fake_nn_factory, createDenseLayer(_, _))
        .WillByDefault(Return(fake_layer));
    ON_CALL(*fake_nn_factory, createLocalNeuralNet(_))
        .WillByDefault(Return(fake_nn));
    ON_CALL(fake_plat_io, push_signal("A", _, _))
        .WillByDefault(Return(0));
    ON_CALL(fake_plat_io, push_signal("B", _, _))
        .WillByDefault(Return(1));
    ON_CALL(fake_plat_io, push_signal("C", _, _))
        .WillByDefault(Return(2));
    ON_CALL(fake_plat_io, push_signal("D", _, _))
        .WillByDefault(Return(3));
    ON_CALL(fake_plat_io, push_signal("E", _, _))
        .WillByDefault(Return(4));

    EXPECT_CALL(*fake_nn_factory, createTensorOneD(_))
        .Times(2);
    EXPECT_CALL(*fake_nn_factory, createTensorOneD(ElementsAre(0, 2, -4)))
        .Times(1);
    EXPECT_CALL(*fake_nn_factory, createTensorTwoD(weight_vals))
        .Times(1);
    EXPECT_CALL(*fake_nn_factory, createDenseLayer(TensorTwoDEqualTo(weights), TensorOneDEqualTo(biases)))
        .Times(1);
    EXPECT_CALL(*fake_nn_factory, createLocalNeuralNet(ElementsAre(fake_layer)))
        .Times(1);

    EXPECT_CALL(fake_plat_io, push_signal("A", _, _)).Times(1);
    EXPECT_CALL(fake_plat_io, push_signal("B", _, _)).Times(1);
    EXPECT_CALL(fake_plat_io, push_signal("C", _, _)).Times(1);
    EXPECT_CALL(fake_plat_io, push_signal("D", _, _)).Times(1);
    EXPECT_CALL(fake_plat_io, push_signal("E", _, _)).Times(1);
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

    ON_CALL(*fake_nn, forward(_)).WillByDefault(Return(tmp1));
    EXPECT_CALL(*fake_nn, forward(_)).Times(2);

    net_map.sample();
    net_map.sample();
    EXPECT_EQ(std::vector<std::string>({"GEO", "PM", "@", "INTEL", "2023"}),
              net_map.trace_names());
    EXPECT_EQ(std::vector<double>({4, 3, -1, 0, 2}), net_map.trace_values());
    std::map<std::string, double> expected_output({{"GEO", 4}, {"PM", 3}, {"@", -1}, {"INTEL", 0}, {"2023", 2}});
    EXPECT_EQ(expected_output, net_map.last_output());
}
