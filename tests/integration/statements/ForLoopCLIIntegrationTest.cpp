#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef HOO_EXECUTABLE
#error "HOO_EXECUTABLE must be defined via CMake -D"
#endif

class ForLoopCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        for (char& c : tempDir) {
            if (c == '\\') c = '/';
        }
        hooExe = HOO_EXECUTABLE;
    }

    std::string createSource(const std::string& source) {
        static int counter = 0;
        const std::string path = tempDir + "/hoo_for_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_for_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
#else
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
#else
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
#endif
#endif
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return {"popen failed", -1};

        std::ostringstream output;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe)) output << buffer;

        const int status = pclose(pipe);
#ifdef _WIN32
        return {output.str(), status};
#else
        return {output.str(), WIFEXITED(status) ? WEXITSTATUS(status) : -1};
#endif
    }

    ExecResult compileAndRun(const std::string& source) {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = createArchive();
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\"");
    }
};

TEST_F(ForLoopCLIIntegrationTest, ForInArraySumsElements) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values = [10, 20, 30, 40];
            var total: int64 = 0;
            for item in values {
                total = total + item;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("100"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForInArrayPrintsEachElement) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.io;
        func :int64 main() {
            var values = [1, 2, 3];
            for item in values {
                print("x");
            }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("xxx"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForInEmptyArrayDoesNotExecute) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values: int64[] = [];
            var visits: int64 = 0;
            for item in values {
                visits = visits + 1;
            }
            return visits;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForInStringCountsCharacters) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var text: string = "hello";
            var count: int64 = 0;
            for ch in text {
                count = count + 1;
            }
            return count;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("5"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForInMapIteratesKeys) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values = new Map(2, 1);
            values.setInt64Int64(10, 1);
            values.setInt64Int64(20, 1);
            values.setInt64Int64(30, 1);
            var total: int64 = 0;
            for key in values {
                total = total + key;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("60"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForInDoubleArrayUsesValues) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values: double[] = [1.5, 2.5, 3.0];
            var total: double = 0.0;
            for value in values {
                total = total + value;
            }
            if total == 7.0 {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeDefaultStepIsExclusive) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 0..10 {
                total = total + i;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("45"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeWithPositiveStep) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 0..10 by 2 {
                total = total + i;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeWithVariableBoundsAndStep) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 sumRange(start: int64, finish: int64, step: int64) {
            var total: int64 = 0;
            for i in start..finish by step {
                total = total + i;
            }
            return total;
        }
        func :int64 main() {
            return sumRange(2, 9, 3);
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeBoundsCanBeExpressions) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in (1 + 1)..(3 + 6) by (1 + 2) {
                total = total + i;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeEqualBoundsIsEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var visits: int64 = 0;
            for i in 5..5 {
                visits = visits + 1;
            }
            return visits;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeSupportsBreak) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 0..10 {
                if i == 5 {
                    break;
                }
                total = total + i;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForRangeSupportsContinue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 0..6 {
                if i % 2 == 0 {
                    continue;
                }
                total = total + i;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("9"), std::string::npos);
}

TEST_F(ForLoopCLIIntegrationTest, ForLoopVariableDoesNotEscapeBody) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 1..4 {
                var doubled = i * 2;
                total = total + doubled;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("12"), std::string::npos);
}
