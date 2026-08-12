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

class WhileLoopCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        hooExe = HOO_EXECUTABLE;
    }

    std::string createSource(const std::string& source) {
        static int counter = 0;
        const std::string path = tempDir + "/hoo_while_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_while_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
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

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopCountsToTen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            while (i < 10) {
                i = i + 1;
            }
            return i;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopSumsElements) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values = [10, 20, 30, 40];
            var total: int64 = 0;
            var i: int64 = 0;
            while (i < 4) {
                total = total + values[i];
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("100"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithFalseConditionNeverRuns) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var ran: int64 = 0;
            while (false) {
                ran = 1;
            }
            return ran;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithBreak) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 100) {
                if (i == 5) {
                    break;
                }
                total = total + i;
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithContinue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 6) {
                i = i + 1;
                if (i % 2 == 0) {
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

TEST_F(WhileLoopCLIIntegrationTest, NestedWhileLoops) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 3) {
                var j: int64 = 0;
                while (j < 3) {
                    total = total + 1;
                    j = j + 1;
                }
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("9"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithIfElseInside) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 10) {
                if (i % 2 == 0) {
                    total = total + i;
                } else {
                    total = total - i;
                }
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("-5"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopPrintsEachIteration) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.io;
        func :int64 main() {
            var i: int64 = 0;
            while (i < 3) {
                print("*");
                i = i + 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("***"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopRunsAtLeastOnce) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            do {
                i = i + 1;
            } while (false);
            return i;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopCountsToTen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            do {
                i = i + 1;
            } while (i < 10);
            return i;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopWithBreak) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            do {
                i = i + 1;
                if (i == 5) {
                    break;
                }
                total = total + i;
            } while (true);
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopWithContinue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            do {
                i = i + 1;
                if (i == 3) {
                    continue;
                }
                total = total + i;
            } while (i < 5);
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("12"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopPrintsEachIteration) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.io;
        func :int64 main() {
            var i: int64 = 0;
            do {
                print("#");
                i = i + 1;
            } while (i < 4);
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("####"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithFloatCondition) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var value: double = 0.5;
            var count: int64 = 0;
            while (value < 5.0) {
                value = value + 1.5;
                count = count + 1;
            }
            return count;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopWithBoolCondition) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var active: bool = true;
            var iterations: int64 = 0;
            while (active) {
                iterations = iterations + 1;
                if (iterations == 3) {
                    active = false;
                }
            }
            return iterations;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, WhileLoopArrayTraversal) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var values = [2, 4, 6, 8];
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 4) {
                total = total + values[i];
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(WhileLoopCLIIntegrationTest, DoWhileLoopNestedInWhile) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 3) {
                var j: int64 = 0;
                do {
                    total = total + 1;
                    j = j + 1;
                } while (j < 2);
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("6"), std::string::npos);
}
