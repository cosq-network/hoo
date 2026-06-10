#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_character.h"

using namespace hooc;

class HooCharacterApiJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooCharacterApiJitTest, FromCodepoint) {
    const std::string source = R"(
        func :int64 test() {
            var ch = Character.fromCodepoint(65);
            return Character.codepoint(ch);
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 65);
}

TEST_F(HooCharacterApiJitTest, CharacterLength) {
    const std::string source = R"(
        func :int64 test() {
            var ch = Character.fromCodepoint(8364);
            return Character.length(ch);
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}
