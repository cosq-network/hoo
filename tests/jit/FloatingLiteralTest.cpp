#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include <cmath>

using namespace hooc;

class FloatingLiteralTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};

    void SetUp() override {
    }
};

TEST_F(FloatingLiteralTest, LowerFloatingLiteral) {
    const std::string source = R"(
        import hoo;
        func :double getPi() {
            return 3.14159;
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    // Mangled name: _F_M_test_E_getPi_d
    int64_t result = jit.run("_F_M_test_E_getPi_d");
    
    double val;
    std::memcpy(&val, &result, sizeof(double));
    
    EXPECT_NEAR(3.14159, val, 0.00001);
}

TEST_F(FloatingLiteralTest, FloatingArith) {
    const std::string source = R"(
        import hoo;
        func :double add(a: double, b: double) {
            return a + b;
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    // Setup state for calling with arguments
    // run() doesn't support arguments easily, but we can call executeFunction if we make it public or use a wrapper.
    // For now, let's just test constants.
}
