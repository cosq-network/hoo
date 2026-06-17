#include <gtest/gtest.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cstring>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooCsvJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooCsvJitTest, EscapeComma) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var r = csv.escape(44);
            csv.release();
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, EscapeQuote) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var r = csv.escape(34);
            csv.release();
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, EscapeNormal) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var r = csv.escape(97);
            csv.release();
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooCsvJitTest, ParseBasic) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var input = "a,b,c\n1,2,3";
            var rows = csv.parse(input);
            csv.release();
            return Array.length(rows);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooCsvJitTest, ParseCustomOpts) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.newWithOpts(59, 39);
            var input = "a;b;c";
            var rows = csv.parse(input);
            csv.release();
            return Array.length(rows);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, GenerateBasic) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var data = [["a", "b"], ["1", "2"]];
            var result = csv.generate(data);
            csv.release();
            return result.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 7);
}

TEST_F(HooCsvJitTest, ReadFile) {
    char tmp_path[] = "/tmp/hoo_test_csv_XXXXXX";
    int fd = mkstemp(tmp_path);
    ASSERT_GT(fd, -1);
    const char* csv = "name,age\nAlice,30\nBob,25\n";
    ASSERT_EQ(write(fd, csv, strlen(csv)), static_cast<ssize_t>(strlen(csv)));
    close(fd);

    std::string hooc_path(tmp_path);
#ifdef _WIN32
    std::replace(hooc_path.begin(), hooc_path.end(), '\\', '/');
#endif
    std::string source = std::string(R"(
        func :int64 test() {
            var csv = Csv.new();
            var rows = csv.readFile(")") + hooc_path + R"(");
            csv.release();
            return Array.length(rows);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
    unlink(tmp_path);
}

TEST_F(HooCsvJitTest, ReadFileNotFound) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var rows = csv.readFile("/tmp/hoo_nonexistent_csv_file.csv");
            csv.release();
            if rows == 0 { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, WriteFile) {
    const std::string source = R"(
        func :int64 test() {
            var csv = Csv.new();
            var data = [["x", "y"], ["10", "20"]];
            var ok = csv.writeFile("/tmp/hoo_csv_jit_write_test.csv", data);
            csv.release();
            return ok;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
    std::remove("/tmp/hoo_csv_jit_write_test.csv");
}
