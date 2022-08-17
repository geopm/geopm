#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TensorTwoD.hpp"

class TensorTwoDTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorTwoD mat;
        TensorTwoD vec;
};

void TensorTwoDTest::SetUp()
{
    mat.set_dim(2, 3);

    mat[0][0] = 1;
    mat[0][1] = 2;
    mat[0][2] = 3;
    mat[1][0] = 4;
    mat[1][1] = 5;
    mat[1][2] = 6;

    vec.set_dim(1, 3);
    vec[0][0] = 1;
    vec[0][1] = 2;
    vec[0][2] = 3;
}

TEST_F(TensorTwoDTest, test_mat_prod) {
    TensorOneD prod = mat * vec[0];
    EXPECT_EQ(2, prod.get_dim());
    EXPECT_EQ(14, prod[0]);
    EXPECT_EQ(32, prod[1]);
}

TEST_F(TensorTwoDTest, test_copy) {
    TensorTwoD copy(3, 4);
    copy.set_dim(1, 1);
    copy = mat;
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, test_copy_constructor) {
    TensorTwoD copy(mat);
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, input) {
    std::istringstream in("2 2 1 1 2");
    TensorTwoD x;
    in >> x;
    EXPECT_EQ(2, x.get_rows());
    EXPECT_EQ(1, x.get_cols());
    EXPECT_EQ(1, x[0][0]);
    EXPECT_EQ(2, x[1][0]);
}

TEST_F(TensorTwoDTest, output) {
    std::ostringstream out;
    TensorTwoD x(1, 2);
    x[0][0] = 8;
    x[0][1] = 16;
    out << x;
    EXPECT_EQ("2 1 2 8 16 ", out.str());
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
