#include <gtest/gtest.h>
#include "runtime/lib/hoo_io.h"
#include "runtime/lib/hoo_string.h"

class HooIOTest : public ::testing::Test {
};

TEST_F(HooIOTest, BasicPrint) {
    HooString str = hoo_string_from_cstr("Hello IO");
    // We can't easily capture stdout/stderr in this environment without complex mocking,
    // but we can call them to ensure no crashes and coverage.
    hoo_print(str);
    hoo_println(str);
    hoo_println(nullptr);
    
    hoo_string_release(str);
}

TEST_F(HooIOTest, ReadChar) {
    // Basic test to ensure it returns something (likely -1 if no input)
    // int64_t ch = hoo_readchar();
    // EXPECT_GE(ch, -1);
}
