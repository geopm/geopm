#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TensorOneD.hpp"

class TensorOneDTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorOneD one, two;
};

void TensorOneDTest::SetUp()
{
    one.set_dim(2);
    two.set_dim(2);

    one[0] = 1;
    one[1] = 2;
    two[0] = 3;
    two[1] = 4;
}

TEST_F(TensorOneDTest, test_sum) {
    TensorOneD three = one + two;
    EXPECT_EQ(4, three[0]);
    EXPECT_EQ(6, three[1]);
}

TEST_F(TensorOneDTest, test_diff) {
    TensorOneD three(one - two);
    EXPECT_EQ(-2, three[0]);
    EXPECT_EQ(-2, three[1]);
}

TEST_F(TensorOneDTest, test_dot) {
    EXPECT_EQ(11, one * two);
}

TEST_F(TensorOneDTest, test_sigmoid) {
    TensorOneD activations(5);

    activations[0] = -log(1/0.1 - 1);
    activations[1] = -log(1/0.25 - 1);
    activations[2] = -log(1/0.5 - 1);
    activations[3] = -log(1/0.75 - 1);
    activations[4] = -log(1/0.9 - 1);

    TensorOneD output = activations.sigmoid();

    EXPECT_FLOAT_EQ(0.1, output[0]);
    EXPECT_FLOAT_EQ(0.25, output[1]);
    EXPECT_FLOAT_EQ(0.5, output[2]);
    EXPECT_FLOAT_EQ(0.75, output[3]);
    EXPECT_FLOAT_EQ(0.9, output[4]);
}

TEST_F(TensorOneDTest, test_copy) {
    two = one;

    // copy is successful
    EXPECT_EQ(1, two[0]);
    EXPECT_EQ(2, two[1]);

    // copy is deep
    two[0] = 9;
    EXPECT_EQ(1, one[0]);
    EXPECT_EQ(9, two[0]);
}

TEST_F(TensorOneDTest, input) {
    TensorOneD x(3);
    x.set_dim(4);
    std::vector<float> vals = {8, 16};
    x = TensorOneD(json11::Json(vals));
    EXPECT_EQ(2, x.get_dim());
    EXPECT_EQ(8, x[0]);
    EXPECT_EQ(16, x[1]);
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
