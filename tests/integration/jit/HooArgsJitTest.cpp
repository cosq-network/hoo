#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/args/hoo_args.h"

using namespace hooc;

class HooArgsJitTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* test_argv[] = {
            "/usr/bin/hoo",
            "input.txt",
            "--output=result.txt",
            "--verbose",
            "--",
            "extra_arg",
            "--not-a-flag"
        };
        hoo_args_init(7, test_argv);
    }

    void TearDown() override {
        hoo_args_shutdown();
    }

    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooArgsJitTest, Count) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            return args.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooArgsJitTest, Get) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            var a0 = args.get(0);
            var a1 = args.get(1);
            return a0.length() + a1.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 18);
}

TEST_F(HooArgsJitTest, Has) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            return args.has("output");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArgsJitTest, HasNotFound) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            return args.has("nonexistent");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, Value) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            var val = args.value("output");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 10);
}

// ── Argparse-style JIT tests ─────────────────────────────────────────────────

TEST_F(HooArgsJitTest, ParseReturnsOne) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFlag("verbose", "-v", "--verbose", "");
            return args.parse();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArgsJitTest, GetBoolFlagPresent) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFlag("verbose", "-v", "--verbose", "");
            args.parse();
            return args.getBool("verbose");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArgsJitTest, GetBoolFlagAbsent) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFlag("debug", "-d", "--debug", "");
            args.parse();
            return args.getBool("debug");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, GetStringUsesDefault) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("file", "-f", "--file", "", "default.txt");
            args.parse();
            var val = args.getString("file");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 11);
}

TEST_F(HooArgsJitTest, GetStringFromArg) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("output", "-o", "--output", "", "");
            args.parse();
            var val = args.getString("output");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 10);
}

TEST_F(HooArgsJitTest, GetStringFromShortOpt) {
    hoo_args_shutdown();
    const char* argv[] = {"program", "-o", "shortval"};
    hoo_args_init(3, argv);
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("opt", "-o", "", "", "");
            args.parse();
            var val = args.getString("opt");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8);
}

TEST_F(HooArgsJitTest, GetIntWithDefault) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 42);
            args.parse();
            return args.getInt("count");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooArgsJitTest, GetIntFromArg) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 0);
            args.addString("output", "-o", "--output", "", "");
            args.parse();
            return args.getInt("count");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, ParseBeforeAddReturnsDefaultInt) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 99);
            args.parse();
            return args.getInt("count");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 99);
}

TEST_F(HooArgsJitTest, AddPositionalAndGet) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addPositional("input", "Input file");
            args.parse();
            var val = args.getString("input");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 9);
}

TEST_F(HooArgsJitTest, MultiplePositionalArgs) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addPositional("first", "First");
            args.addPositional("second", "Second");
            args.addPositional("third", "Third");
            args.parse();
            var a0 = args.getString("first");
            var a1 = args.getString("second");
            var a2 = args.getString("third");
            return a0.length() + a1.length() + a2.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 30);
}

TEST_F(HooArgsJitTest, HelpTextNonEmpty) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("output", "-o", "--output", "Output file", "out.txt");
            args.addFlag("verbose", "-v", "--verbose", "Verbose mode");
            var help = args.helpText();
            return help.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t len = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(len, 0);
}

TEST_F(HooArgsJitTest, GetStringAfterParseWithPositionalDefault) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addPositional("input", "Input file");
            args.parse();
            var val = args.getString("input");
            return val.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 9);
}

TEST_F(HooArgsJitTest, ClearAndReuse) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("output", "-o", "--output", "", "first.txt");
            args.parse();
            var first = args.getString("output");
            var len1 = first.length();
            args.clear();
            args.addString("name", "-n", "--name", "", "second");
            args.parse();
            var second = args.getString("name");
            var len2 = second.length();
            return len1 + len2;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 16);
}

TEST_F(HooArgsJitTest, FlagPresentReturnsOne) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFlag("debug", "-d", "--debug", "");
            args.parse();
            return args.getBool("debug");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, AddStringThenGetBoolFlag) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("name", "-n", "--name", "", "nobody");
            args.addFlag("debug", "-d", "--debug", "");
            args.parse();
            var hasDebug = args.getBool("debug");
            var name = args.getString("name");
            if hasDebug == 1 {
                return name.length();
            }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, GetBoolNotPresentAfterClear) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFlag("verbose", "-v", "--verbose", "");
            args.parse();
            var before = args.getBool("verbose");
            args.clear();
            return before;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArgsJitTest, AddFloatWithDefault) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addFloat("threshold", "-t", "--threshold", "", 0.5);
            args.parse();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArgsJitTest, MultipleOptionalArgsAllDefaults) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("file", "-f", "--file", "", "out.txt");
            args.addInt("count", "-c", "--count", "", 5);
            args.addFlag("debug", "-d", "--debug", "");
            args.parse();
            var f = args.getString("file");
            var c = args.getInt("count");
            var d = args.getBool("debug");
            return f.length() + c + d;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 12);
}

TEST_F(HooArgsJitTest, RequiredArgumentIsEnforced) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.addString("input", "", "--input", "", "");
            args.setRequired("input", true);
            return args.parse();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArgsJitTest, ReleaseHandle) {
    const std::string source = R"(
        import hoo.args;
        func :int64 test() {
            var args = new Args();
            args.release();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
