#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

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

TEST_F(TensorTwoDTest, test_array_overload) {
    const TensorTwoD mat_copy(mat);
    mat[0] = mat_copy[1];
    EXPECT_EQ(4, mat[0][0]);
    EXPECT_EQ(5, mat[0][1]);
    EXPECT_EQ(6, mat[0][2]);

    // check that the copy is deep
    mat[0][0] = 7;
    EXPECT_EQ(7, mat[0][0]);
    EXPECT_EQ(4, mat_copy[1][0]);
}


TEST_F(TensorTwoDTest, input) {
    std::vector<std::vector<float> > vals = {{1}, {2}};
    TensorTwoD x;
    x = TensorTwoD(json11::Json(vals));
    EXPECT_EQ(2, x.get_rows());
    EXPECT_EQ(1, x.get_cols());
    EXPECT_EQ(1, x[0][0]);
    EXPECT_EQ(2, x[1][0]);
}

TEST_F(TensorTwoDTest, test_degenerate_size) {
    TensorTwoD x;
    EXPECT_EQ(0, x.get_cols());
}

TEST_F(TensorTwoDTest, test_bad_dimensions) {
    vec.set_dim(1, 2);
    EXPECT_THROW(mat * vec[0], geopm::Exception);
    EXPECT_THROW(vec.set_dim(0, 1), geopm::Exception);
    std::vector<std::vector<float> > vals = {{1}, {2, 3}};
    EXPECT_THROW(TensorTwoD(json11::Json(vals)), geopm::Exception);
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
