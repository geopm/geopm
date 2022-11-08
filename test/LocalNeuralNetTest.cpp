/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorOneD.hpp"
#include "LocalNeuralNet.hpp"

using geopm::TensorOneD;
using geopm::LocalNeuralNet;

class LocalNeuralNetTest : public ::testing::Test
{
    protected:
        void SetUp();
        LocalNeuralNet net;
};

void LocalNeuralNetTest::SetUp()
{
    net = LocalNeuralNet (
            json11::Json::array {
                json11::Json::array {
                    json11::Json::array {  json11::Json::array {8, 16} },
                    json11::Json::array {3}
                },
                json11::Json::array {
                    json11::Json::array { json11::Json::array { 1 } },
                    json11::Json::array {0}
                }
            }
          );
}

TEST_F(LocalNeuralNetTest, test_inference) {
    TensorOneD inp(2);
    inp[0] = 1;
    inp[1] = 2;
    TensorOneD out = net.model(inp);
    EXPECT_EQ(1u, out.get_dim());
    EXPECT_NEAR(1/(1 + expf(-43)), out[0], 1e-6);
}

TEST_F(LocalNeuralNetTest, test_copy) {
    LocalNeuralNet net2(net);
    TensorOneD inp(2);
    inp[0] = 0;
    inp[1] = 0;
    TensorOneD out = net2.model(inp);
    EXPECT_EQ(1u, out.get_dim());
    EXPECT_NEAR(1/(1 + expf(-3)), out[0], 1e-6);
    net2 = net;
    out = net2.model(inp);
    EXPECT_NEAR(1/(1 + expf(-3)), out[0], 1e-6);
}

TEST_F(LocalNeuralNetTest, test_bad_layers) {
    EXPECT_THROW(LocalNeuralNet (
            json11::Json::array {
                json11::Json::array {
                    json11::Json::array {  json11::Json::array {8, 16} },
                    json11::Json::array {3, 0}
                }
            }
          ), geopm::Exception);

    EXPECT_THROW(LocalNeuralNet (
            json11::Json::array {
                json11::Json::array {
                    json11::Json::array {  json11::Json::array {8, 16} },
                    json11::Json::array {3}
                },
                json11::Json::array {
                    json11::Json::array { json11::Json::array { 1, 2 } },
                    json11::Json::array {0}
                }
            }
          ), geopm::Exception);

}

TEST_F(LocalNeuralNetTest, test_non_array) {
    std::string vals = "soup";
    EXPECT_THROW(LocalNeuralNet(json11::Json(vals)), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_empty_array) {
    EXPECT_THROW(LocalNeuralNet(json11::Json::array {}), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_malformed_array) {
    EXPECT_THROW(LocalNeuralNet (
            json11::Json::array {
                json11::Json::array {
                    json11::Json::array {  json11::Json::array {8, 16} },
                    json11::Json::array {3},
                    json11::Json::array {3, 0}
                }
            }
          ), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_inference_bad_dimensions) {
    TensorOneD inp(3);
    inp[0] = 1;
    inp[1] = 2;
    inp[2] = 3;
    EXPECT_THROW(net.model(inp), geopm::Exception);
}

