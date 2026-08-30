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

class IfStatementCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_if_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_if_cli_"
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

TEST_F(IfStatementCLIIntegrationTest, SimpleIfTrueExecutesThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (true) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, SimpleIfFalseSkipsThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (false) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseTrueExecutesThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (true) {
                return 10;
            } else {
                return 20;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseFalseExecutesElse) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (false) {
                return 10;
            } else {
                return 20;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfFirstConditionTrue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            if (x == 1) {
                return 10;
            } else {
                if (x == 2) {
                    return 20;
                }
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfSecondConditionTrue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            if (x == 1) {
                return 10;
            } else {
                if (x == 2) {
                    return 20;
                }
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfElseFirstConditionTrue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            if (x == 1) {
                return 10;
            } else {
                if (x == 2) {
                    return 20;
                } else {
                    return 30;
                }
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfElseSecondConditionTrue) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            if (x == 1) {
                return 10;
            } else {
                if (x == 2) {
                    return 20;
                } else {
                    return 30;
                }
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfElseAllFalse) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 99;
            if (x == 1) {
                return 10;
            } else {
                if (x == 2) {
                    return 20;
                } else {
                    return 30;
                }
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("30"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithComparisonOperators) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 10;
            var b: int64 = 20;
            if (a < b) {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithLogicalAnd) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: bool = true;
            var b: bool = true;
            if (a && b) {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithLogicalOr) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: bool = false;
            var b: bool = true;
            if (a || b) {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithNegatedCondition) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var flag: bool = false;
            if (!flag) {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, NestedIfStatements) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 5;
            var b: int64 = 10;
            if (a > 0) {
                if (b > 5) {
                    return 1;
                } else {
                    return 2;
                }
            } else {
                return 3;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithVariableAssignmentInThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            if (true) {
                result = 42;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseWithVariableAssignment) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            if (false) {
                result = 10;
            } else {
                result = 20;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithMultipleStatementsInThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 0;
            var b: int64 = 0;
            if (true) {
                a = 5;
                b = 10;
            }
            return a + b;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseWithMultipleStatementsInEachBranch) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 0;
            var b: int64 = 0;
            if (true) {
                a = 1;
                b = 2;
            } else {
                a = 3;
                b = 4;
            }
            return a + b;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithReturnInsideThen) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (true) {
                return 7;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("7"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseWithReturnInBothBranches) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 3;
            if (x == 1) {
                return 10;
            } else {
                return 20;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithFloatComparison) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var value: double = 3.5;
            if (value > 3.0) {
                return 1;
            } else {
                return 0;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfWithCharComparison) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var ch: char = 'b';
            if (ch == 'a') {
                return 1;
            } else {
                return 2;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("2"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfChainWithThreeBranches) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var score: int64 = 75;
            if (score >= 90) {
                return 4;
            } else {
                if (score >= 80) {
                    return 3;
                } else {
                    if (score >= 70) {
                        return 2;
                    } else {
                        return 1;
                    }
                }
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("2"), std::string::npos);
}

TEST_F(IfStatementCLIIntegrationTest, IfElseIfChainWithExpressionCondition) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            if (x + 1 == 6) {
                return 10;
            } else {
                if (x * 2 == 10) {
                    return 20;
                } else {
                    return 30;
                }
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}
