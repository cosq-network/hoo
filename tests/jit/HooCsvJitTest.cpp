#include <gtest/gtest.h>
#include <unistd.h>
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
        func :int64 test() { return Csv.escape(44); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, EscapeQuote) {
    const std::string source = R"(
        func :int64 test() { return Csv.escape(34); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, EscapeNormal) {
    const std::string source = R"(
        func :int64 test() { return Csv.escape(97); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooCsvJitTest, ReadFile) {
    // Create a temp CSV file
    char tmp_path[] = "/tmp/hoo_test_csv_XXXXXX";
    int fd = mkstemp(tmp_path);
    ASSERT_GT(fd, -1);
    const char* csv = "name,age\nAlice,30\nBob,25\n";
    ASSERT_EQ(write(fd, csv, strlen(csv)), static_cast<ssize_t>(strlen(csv)));
    close(fd);

    std::string source = std::string(R"(
        func :int64 test() {
            return Csv.readFile(")") + tmp_path + R"(");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
    unlink(tmp_path);
}

TEST_F(HooCsvJitTest, ReadFileNotFound) {
    const std::string source = R"(
        func :int64 test() {
            return Csv.readFile("/tmp/hoo_nonexistent_csv_file.csv");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}
