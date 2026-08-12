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

class ExceptionHandlingCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_exception_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_exception_cli_"
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

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchCatchesThrownException) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            try {
                throw new Exception("boom");
            } catch (e: Exception) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryFinallyRunsFinallyWithoutException) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                result = 10;
            } finally {
                result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("11"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchFinallyWithThrownException) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                result = 10;
                throw new Exception("boom");
            } catch (e: Exception) {
                result = 100;
            } finally {
                result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("101"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, UncaughtThrowReturnsNonZeroExit) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            throw new Exception("uncaught");
            return 0;
        }
    )");
    EXPECT_NE(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("Unhandled exception trap"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, MultipleCatchClausesSecondMatches) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            try {
                throw new Exception("boom");
            } catch (e: int64) {
                return 1;
            } catch (e: Exception) {
                return 2;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("2"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, NestedTryCatchInnerCatches) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                try {
                    throw new Exception("inner");
                } catch (e: Exception) {
                    result = 50;
                }
                result = result + 1;
            } catch (e: Exception) {
                result = result + 100;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("51"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryFinallyRunsAfterReturnInTry) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                result = 10;
                return result;
            } finally {
                result = 999;
            }
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchWithVariableAssignmentInCatch) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var caught: int64 = 0;
            try {
                throw new Exception("boom");
            } catch (e: Exception) {
                caught = 42;
            }
            return caught;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchFinallyWithVariableState) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 0;
            var y: int64 = 0;
            try {
                x = 5;
                throw new Exception("boom");
            } catch (e: Exception) {
                y = 10;
            } finally {
                x = x + 1;
            }
            return x + y;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("16"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryWithNoCatchAndNoFinallyIsUnhandled) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            try {
                throw new Exception("boom");
            }
            return 0;
        }
    )");
    EXPECT_NE(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("Unhandled rethrow trap"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, CatchBlockWithMultipleStatements) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 0;
            var b: int64 = 0;
            try {
                throw new Exception("boom");
            } catch (e: Exception) {
                a = 1;
                b = 2;
            }
            return a + b;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryFinallyWithMultipleStatementsInTry) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 0;
            var b: int64 = 0;
            try {
                a = 1;
                b = 2;
            } finally {
                a = a + 10;
                b = b + 20;
            }
            return a + b;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("33"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, ThrowInsideNestedTryCaughtByOuter) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                try {
                    throw new Exception("inner");
                } catch (e: Exception) {
                    result = 50;
                }
            } catch (e: Exception) {
                result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("50"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchWithIfElseInside) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            try {
                if (x > 3) {
                    throw new Exception("big");
                } else {
                    throw new Exception("small");
                }
            } catch (e: Exception) {
                if (x > 3) {
                    return 1;
                } else {
                    return 2;
                }
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ExceptionHandlingCLIIntegrationTest, TryCatchFinallyWithIfElseInCatch) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result: int64 = 0;
            try {
                throw new Exception("boom");
            } catch (e: Exception) {
                if (true) {
                    result = 10;
                } else {
                    result = 20;
                }
            } finally {
                result = result + 1;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("11"), std::string::npos);
}
