#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooPathJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooPathJitTest, Dirname) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var d = Path.dirname("a/b/c.txt");
            return d.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooPathJitTest, Basename) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var b = Path.basename("a/b/c.txt");
            return b.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooPathJitTest, Extension) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var e = Path.extension("a/b/c.txt");
            return e.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 4);
}

TEST_F(HooPathJitTest, Stem) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var s = Path.stem("a/b/c.txt");
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, Join) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var p = Path.join("a", "b");
            return p.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooPathJitTest, IsAbsolute) {
#ifdef _WIN32
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.is_absolute("C:\\usr\\bin"); }
    )";
#else
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.is_absolute("/usr/bin"); }
    )";
#endif
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, IsRelative) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.is_relative("foo/bar"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, HasExtension) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.has_extension("file.txt"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, Normalize) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var p = Path.normalize("a/b/../c");
            return p.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooPathJitTest, Separator) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.separator(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
#ifdef _WIN32
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), static_cast<int64_t>('\\'));
#else
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), static_cast<int64_t>('/'));
#endif
}

TEST_F(HooPathJitTest, ListSeparator) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return Path.list_separator(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
#ifdef _WIN32
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), static_cast<int64_t>(';'));
#else
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), static_cast<int64_t>(':'));
#endif
}

TEST_F(HooPathJitTest, Relative) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var rel = Path.relative("/a/b/c/d", "/a/b");
            return rel.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooPathJitTest, Absolute) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var abs = Path.absolute(".");
            return abs.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}
