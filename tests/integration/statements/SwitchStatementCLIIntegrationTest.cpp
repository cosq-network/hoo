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

class SwitchStatementCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_switch_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_switch_cli_"
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

TEST_F(SwitchStatementCLIIntegrationTest, SwitchBasicMatchingCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; break;
                case 3: result = 30; break;
                default: result = 0;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchDefaultExecutesWhenNoMatch) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 99;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; break;
                default: result = 0;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchFallThroughWithoutBreak) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 100;
                case 2: result = 200;
                default: result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("201"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithoutDefaultNoMatchFallsThrough) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 99;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; break;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchInLoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 1..6 {
                switch (i) {
                    case 1: total = total + 1; break;
                    case 2: total = total + 2; break;
                    default: total = total + i;
                }
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchContinueTargetsEnclosingLoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 1..6 {
                switch (i) {
                    case 2: continue;
                    case 4: continue;
                    default: total = total + i;
                }
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("9"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchBreakExitsSwitchButContinuesLoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 1..6 {
                switch (i) {
                    case 3: break;
                    default: total = total + i;
                }
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("12"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, NestedSwitch) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var y: int64 = 2;
            var result: int64 = 0;
            switch (x) {
                case 1:
                    switch (y) {
                        case 1: result = 10; break;
                        case 2: result = 20; break;
                        default: result = 30;
                    }
                    break;
                case 2: result = 40; break;
                default: result = 50;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithIfElseInsideCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            var result: int64 = 0;
            switch (x) {
                case 1:
                    if (true) {
                        result = 10;
                    } else {
                        result = 11;
                    }
                    break;
                case 2:
                    result = 20;
                    break;
                default:
                    result = 30;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithMultipleStatementsPerCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var a: int64 = 0;
            var b: int64 = 0;
            switch (x) {
                case 1:
                    a = 5;
                    b = 10;
                    break;
                case 2:
                    a = 20;
                    b = 30;
                    break;
                default:
                    a = 40;
                    b = 50;
            }
            return a + b;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithReturnInCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            switch (x) {
                case 1: return 10;
                case 2: return 20;
                case 3: return 30;
                default: return 0;
            }
            return 999;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithExpressionDiscriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            var result: int64 = 0;
            switch (x + 1) {
                case 5: result = 10; break;
                case 6: result = 20; break;
                default: result = 30;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchInt8Discriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int8 = 2;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; break;
                default: result = 0;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchByteDiscriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: byte = 2;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; break;
                default: result = 0;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchBoolDiscriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bool = true;
            var result: int64 = 0;
            switch (x) {
                case false: result = 10; break;
                case true: result = 20; break;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchBitDiscriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bit = 1;
            var result: int64 = 0;
            switch (x) {
                case 0: result = 10; break;
                case 1: result = 20; break;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithVariableAssignmentInCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2;
            var result: int64 = 0;
            switch (x) {
                case 1: result = 10; break;
                case 2: result = 20; result = result + 5; break;
                default: result = 30;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("25"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchEmptyCaseFallsThrough) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var result: int64 = 0;
            switch (x) {
                case 1:
                case 2: result = 20; break;
                default: result = 0;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchMultipleCasesFallToDefault) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var result: int64 = 0;
            switch (x) {
                case 1:
                case 2:
                case 3: result = 100;
                default: result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("101"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithVariableDeclarationInCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var result: int64 = 0;
            switch (x) {
                case 1:
                    var temp: int64 = 42;
                    result = temp;
                    break;
                case 2:
                    result = 20;
                    break;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchWithNestedLoopInsideCase) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            var total: int64 = 0;
            switch (x) {
                case 1:
                    for i in 1..4 {
                        total = total + i;
                    }
                    break;
                case 2:
                    total = 100;
                    break;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("6"), std::string::npos);
}

TEST_F(SwitchStatementCLIIntegrationTest, SwitchRejectsStringDiscriminant) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = "a";
            switch (x) {
                case "a": return 1;
                default: return 0;
            }
        }
    )");
    EXPECT_NE(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("switch only supports integer-like discriminants"), std::string::npos);
}
