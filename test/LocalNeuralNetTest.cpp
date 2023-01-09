/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "LocalNeuralNetImp.hpp"

using geopm::LocalNeuralNetImp;

class LocalNeuralNetTest : public ::testing::Test
{
};

TEST_F(LocalNeuralNetTest, test_inference)
{
    LocalNeuralNetImp net(
            std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> {8, 16} },
                    std::vector<float> {3}
                ),
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> { 1 } },
                    std::vector<float> {0}
                )
            }
          );

    std::vector<float> inp = {1, 2};
    std::vector<float> out = net(inp);
    EXPECT_EQ(1u, out.size());
    EXPECT_NEAR(1/(1 + expf(-43)), out[0], 1e-6);
}

TEST_F(LocalNeuralNetTest, test_bad_layers)
{
    EXPECT_THROW(LocalNeuralNetImp (
            std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> {8, 16} },
                    std::vector<float> {3, 0}
                ),
            }
          ), geopm::Exception);

    EXPECT_THROW(LocalNeuralNetImp (
            std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> {8, 16} },
                    std::vector<float> {3}
                ),
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> {1, 2} },
                    std::vector<float> {0}
                ),
            }
          ), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_empty_array)
{
    EXPECT_THROW(LocalNeuralNetImp (
            std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
            }
          ), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_inference_bad_input_dimensions)
{
    std::vector<float> inp = {1, 2, 3};
    LocalNeuralNetImp net(
            std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> {8, 16} },
                    std::vector<float> {3}
                ),
                std::make_pair (
                    std::vector<std::vector<float> > {  std::vector<float> { 1 } },
                    std::vector<float> {0}
                )
            }
          );
    EXPECT_THROW(net(inp), geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_non_rectangular_weights)
{
    EXPECT_THROW(
            LocalNeuralNetImp net(
                    std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                        std::make_pair (
                            std::vector<std::vector<float> > { std::vector<float> {8, 16}, std::vector<float> {1} },
                            std::vector<float> {3, 1}
                        ),
                    }
                  ),
            geopm::Exception);
}

TEST_F(LocalNeuralNetTest, test_empty_bias)
{
    EXPECT_THROW(
            LocalNeuralNetImp net(
                    std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > {
                        std::make_pair (
                            std::vector<std::vector<float> > { std::vector<float> {8, 16} },
                            std::vector<float> {}
                        ),
                    }
                  ),
            geopm::Exception);
}
