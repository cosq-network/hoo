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
            var d = path_dirname("a/b/c.txt");
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
            var b = path_basename("a/b/c.txt");
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
            var e = path_extension("a/b/c.txt");
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
            var s = path_stem("a/b/c.txt");
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
            var p = path_join("a", "b");
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
        func :int64 test() { return path_is_absolute("C:\\usr\\bin"); }
    )";
#else
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return path_is_absolute("/usr/bin"); }
    )";
#endif
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, IsRelative) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return path_is_relative("foo/bar"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, HasExtension) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return path_has_extension("file.txt"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, Normalize) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var p = path_normalize("a/b/../c");
            return p.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooPathJitTest, Separator) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() { return path_separator(); }
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
        func :int64 test() { return path_list_separator(); }
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
            var rel = path_relative("/a/b/c/d", "/a/b");
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
            var abs = path_absolute(".");
            return abs.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooPathJitTest, Filename) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var f = path_filename("a/b/c.txt");
            if (!f) { return 0; }
            if (!f.equals("c.txt")) { return 0; }
            return f.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooPathJitTest, Parent) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var p = path_parent("a/b/c.txt");
            if (!p) { return 0; }
            if (!p.equals("a/b")) { return 0; }
            return p.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooPathJitTest, Root) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var r = path_root("a/b/c.txt");
            if (!r) { return 0; }
            return r.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooPathJitTest, SplitReturnsParts) {
    const std::string source = R"(
        import hoo.path;
        import hoo;
        func :int64 test() {
            var parts = path_split("foo/bar/baz");
            if (!parts) { return 0; }
            if (parts.length() != 3) { return 0; }
            var p0: string = parts.getString(0);
            var p2: string = parts.getString(2);
            if (!p0.equals("foo")) { return 0; }
            if (!p2.equals("baz")) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooPathJitTest, ExtensionNoDotReturnsEmptyNotNull) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var e = path_extension("file");
            if (!e) { return 0; }
            return e.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooPathJitTest, StemNoDotReturnsFilenameNotNull) {
    const std::string source = R"(
        import hoo.path;
        func :int64 test() {
            var s = path_stem("file");
            if (!s) { return 0; }
            if (!s.equals("file")) { return 0; }
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 4);
}
