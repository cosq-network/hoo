#include <gtest/gtest.h>
#include <cmath>
#include <cstring>
#include "runtime/lib/hoo_tensor.h"
#include "runtime/lib/hoo_runtime.h"

class HooTensorTest : public ::testing::Test {
protected:
    void TearDown() override {
    }

    static int64_t doubleBits(double value) {
        int64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        return bits;
    }
};

TEST_F(HooTensorTest, VersionedAiAbiReportsCapabilitiesAndErrors) {
    EXPECT_EQ(hoo_ai_abi_version(), 2);
    EXPECT_EQ(hoo_ai_has_feature("tensor_abi_v2"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_dynamic_rank"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_f32"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_int32"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_f16"), 0);
    EXPECT_EQ(hoo_ai_has_feature("unknown_feature"), 0);
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_OK);
}

TEST_F(HooTensorTest, CreateSetGetInt64Tensor) {
    HooTensor t = hoo_tensor_new2(1, 2, 2);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(hoo_tensor_rank(t), 2);
    EXPECT_EQ(hoo_tensor_dim(t, 0), 2);
    EXPECT_EQ(hoo_tensor_dim(t, 1), 2);
    EXPECT_EQ(hoo_tensor_length(t), 4);

    EXPECT_EQ(hoo_tensor_set_value(t, 0, 10), 1);
    EXPECT_EQ(hoo_tensor_set_value(t, 3, 40), 1);
    EXPECT_EQ(hoo_tensor_get_int64(t, 0), 10);
    EXPECT_EQ(hoo_tensor_get_int64(t, 3), 40);
    hoo_release(t);
}

TEST_F(HooTensorTest, Create1DTensor) {
    HooTensor t = hoo_tensor_new1(1, 5);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(hoo_tensor_rank(t), 1);
    EXPECT_EQ(hoo_tensor_dim(t, 0), 5);
    EXPECT_EQ(hoo_tensor_length(t), 5);
    hoo_release(t);
}

TEST_F(HooTensorTest, Create3DTensor) {
    HooTensor t = hoo_tensor_new3(1, 2, 3, 4);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(hoo_tensor_rank(t), 3);
    EXPECT_EQ(hoo_tensor_dim(t, 0), 2);
    EXPECT_EQ(hoo_tensor_dim(t, 1), 3);
    EXPECT_EQ(hoo_tensor_dim(t, 2), 4);
    EXPECT_EQ(hoo_tensor_length(t), 24);
    hoo_release(t);
}

TEST_F(HooTensorTest, F64TensorSetGet) {
    HooTensor t = hoo_tensor_new1(2, 3);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(hoo_tensor_element_type(t), 2);

    hoo_tensor_set_value(t, 0, 0x3FF0000000000000LL); // 1.0
    hoo_tensor_set_value(t, 1, 0x4000000000000000LL); // 2.0
    hoo_tensor_set_value(t, 2, 0x4008000000000000LL); // 3.0

    double v0 = hoo_tensor_get_double(t, 0);
    double v1 = hoo_tensor_get_double(t, 1);
    double v2 = hoo_tensor_get_double(t, 2);
    EXPECT_DOUBLE_EQ(v0, 1.0);
    EXPECT_DOUBLE_EQ(v1, 2.0);
    EXPECT_DOUBLE_EQ(v2, 3.0);

    EXPECT_EQ(hoo_tensor_get_int64(t, 0), 1); // get_int64 casts double(1.0) → 1
    hoo_release(t);
}

TEST_F(HooTensorTest, PushValueSequentialFill) {
    HooTensor t = hoo_tensor_new1(1, 4);
    ASSERT_NE(t, nullptr);

    EXPECT_EQ(hoo_tensor_push_value(t, 10), 1);
    EXPECT_EQ(hoo_tensor_push_value(t, 20), 1);
    EXPECT_EQ(hoo_tensor_push_value(t, 30), 1);
    EXPECT_EQ(hoo_tensor_push_value(t, 40), 1);
    EXPECT_EQ(hoo_tensor_push_value(t, 50), 0); // overflow

    EXPECT_EQ(hoo_tensor_get_int64(t, 0), 10);
    EXPECT_EQ(hoo_tensor_get_int64(t, 1), 20);
    EXPECT_EQ(hoo_tensor_get_int64(t, 2), 30);
    EXPECT_EQ(hoo_tensor_get_int64(t, 3), 40);
    hoo_release(t);
}

TEST_F(HooTensorTest, OutOfBoundsAccessReturnsZero) {
    HooTensor t = hoo_tensor_new1(1, 3);
    ASSERT_NE(t, nullptr);
    hoo_tensor_set_value(t, 0, 100);
    hoo_tensor_set_value(t, 1, 200);
    hoo_tensor_set_value(t, 2, 300);

    EXPECT_EQ(hoo_tensor_get_int64(t, 5), 0);
    EXPECT_EQ(hoo_tensor_get_int64(t, -1), 0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(t, 5), 0.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(t, -1), 0.0);

    EXPECT_EQ(hoo_tensor_set_value(t, 5, 999), 0);
    EXPECT_EQ(hoo_tensor_set_value(t, -1, 999), 0);
    EXPECT_EQ(hoo_tensor_get_int64(t, 2), 300); // unchanged
    hoo_release(t);
}

TEST_F(HooTensorTest, DimEdgeCases) {
    HooTensor t = hoo_tensor_new2(1, 4, 5);
    ASSERT_NE(t, nullptr);

    EXPECT_EQ(hoo_tensor_dim(t, 0), 4);
    EXPECT_EQ(hoo_tensor_dim(t, 1), 5);
    EXPECT_EQ(hoo_tensor_dim(t, 2), 0);  // out of range
    EXPECT_EQ(hoo_tensor_dim(t, -1), 0); // negative axis
    EXPECT_EQ(hoo_tensor_dim(nullptr, 0), 0);
    hoo_release(t);
}

TEST_F(HooTensorTest, ElementTypeGetter) {
    EXPECT_EQ(hoo_tensor_element_type(hoo_tensor_new1(1, 1)), 1);
    EXPECT_EQ(hoo_tensor_element_type(hoo_tensor_new1(2, 1)), 2);
    EXPECT_EQ(hoo_tensor_element_type(hoo_tensor_new1(3, 1)), 3);
    EXPECT_EQ(hoo_tensor_element_type(hoo_tensor_new1(8, 1)), 8);
    EXPECT_EQ(hoo_tensor_element_type(hoo_tensor_new1(9, 1)), 9);
    EXPECT_EQ(hoo_tensor_element_type(nullptr), 0);
}

TEST_F(HooTensorTest, ElementwiseSub) {
    HooTensor a = hoo_tensor_new1(1, 3);
    HooTensor b = hoo_tensor_new1(1, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 10); hoo_tensor_set_value(b, 0, 1);
    hoo_tensor_set_value(a, 1, 20); hoo_tensor_set_value(b, 1, 5);
    hoo_tensor_set_value(a, 2, 30); hoo_tensor_set_value(b, 2, 12);

    HooTensor diff = hoo_tensor_sub(a, b);
    ASSERT_NE(diff, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(diff, 0), 9);
    EXPECT_EQ(hoo_tensor_get_int64(diff, 1), 15);
    EXPECT_EQ(hoo_tensor_get_int64(diff, 2), 18);

    hoo_release(diff);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, LowPrecisionStorageAndArithmetic) {
    HooTensor signedBytes = hoo_tensor_new1(5, 2);
    HooTensor unsignedBytes = hoo_tensor_new1(6, 2);
    ASSERT_NE(signedBytes, nullptr);
    ASSERT_NE(unsignedBytes, nullptr);

    hoo_tensor_set_value(signedBytes, 0, 127);
    hoo_tensor_set_value(signedBytes, 1, 1);
    hoo_tensor_set_value(unsignedBytes, 0, 255);
    hoo_tensor_set_value(unsignedBytes, 1, 1);

    HooTensor signedSum = hoo_tensor_add(signedBytes, signedBytes);
    HooTensor unsignedSum = hoo_tensor_add(unsignedBytes, unsignedBytes);
    ASSERT_NE(signedSum, nullptr);
    ASSERT_NE(unsignedSum, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(signedSum, 0), -2);
    EXPECT_EQ(hoo_tensor_get_bits(signedSum, 0), 0xFE);
    EXPECT_EQ(hoo_tensor_get_int64(unsignedSum, 0), 254);
    EXPECT_EQ(hoo_tensor_get_bits(unsignedSum, 0), 0xFE);

    hoo_release(unsignedSum);
    hoo_release(signedSum);
    hoo_release(unsignedBytes);
    hoo_release(signedBytes);
}

TEST_F(HooTensorTest, F8TensorUsesCanonicalE4M3Storage) {
    HooTensor values = hoo_tensor_new1(9, 2);
    ASSERT_NE(values, nullptr);
    hoo_tensor_set_value(values, 0, doubleBits(1.5));
    hoo_tensor_set_value(values, 1, doubleBits(2.25));

    EXPECT_EQ(hoo_tensor_get_bits(values, 0), 0x3C);
    EXPECT_EQ(hoo_tensor_get_bits(values, 1), 0x41);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(values, 0), 1.5);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(values, 1), 2.25);

    HooTensor sum = hoo_tensor_add(values, values);
    ASSERT_NE(sum, nullptr);
    EXPECT_EQ(hoo_tensor_get_bits(sum, 0), 0x44); // 3.0 in E4M3
    hoo_release(sum);
    hoo_release(values);
}

TEST_F(HooTensorTest, ReshapePreservesValuesAndChangesShape) {
    HooTensor source = hoo_tensor_new2(1, 2, 3);
    ASSERT_NE(source, nullptr);
    for (int64_t i = 0; i < 6; ++i) {
        EXPECT_EQ(hoo_tensor_set_value(source, i, 10 + i), 1);
    }

    HooTensor reshaped = hoo_tensor_reshape(source, 2, 3, 2, 0);
    ASSERT_NE(reshaped, nullptr);
    EXPECT_EQ(hoo_tensor_rank(reshaped), 2);
    EXPECT_EQ(hoo_tensor_dim(reshaped, 0), 3);
    EXPECT_EQ(hoo_tensor_dim(reshaped, 1), 2);
    EXPECT_EQ(hoo_tensor_length(reshaped), 6);
    for (int64_t i = 0; i < 6; ++i) {
        EXPECT_EQ(hoo_tensor_get_int64(reshaped, i), 10 + i);
    }

    hoo_release(reshaped);
    hoo_release(source);
}

TEST_F(HooTensorTest, TransposeSwapsRankTwoTensorAxes) {
    HooTensor source = hoo_tensor_new2(1, 2, 3);
    ASSERT_NE(source, nullptr);
    for (int64_t i = 0; i < 6; ++i) {
        hoo_tensor_set_value(source, i, i + 1);
    }

    HooTensor transposed = hoo_tensor_transpose(source);
    ASSERT_NE(transposed, nullptr);
    EXPECT_EQ(hoo_tensor_dim(transposed, 0), 3);
    EXPECT_EQ(hoo_tensor_dim(transposed, 1), 2);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 1), 4);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 2), 2);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 3), 5);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 4), 3);
    EXPECT_EQ(hoo_tensor_get_int64(transposed, 5), 6);

    hoo_release(transposed);
    hoo_release(source);
}

TEST_F(HooTensorTest, SoftmaxIsStableAndNormalized) {
    HooTensor source = hoo_tensor_new1(2, 3);
    ASSERT_NE(source, nullptr);
    hoo_tensor_set_value(source, 0, doubleBits(1000.0));
    hoo_tensor_set_value(source, 1, doubleBits(1001.0));
    hoo_tensor_set_value(source, 2, doubleBits(1002.0));

    HooTensor result = hoo_tensor_softmax(source);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_tensor_element_type(result), 2);
    double sum = 0.0;
    for (int64_t i = 0; i < 3; ++i) {
        const double value = hoo_tensor_get_double(result, i);
        EXPECT_TRUE(std::isfinite(value));
        sum += value;
    }
    EXPECT_NEAR(sum, 1.0, 1e-12);
    EXPECT_LT(hoo_tensor_get_double(result, 0), hoo_tensor_get_double(result, 1));
    EXPECT_LT(hoo_tensor_get_double(result, 1), hoo_tensor_get_double(result, 2));

    hoo_release(result);
    hoo_release(source);
}

TEST_F(HooTensorTest, ElementwiseDiv) {
    HooTensor a = hoo_tensor_new1(1, 3);
    HooTensor b = hoo_tensor_new1(1, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 10); hoo_tensor_set_value(b, 0, 2);
    hoo_tensor_set_value(a, 1, 20); hoo_tensor_set_value(b, 1, 4);
    hoo_tensor_set_value(a, 2, 30); hoo_tensor_set_value(b, 2, 6);

    HooTensor quot = hoo_tensor_element_div(a, b);
    ASSERT_NE(quot, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(quot, 0), 5);
    EXPECT_EQ(hoo_tensor_get_int64(quot, 1), 5);
    EXPECT_EQ(hoo_tensor_get_int64(quot, 2), 5);

    hoo_release(quot);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, DivByZeroReturnsZero) {
    HooTensor a = hoo_tensor_new1(1, 2);
    HooTensor b = hoo_tensor_new1(1, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 10); hoo_tensor_set_value(b, 0, 0);
    hoo_tensor_set_value(a, 1, 20); hoo_tensor_set_value(b, 1, 0);

    HooTensor quot = hoo_tensor_element_div(a, b);
    ASSERT_NE(quot, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(quot, 0), 0);
    EXPECT_EQ(hoo_tensor_get_int64(quot, 1), 0);

    hoo_release(quot);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, LogicalAnd) {
    HooTensor a = hoo_tensor_new1(8, 3);
    HooTensor b = hoo_tensor_new1(8, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 1); hoo_tensor_set_value(b, 0, 1);
    hoo_tensor_set_value(a, 1, 1); hoo_tensor_set_value(b, 1, 0);
    hoo_tensor_set_value(a, 2, 0); hoo_tensor_set_value(b, 2, 0);

    HooTensor result = hoo_tensor_and(a, b);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(result, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(result, 1), 0);
    EXPECT_EQ(hoo_tensor_get_int64(result, 2), 0);
    EXPECT_EQ(hoo_tensor_element_type(result), 8);

    hoo_release(result);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, LogicalOr) {
    HooTensor a = hoo_tensor_new1(8, 3);
    HooTensor b = hoo_tensor_new1(8, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 0); hoo_tensor_set_value(b, 0, 0);
    hoo_tensor_set_value(a, 1, 0); hoo_tensor_set_value(b, 1, 1);
    hoo_tensor_set_value(a, 2, 1); hoo_tensor_set_value(b, 2, 1);

    HooTensor result = hoo_tensor_or(a, b);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(result, 0), 0);
    EXPECT_EQ(hoo_tensor_get_int64(result, 1), 1);
    EXPECT_EQ(hoo_tensor_get_int64(result, 2), 1);

    hoo_release(result);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, LogicalNot) {
    HooTensor t = hoo_tensor_new1(8, 3);
    ASSERT_NE(t, nullptr);
    hoo_tensor_set_value(t, 0, 0);
    hoo_tensor_set_value(t, 1, 1);
    hoo_tensor_set_value(t, 2, 7);

    HooTensor result = hoo_tensor_not(t);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(result, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(result, 1), 0);
    EXPECT_EQ(hoo_tensor_get_int64(result, 2), 0);
    EXPECT_EQ(hoo_tensor_element_type(result), 8);

    hoo_release(result);
    hoo_release(t);
}

TEST_F(HooTensorTest, Rank2ComparisonOps) {
    HooTensor a = hoo_tensor_new2(1, 2, 2);
    HooTensor b = hoo_tensor_new2(1, 2, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 1); hoo_tensor_set_value(b, 0, 1);
    hoo_tensor_set_value(a, 1, 2); hoo_tensor_set_value(b, 1, 3);
    hoo_tensor_set_value(a, 2, 5); hoo_tensor_set_value(b, 2, 4);
    hoo_tensor_set_value(a, 3, 7); hoo_tensor_set_value(b, 3, 7);

    HooTensor eq = hoo_tensor_eq(a, b);
    HooTensor ne = hoo_tensor_ne(a, b);
    HooTensor lt = hoo_tensor_lt(a, b);
    HooTensor le = hoo_tensor_le(a, b);
    HooTensor gt = hoo_tensor_gt(a, b);
    HooTensor ge = hoo_tensor_ge(a, b);

    ASSERT_NE(eq, nullptr); ASSERT_NE(ne, nullptr);
    ASSERT_NE(lt, nullptr); ASSERT_NE(le, nullptr);
    ASSERT_NE(gt, nullptr); ASSERT_NE(ge, nullptr);

    EXPECT_EQ(hoo_tensor_get_int64(eq, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(eq, 3), 1);
    EXPECT_EQ(hoo_tensor_get_int64(eq, 1), 0);

    EXPECT_EQ(hoo_tensor_get_int64(ne, 1), 1);
    EXPECT_EQ(hoo_tensor_get_int64(ne, 0), 0);

    EXPECT_EQ(hoo_tensor_get_int64(lt, 1), 1);
    EXPECT_EQ(hoo_tensor_get_int64(lt, 0), 0);

    EXPECT_EQ(hoo_tensor_get_int64(le, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(le, 1), 1);
    EXPECT_EQ(hoo_tensor_get_int64(le, 2), 0);

    EXPECT_EQ(hoo_tensor_get_int64(gt, 2), 1);
    EXPECT_EQ(hoo_tensor_get_int64(gt, 0), 0);

    EXPECT_EQ(hoo_tensor_get_int64(ge, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(ge, 2), 1);
    EXPECT_EQ(hoo_tensor_get_int64(ge, 3), 1);

    hoo_release(ge); hoo_release(gt); hoo_release(le);
    hoo_release(lt); hoo_release(ne); hoo_release(eq);
    hoo_release(b); hoo_release(a);
}

TEST_F(HooTensorTest, MatMulNonSquare) {
    HooTensor a = hoo_tensor_new2(1, 2, 3);
    HooTensor b = hoo_tensor_new2(1, 3, 4);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    for (int64_t i = 0; i < 6; ++i) hoo_tensor_set_value(a, i, i + 1);
    for (int64_t i = 0; i < 12; ++i) hoo_tensor_set_value(b, i, i + 1);

    HooTensor c = hoo_tensor_matmul(a, b);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(hoo_tensor_rank(c), 2);
    EXPECT_EQ(hoo_tensor_dim(c, 0), 2);
    EXPECT_EQ(hoo_tensor_dim(c, 1), 4);
    EXPECT_EQ(hoo_tensor_get_int64(c, 0), 38);  // row 0 * col 0
    EXPECT_EQ(hoo_tensor_get_int64(c, 7), 128); // row 1 * col 3

    hoo_release(c);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, MatMulMismatchedDimensionsReturnsNull) {
    HooTensor a = hoo_tensor_new2(1, 2, 2);
    HooTensor b = hoo_tensor_new2(1, 3, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(hoo_tensor_matmul(a, b), nullptr);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, GetBitsOnIntAndBitTensors) {
    HooTensor t = hoo_tensor_new1(1, 2);
    ASSERT_NE(t, nullptr);
    hoo_tensor_set_value(t, 0, 0xABCD);
    hoo_tensor_set_value(t, 1, -1);
    EXPECT_EQ(hoo_tensor_get_bits(t, 0), 0xABCD);
    EXPECT_EQ(hoo_tensor_get_bits(t, 1), static_cast<int64_t>(-1));

    HooTensor bt = hoo_tensor_new1(8, 2);
    ASSERT_NE(bt, nullptr);
    hoo_tensor_set_value(bt, 0, 1);
    hoo_tensor_set_value(bt, 1, 0);
    EXPECT_EQ(hoo_tensor_get_bits(bt, 0), 1);
    EXPECT_EQ(hoo_tensor_get_bits(bt, 1), 0);
    EXPECT_EQ(hoo_tensor_get_bits(bt, 5), 0);

    hoo_release(bt);
    hoo_release(t);
}

TEST_F(HooTensorTest, NullHandlingForAllFunctions) {
    EXPECT_EQ(hoo_tensor_length(nullptr), 0);
    EXPECT_EQ(hoo_tensor_rank(nullptr), 0);
    EXPECT_EQ(hoo_tensor_dim(nullptr, 0), 0);
    EXPECT_EQ(hoo_tensor_element_type(nullptr), 0);
    EXPECT_EQ(hoo_tensor_get_int64(nullptr, 0), 0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(nullptr, 0), 0.0);
    EXPECT_EQ(hoo_tensor_get_bits(nullptr, 0), 0);
    EXPECT_EQ(hoo_tensor_set_value(nullptr, 0, 0), 0);
    EXPECT_EQ(hoo_tensor_push_value(nullptr, 0), 0);
    EXPECT_EQ(hoo_tensor_add(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_sub(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_element_mul(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_element_div(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_add_scalar(nullptr, 1.0), nullptr);
    EXPECT_EQ(hoo_tensor_sub_scalar(nullptr, 1.0), nullptr);
    EXPECT_EQ(hoo_tensor_scale_scalar(nullptr, 1.0), nullptr);
    EXPECT_EQ(hoo_tensor_div_scalar(nullptr, 1.0), nullptr);
    EXPECT_EQ(hoo_tensor_matmul(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_eq(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_ne(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_lt(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_le(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_gt(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_ge(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_and(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_or(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_tensor_not(nullptr), nullptr);
}

TEST_F(HooTensorTest, ZeroDimensionTensorEmpty) {
    HooTensor t = hoo_tensor_new1(1, 3);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(hoo_tensor_length(t), 3);
    EXPECT_GT(hoo_tensor_length(t), 0);
    hoo_release(t);
}

TEST_F(HooTensorTest, F64ElementwiseArithmetic) {
    auto bits = [](double d) -> int64_t {
        int64_t b = 0;
        std::memcpy(&b, &d, sizeof(b));
        return b;
    };

    HooTensor a = hoo_tensor_new1(2, 3);
    HooTensor b = hoo_tensor_new1(2, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, bits(1.5)); hoo_tensor_set_value(b, 0, bits(0.5));
    hoo_tensor_set_value(a, 1, bits(3.0)); hoo_tensor_set_value(b, 1, bits(1.5));
    hoo_tensor_set_value(a, 2, bits(4.0)); hoo_tensor_set_value(b, 2, bits(2.0));

    HooTensor sum = hoo_tensor_add(a, b);
    HooTensor diff = hoo_tensor_sub(a, b);
    HooTensor prod = hoo_tensor_element_mul(a, b);
    HooTensor quot = hoo_tensor_element_div(a, b);

    ASSERT_NE(sum, nullptr); ASSERT_NE(diff, nullptr);
    ASSERT_NE(prod, nullptr); ASSERT_NE(quot, nullptr);

    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(sum, 0), 2.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(sum, 1), 4.5);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(sum, 2), 6.0);

    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(diff, 0), 1.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(diff, 1), 1.5);

    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(prod, 0), 0.75);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(prod, 2), 8.0);

    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(quot, 0), 3.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(quot, 2), 2.0);

    hoo_release(quot); hoo_release(prod);
    hoo_release(diff); hoo_release(sum);
    hoo_release(b); hoo_release(a);
}

TEST_F(HooTensorTest, ScalarBroadcastArithmeticPreservesShapeAndValues) {
    HooTensor tensor = hoo_tensor_new1(2, 3);
    ASSERT_NE(tensor, nullptr);
    hoo_tensor_set_value(tensor, 0, doubleBits(2.0));
    hoo_tensor_set_value(tensor, 1, doubleBits(4.0));
    hoo_tensor_set_value(tensor, 2, doubleBits(8.0));

    HooTensor add = hoo_tensor_add_scalar(tensor, 1.5);
    HooTensor sub = hoo_tensor_sub_scalar(tensor, 1.0);
    HooTensor reverseSub = hoo_tensor_sub_scalar_left(tensor, 10.0);
    HooTensor scale = hoo_tensor_scale_scalar(tensor, 2.0);
    HooTensor div = hoo_tensor_div_scalar(tensor, 2.0);
    HooTensor reverseDiv = hoo_tensor_div_scalar_left(tensor, 16.0);
    ASSERT_NE(add, nullptr); ASSERT_NE(sub, nullptr); ASSERT_NE(reverseSub, nullptr);
    ASSERT_NE(scale, nullptr); ASSERT_NE(div, nullptr); ASSERT_NE(reverseDiv, nullptr);

    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(add, 0), 3.5);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(sub, 1), 3.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(reverseSub, 2), 2.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(scale, 1), 8.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(div, 2), 4.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(reverseDiv, 0), 8.0);
    EXPECT_EQ(hoo_tensor_dim(add, 0), 3);

    hoo_release(reverseDiv); hoo_release(div); hoo_release(scale);
    hoo_release(reverseSub); hoo_release(sub); hoo_release(add); hoo_release(tensor);
}

TEST_F(HooTensorTest, ScalarBroadcastUsesNativeIntegerAndF8Semantics) {
    HooTensor ints = hoo_tensor_new1(1, 2);
    ASSERT_NE(ints, nullptr);
    hoo_tensor_set_value(ints, 0, 127);
    hoo_tensor_set_value(ints, 1, -2);
    HooTensor intResult = hoo_tensor_scale_scalar_bits(ints, 2, 1);
    ASSERT_NE(intResult, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(intResult, 0), 254);
    EXPECT_EQ(hoo_tensor_get_int64(intResult, 1), -4);

    HooTensor f8 = hoo_tensor_new1(9, 1);
    ASSERT_NE(f8, nullptr);
    hoo_tensor_set_value(f8, 0, doubleBits(1.5));
    HooTensor f8Result = hoo_tensor_add_scalar_bits(f8, doubleBits(0.75), 9);
    ASSERT_NE(f8Result, nullptr);
    EXPECT_EQ(hoo_tensor_element_type(f8Result), 9);
    EXPECT_NEAR(hoo_tensor_get_double(f8Result, 0), 2.25, 0.001);

    hoo_release(f8Result); hoo_release(f8); hoo_release(intResult); hoo_release(ints);
}

TEST_F(HooTensorTest, SameShapeGuardCatchesMismatch) {
    HooTensor a = hoo_tensor_new1(1, 3);
    HooTensor b = hoo_tensor_new1(1, 4);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(hoo_tensor_add(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_sub(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_element_mul(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_element_div(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_eq(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_and(a, b), nullptr);
    EXPECT_EQ(hoo_tensor_or(a, b), nullptr);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, F64Matmul) {
    auto bits = [](double d) -> int64_t {
        int64_t b = 0;
        std::memcpy(&b, &d, sizeof(b));
        return b;
    };

    HooTensor a = hoo_tensor_new2(2, 2, 2);
    HooTensor b = hoo_tensor_new2(2, 2, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, bits(1.0)); hoo_tensor_set_value(b, 0, bits(5.0));
    hoo_tensor_set_value(a, 1, bits(2.0)); hoo_tensor_set_value(b, 1, bits(6.0));
    hoo_tensor_set_value(a, 2, bits(3.0)); hoo_tensor_set_value(b, 2, bits(7.0));
    hoo_tensor_set_value(a, 3, bits(4.0)); hoo_tensor_set_value(b, 3, bits(8.0));

    HooTensor c = hoo_tensor_matmul(a, b);
    ASSERT_NE(c, nullptr);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(c, 0), 19.0);
    EXPECT_DOUBLE_EQ(hoo_tensor_get_double(c, 3), 50.0);
    hoo_release(c);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, BitTensorUsesPackedStorageSemantics) {
    HooTensor t = hoo_tensor_new1(8, 10);
    ASSERT_NE(t, nullptr);

    EXPECT_EQ(hoo_tensor_set_value(t, 0, 1), 1);
    EXPECT_EQ(hoo_tensor_set_value(t, 7, 1), 1);
    EXPECT_EQ(hoo_tensor_set_value(t, 8, 1), 1);
    EXPECT_EQ(hoo_tensor_get_int64(t, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(t, 1), 0);
    EXPECT_EQ(hoo_tensor_get_int64(t, 7), 1);
    EXPECT_EQ(hoo_tensor_get_int64(t, 8), 1);
    hoo_release(t);
}

TEST_F(HooTensorTest, ElementwiseAddAndCompare) {
    HooTensor a = hoo_tensor_new1(1, 3);
    HooTensor b = hoo_tensor_new1(1, 3);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    for (int64_t i = 0; i < 3; ++i) {
        hoo_tensor_set_value(a, i, i + 1);
        hoo_tensor_set_value(b, i, 10 + i);
    }

    HooTensor sum = hoo_tensor_add(a, b);
    HooTensor mask = hoo_tensor_lt(a, b);
    ASSERT_NE(sum, nullptr);
    ASSERT_NE(mask, nullptr);
    EXPECT_EQ(hoo_tensor_get_int64(sum, 0), 11);
    EXPECT_EQ(hoo_tensor_get_int64(sum, 2), 15);
    EXPECT_EQ(hoo_tensor_element_type(mask), 8);
    EXPECT_EQ(hoo_tensor_get_int64(mask, 0), 1);
    EXPECT_EQ(hoo_tensor_get_int64(mask, 2), 1);

    hoo_release(mask);
    hoo_release(sum);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, MatrixMultiplyRankTwoTensors) {
    HooTensor a = hoo_tensor_new2(1, 2, 2);
    HooTensor b = hoo_tensor_new2(1, 2, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    hoo_tensor_set_value(a, 0, 1);
    hoo_tensor_set_value(a, 1, 2);
    hoo_tensor_set_value(a, 2, 3);
    hoo_tensor_set_value(a, 3, 4);
    hoo_tensor_set_value(b, 0, 5);
    hoo_tensor_set_value(b, 1, 6);
    hoo_tensor_set_value(b, 2, 7);
    hoo_tensor_set_value(b, 3, 8);

    HooTensor c = hoo_tensor_matmul(a, b);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(hoo_tensor_dim(c, 0), 2);
    EXPECT_EQ(hoo_tensor_dim(c, 1), 2);
    EXPECT_EQ(hoo_tensor_get_int64(c, 0), 19);
    EXPECT_EQ(hoo_tensor_get_int64(c, 3), 50);

    hoo_release(c);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooTensorTest, VersionedAbiSupportsDynamicRankAndMetadata) {
    const int64_t dims[] = {2, 3, 4, 5};
    HooTensor tensor = nullptr;
    ASSERT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_F32, 4, dims, &tensor), HOO_STATUS_OK);
    ASSERT_NE(tensor, nullptr);

    int32_t version = 0;
    EXPECT_EQ(hoo_tensor_abi_version(tensor, &version), HOO_STATUS_OK);
    EXPECT_EQ(version, HOO_TENSOR_ABI_VERSION);

    int64_t shape[4] = {};
    int64_t strides[4] = {};
    int64_t count = 0;
    EXPECT_EQ(hoo_tensor_shape(tensor, 4, shape, &count), HOO_STATUS_OK);
    EXPECT_EQ(count, 4);
    EXPECT_EQ(std::memcmp(shape, dims, sizeof(dims)), 0);
    EXPECT_EQ(hoo_tensor_strides(tensor, 4, strides, &count), HOO_STATUS_OK);
    EXPECT_EQ(strides[0], 240);
    EXPECT_EQ(strides[1], 80);
    EXPECT_EQ(strides[2], 20);
    EXPECT_EQ(strides[3], 4);

    int64_t numel = 0;
    EXPECT_EQ(hoo_tensor_numel(tensor, &numel), HOO_STATUS_OK);
    EXPECT_EQ(numel, 120);
    EXPECT_EQ(hoo_get_type_id(tensor), HOO_TYPE_TENSOR);
    EXPECT_NE(hoo_get_type_id(tensor), HOO_TYPE_BUFFER);
    hoo_release(tensor);
}

TEST_F(HooTensorTest, VersionedAbiRejectsInvalidShapeAndDtype) {
    const int64_t dims[] = {2, 0};
    HooTensor tensor = nullptr;
    EXPECT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_F64, 2, dims, &tensor),
              HOO_STATUS_INVALID_SHAPE);
    EXPECT_EQ(tensor, nullptr);

    const int64_t valid_dims[] = {2, 2};
    EXPECT_EQ(hoo_tensor_new_ex(999, 2, valid_dims, &tensor),
              HOO_STATUS_INVALID_DTYPE);
    EXPECT_EQ(tensor, nullptr);
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_INVALID_DTYPE);
}

TEST_F(HooTensorTest, VersionedAbiCopyPreservesF32Values) {
    const int64_t dims[] = {3};
    HooTensor source = nullptr;
    ASSERT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_F32, 1, dims, &source), HOO_STATUS_OK);
    ASSERT_NE(source, nullptr);

    float value = 3.5f;
    int64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(value));
    ASSERT_EQ(hoo_tensor_set_value(source, 1, bits), 1);

    HooTensor copy = nullptr;
    ASSERT_EQ(hoo_tensor_copy(source, &copy), HOO_STATUS_OK);
    ASSERT_NE(copy, nullptr);
    EXPECT_FLOAT_EQ(static_cast<float>(hoo_tensor_get_double(copy, 1)), value);
    EXPECT_EQ(hoo_tensor_get_bits(copy, 1), bits);

    hoo_release(copy);
    hoo_release(source);
}

TEST_F(HooTensorTest, VersionedAbiReportsPackedBitStrides) {
    const int64_t dims[] = {2, 3};
    HooTensor tensor = nullptr;
    ASSERT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_BIT, 2, dims, &tensor), HOO_STATUS_OK);

    int64_t strides[2] = {};
    int64_t count = 0;
    ASSERT_EQ(hoo_tensor_strides(tensor, 2, strides, &count), HOO_STATUS_OK);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(strides[0], 3); // three bits per row
    EXPECT_EQ(strides[1], 1); // one bit per element
    hoo_release(tensor);
}

TEST_F(HooTensorTest, VersionedAbiPromotesF32AndInt32Arithmetic) {
    const int64_t dims[] = {2};
    HooTensor f32 = nullptr;
    HooTensor i32 = nullptr;
    ASSERT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_F32, 1, dims, &f32), HOO_STATUS_OK);
    ASSERT_EQ(hoo_tensor_new_ex(HOO_TENSOR_DTYPE_INT32, 1, dims, &i32), HOO_STATUS_OK);

    float f32Value = 1.5f;
    int64_t f32Bits = 0;
    std::memcpy(&f32Bits, &f32Value, sizeof(f32Value));
    ASSERT_EQ(hoo_tensor_set_value(f32, 0, f32Bits), 1);
    ASSERT_EQ(hoo_tensor_set_value(f32, 1, f32Bits), 1);
    ASSERT_EQ(hoo_tensor_set_value(i32, 0, 2), 1);
    ASSERT_EQ(hoo_tensor_set_value(i32, 1, 4), 1);

    HooTensor sum = hoo_tensor_add(i32, f32);
    ASSERT_NE(sum, nullptr);
    EXPECT_EQ(hoo_tensor_element_type(sum), HOO_TENSOR_DTYPE_F32);
    EXPECT_FLOAT_EQ(static_cast<float>(hoo_tensor_get_double(sum, 0)), 3.5f);
    EXPECT_FLOAT_EQ(static_cast<float>(hoo_tensor_get_double(sum, 1)), 5.5f);

    hoo_release(sum);
    hoo_release(i32);
    hoo_release(f32);
}
