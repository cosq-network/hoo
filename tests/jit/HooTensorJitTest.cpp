#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include "src/hvm/HVMJIT.h"
#include "src/core/DefaultIOProvider.h"

using namespace hooc;

class HooTensorJitTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
    }

    double bitsToDouble(int64_t bits) {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

TEST_F(HooTensorJitTest, TensorLiteralIndexing) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var values = [10, 20, 30]t;
            return values[0] + values[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 40) << jit->getLastError();
}

TEST_F(HooTensorJitTest, DeclaredTensorAllocatesZeroFilledStorage) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var values: tensor<int64>[3];
            return values[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 0) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorF64LiteralAndIndexing) {
    const std::string code = R"(
        import hoo;
        func :double test() {
            var values: tensor<f64>[2] = [1.25, 2.5]t;
            return values[0] + values[1];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_NEAR(bitsToDouble(jit->run("_F_test_d")), 3.75, 0.00001) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorParameterAndReturn) {
    const std::string code = R"(
        import hoo;
        func :tensor<int64>[2] id(x: tensor<int64>[2]) {
            return x;
        }

        func :int64 test() {
            var values = [9, 8]t;
            var copied = id(values);
            return copied[1];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorElementwiseAddAndMultiply) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var sum = a + b;
            var product = a .* b;
            return sum[2] + product[1];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 19) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorElementwiseSubAndDiv) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [10, 20, 30]t;
            var b = [1, 4, 6]t;
            var diff = a - b;
            var quot = a ./ b;
            return diff[1] + quot[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 21) << jit->getLastError(); // (20-4) + (30/6) = 16 + 5 = 21
}

TEST_F(HooTensorJitTest, TensorComparisonAndBitLogic) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [1, 5, 3]t;
            var b = [2, 4, 3]t;
            var lt = a < b;
            var eq = a == b;
            var mask = lt || eq;
            var inverted = !mask;
            return mask[0] + mask[2] + inverted[1];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 3) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorMatrixMultiply) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [[1, 2], [3, 4]]t;
            var b = [[5, 6], [7, 8]]t;
            var c = a * b;
            return c[0] + c[3];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 69) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorMatMulNonSquare) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [[1, 2, 3], [4, 5, 6]]t;
            var b = [[7, 8], [9, 10], [11, 12]]t;
            var c = a * b;
            return c[0] + c[3];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // [[1,2,3],[4,5,6]] * [[7,8],[9,10],[11,12]] = [[58,64],[139,154]]
    // c[0]=58, c[3]=154 => 212
    EXPECT_EQ(jit->run("_F_test_i8"), 212) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorAllComparisonOps) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [1, 2, 3]t;
            var b = [1, 3, 2]t;
            var lt  = a < b;
            var le  = a <= b;
            var gt  = a > b;
            var ge  = a >= b;
            var eq  = a == b;
            var ne  = a != b;
            return lt[0] + lt[2] + le[1] + gt[1] + ge[2] + eq[0] + ne[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // lt:  [0, 1, 0] -> lt[0]=0, lt[2]=0
    // le:  [1, 1, 0] -> le[1]=1
    // gt:  [0, 0, 1] -> gt[1]=0
    // ge:  [1, 0, 1] -> ge[2]=1
    // eq:  [1, 0, 0] -> eq[0]=1
    // ne:  [0, 1, 1] -> ne[2]=1
    // sum: 0 + 0 + 1 + 0 + 1 + 1 + 1 = 4
    EXPECT_EQ(jit->run("_F_test_i8"), 4) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorBitLiteralAndAnd) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [1, 0, 1]t;
            var b = [1, 1, 0]t;
            var and = a && b;
            return and[0] + and[1] + and[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError(); // only position 0 is 1
}

TEST_F(HooTensorJitTest, TensorOrAndNot) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [1, 0]t;
            var b = [0, 0]t;
            var or = a || b;
            var not = !or;
            return or[0] + or[1] + not[0] + not[1];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // or: [1, 0], not: [0, 1]
    // sum: 1 + 0 + 0 + 1 = 2
    EXPECT_EQ(jit->run("_F_test_i8"), 2) << jit->getLastError();
}

TEST_F(HooTensorJitTest, Tensor2DLinearIndexing) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var m = [[1, 2], [3, 4]]t;
            return m[0] + m[3];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 5) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorDeclaredWithBitElement) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var mask: tensor<bit>[4] = [1, 0, 1, 0]t;
            return mask[0] + mask[1] + mask[2] + mask[3];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 2) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorF64ElementwiseArithmetic) {
    const std::string code = R"(
        import hoo;
        func :double test() {
            var a: tensor<f64>[3] = [1.5, 3.0, 4.0]t;
            var b: tensor<f64>[3] = [0.5, 1.5, 2.0]t;
            var sum = a + b;
            var diff = a - b;
            return sum[0] + diff[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // sum[0] = 2.0, diff[2] = 2.0, total = 4.0
    EXPECT_NEAR(bitsToDouble(jit->run("_F_test_d")), 4.0, 0.00001) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorF64Matmul) {
    const std::string code = R"(
        import hoo;
        func :double test() {
            var a: tensor<f64>[2, 2] = [[1.0, 2.0], [3.0, 4.0]]t;
            var b: tensor<f64>[2, 2] = [[5.0, 6.0], [7.0, 8.0]]t;
            var c = a * b;
            return c[0] + c[3];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // c[0]=19.0, c[3]=50.0 => 69.0
    EXPECT_NEAR(bitsToDouble(jit->run("_F_test_d")), 69.0, 0.00001) << jit->getLastError();
}

TEST_F(HooTensorJitTest, MultipleTensorOperationsChained) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [2, 4, 6]t;
            var b = [1, 2, 3]t;
            var c = [10, 20, 30]t;
            var t1 = a + b;
            var t2 = c - t1;
            var result = t2 ./ b;
            return result[0] + result[1] + result[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // t1 = [3, 6, 9], t2 = [7, 14, 21], result = [7, 7, 7]
    // sum = 21
    EXPECT_EQ(jit->run("_F_test_i8"), 21) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorWithNegativeValues) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var a = [-5, 3, -2]t;
            var b = [2, -1, 4]t;
            var sum = a + b;
            return sum[0] + sum[1] + sum[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    // sum = [-3, 2, 2] => 1
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(HooTensorJitTest, TensorReturnTypePassThrough) {
    const std::string code = R"(
        import hoo;
        func :tensor<int64>[3] doubleIt(x: tensor<int64>[3]) {
            return x + x;
        }

        func :int64 test() {
            var v = [1, 2, 3]t;
            var d = doubleIt(v);
            return d[0] + d[2];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError(); // (1+1)+(3+3) = 8
}

TEST_F(HooTensorJitTest, Tensor3DLiteralAndLinearIndexing) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var t3 = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]t;
            return t3[0] + t3[7];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 9) << jit->getLastError(); // 1 + 8 = 9
}

TEST_F(HooTensorJitTest, Tensor3DDeclaredType) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var t3: tensor<int64>[2, 2, 2] = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]t;
            return t3[3] + t3[5];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 10) << jit->getLastError(); // 4 + 6 = 10
}

TEST_F(HooTensorJitTest, Tensor3DZeroFilled) {
    const std::string code = R"(
        import hoo;
        func :int64 test() {
            var t3: tensor<int64>[2, 3, 4];
            return t3[0] + t3[11] + t3[23];
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 0) << jit->getLastError();
}
