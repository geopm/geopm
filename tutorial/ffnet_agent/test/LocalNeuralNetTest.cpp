#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "LocalNeuralNet.hpp"

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
    EXPECT_EQ(1, out.get_dim());
    EXPECT_EQ(1/(1 + expf(-43)), out[0]);
}

TEST_F(LocalNeuralNetTest, test_copy) {
    LocalNeuralNet net2(net);
    TensorOneD inp(2);
    inp[0] = 0;
    inp[1] = 0;
    TensorOneD out = net2.model(inp);
    EXPECT_EQ(1, out.get_dim());
    EXPECT_EQ(1/(1 + expf(-3)), out[0]);
    net2 = net;
    out = net2.model(inp);
    EXPECT_EQ(1/(1 + expf(-3)), out[0]);
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
