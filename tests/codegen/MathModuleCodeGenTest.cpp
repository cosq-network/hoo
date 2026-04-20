#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <cmath>
#include "src/jit/HoocJIT.h"

using namespace hooc;

class MathModuleCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(MathModuleCodeGenTest, MathSqrt) {
    std::string code = R"(func:double main() { return sqrt(16.0); })";
    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;
    auto execResult = jit->executeFunction<double>("main");
    ASSERT_TRUE(execResult.success) << jit->getLastError();
    EXPECT_DOUBLE_EQ(execResult.value, 4.0);
}

TEST_F(MathModuleCodeGenTest, MathClamp) {
    std::string code = R"(func:double main() { return clamp(15.0, 0.0, 10.0); })";
    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;
    auto execResult = jit->executeFunction<double>("main");
    ASSERT_TRUE(execResult.success) << jit->getLastError();
    EXPECT_DOUBLE_EQ(execResult.value, 10.0);
}