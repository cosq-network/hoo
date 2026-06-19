#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooCsvJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};

    static std::filesystem::path tempCsvPath(const std::string& prefix) {
        auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(stamp) + ".csv");
    }
};

TEST_F(HooCsvJitTest, EscapeComma) {
    const std::string source = R"(
        func :int64 test() {
            var csv = new Csv();
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
            var csv = new Csv();
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
            var csv = new Csv();
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
            var csv = new Csv();
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
            var csv = csv_fromOpts(59, 39);
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
            var csv = new Csv();
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
    const auto tmp_path = tempCsvPath("hoo_test_csv_jit_read");
    {
        std::ofstream out(tmp_path, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out << "name,age\nAlice,30\nBob,25\n";
    }

    std::string hooc_path = tmp_path.generic_string();
    std::string source = std::string(R"(
        func :int64 test() {
            var csv = new Csv();
            var rows = csv.readFile(")") + hooc_path + R"(");
            csv.release();
            return Array.length(rows);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
    std::filesystem::remove(tmp_path);
}

TEST_F(HooCsvJitTest, ReadFileNotFound) {
    const auto missing_path = tempCsvPath("hoo_nonexistent_csv_file");
    std::filesystem::remove(missing_path);
    const std::string source = std::string(R"(
        func :int64 test() {
            var csv = new Csv();
            var rows = csv.readFile(")") + missing_path.generic_string() + R"(");
            csv.release();
            if rows == 0 { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooCsvJitTest, WriteFile) {
    const auto tmp_path = tempCsvPath("hoo_csv_jit_write");
    const std::string hooc_path = tmp_path.generic_string();
    std::string source = std::string(R"(
        func :int64 test() {
            var csv = new Csv();
            var data = [["x", "y"], ["10", "20"]];
            var ok = csv.writeFile(")") + hooc_path + R"(", data);
            csv.release();
            return ok;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
    std::filesystem::remove(tmp_path);
}
