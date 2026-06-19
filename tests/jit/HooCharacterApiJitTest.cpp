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

TEST_F(HooCharacterApiJitTest, New) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = new Character(65);
            return ch.codepoint();
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 65);
}

TEST_F(HooCharacterApiJitTest, Length) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = new Character(8364);
            return ch.length();
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooCharacterApiJitTest, Data) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = new Character(65);
            var d = ch.data();
            return d.length();
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCharacterApiJitTest, DataEquals) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = new Character(65);
            var d = ch.data();
            return d.equals("A");
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCharacterApiJitTest, CodepointAfterLength) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = new Character(128512);
            var len = ch.length();
            var cp = ch.codepoint();
            return cp;
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 128512);
}
