#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/text/hoo_character.h"
#include "runtime/lib/core/hoo_runtime.h"

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

TEST_F(HooCharacterApiJitTest, FromUtf8) {
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = character_from_utf8("€");
            return ch.codepoint();
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0x20AC);
}

TEST_F(HooCharacterApiJitTest, TempLeakProbe) {
    hoo_reset_memory_stats();
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var ch = character_from_utf8("abcde");
            return ch.codepoint();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 97);
    hoo_print_memory_stats();
}

TEST_F(HooCharacterApiJitTest, TempLeakProbeForIn) {
    hoo_reset_memory_stats();
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var n = 0;
            for c in "abcde" { n += 1; }
            return n;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
    hoo_print_memory_stats();
}

TEST_F(HooCharacterApiJitTest, TempLeakProbeForInCodepoint) {
    hoo_reset_memory_stats();
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var n = 0;
            for c in "abcde" { n += c.codepoint(); }
            return n;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 97 + 98 + 99 + 100 + 101);
    hoo_print_memory_stats();
}

TEST_F(HooCharacterApiJitTest, TempLeakProbeExprStmt) {
    hoo_reset_memory_stats();
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            'A';
            'B';
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
    hoo_print_memory_stats();
}

TEST_F(HooCharacterApiJitTest, TempLeakProbeArrayLit) {
    hoo_reset_memory_stats();
    const std::string source = R"(
        import hoo.character;
        func :int64 test() {
            var a = ['A', 'B', 'C'];
            return a.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
    hoo_print_memory_stats();
}
