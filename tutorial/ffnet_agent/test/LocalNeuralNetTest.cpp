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
    std::istringstream in("4 2 1 2 8 16 1 1 3 2 1 1 1 1 1 0");
    in >> net;
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
}

TEST_F(LocalNeuralNetTest, test_out) {
    std::ostringstream out;
    out << net;
    EXPECT_EQ("4\n2 1 2 8 16 \n1 1 3 \n2 1 1 1 \n1 1 0 \n", out.str());
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
