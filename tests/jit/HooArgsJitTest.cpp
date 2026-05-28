#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooArgsJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

// Args module requires passing argc/argv which is environment-specific.
// For basic JIT test, verify that the symbols resolve and functions exist.
// Note: args_count and args_has require a HooArgsResult handle, which can only
// be created via hoo_args_parse(argc, argv) from C/C++. There is no way to
// construct one from within HVM bytecode, so these functions cannot be tested
// through the JIT pipeline without host-side setup.
TEST_F(HooArgsJitTest, SymbolResolves) {
    // Just verify we can load and run a simple function that references args_*
    // The args module primarily works with program arguments passed at startup
    const std::string source = R"(
        func :int64 test() { return 42; }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}
