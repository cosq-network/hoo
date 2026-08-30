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

class VariableDeclarationCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_var_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_var_cli_"
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

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredInt64) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 42;
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitInt64) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 42;
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredDouble) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 3.14;
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitDouble) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = 2.718;
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredBool) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = true;
            if (x) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitBool) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bool = false;
            if (!x) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredChar) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var ch = 'A';
            if (ch.codepoint() == 65) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitChar) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var ch: char = 'A';
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredByte) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var b = 255;
            if (b == 255) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitByte) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var b: byte = 128;
            if (b == 128) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredBit) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var flag = true;
            if (flag) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitBit) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var flag: bit = 1;
            if (flag) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredInt8) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = -128;
            if (x == -128) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitInt8) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int8 = 127;
            if (x == 127) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredF8) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 1.5f8;
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredString) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s = "hello";
            if (s.equals("hello")) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitString) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "world";
            if (s.equals("world")) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredArray) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [1, 2, 3];
            return arr[0];
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithExplicitArrayType) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr: int64[] = [10, 20, 30];
            return arr[1];
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithInferredMap) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 100);
            return m.getInt64Int64(1);
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("100"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithoutInitializer) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64;
            x = 42;
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("42"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarReassignment) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            x = 20;
            x = 30;
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("30"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, MultipleVarDeclarations) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 1;
            var b: int64 = 2;
            var c: int64 = 3;
            return a + b + c;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("6"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithArithmeticExpression) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var sum: int64 = 10 + 20 + 30;
            return sum;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("60"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithBooleanExpression) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var result = true && false;
            if (!result) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithComparisonExpression) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var isGreater = 10 > 5;
            if (isGreater) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithNegativeNumber) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = -42;
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("-42"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarScopeInIfBlock) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            if (true) {
                var y: int64 = 20;
                x = x + y;
            }
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("30"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarScopeInWhileLoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < 5) {
                var inc: int64 = 1;
                total = total + inc;
                i = i + 1;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("5"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarScopeInForLoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var total: int64 = 0;
            for i in 1..4 {
                var val: int64 = i;
                total = total + val;
            }
            return total;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("6"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VariableShadowingInInnerScope) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            if (true) {
                var x: int64 = 20;
            }
            return x;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, ModuleLevelConstant) {
    const auto result = compileAndRun(R"(
        import hoo;
        const version = "1.0";
        func :int64 main() {
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, ModuleLevelConstantWithType) {
    const auto result = compileAndRun(R"(
        import hoo;
        const max: int64 = 100;
        func :int64 main() {
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithStringConcatenation) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var first = "hello";
            var second = "world";
            var combined = first + " " + second;
            if (combined.equals("hello world")) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithArrayLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [1, 2, 3, 4, 5];
            var len = arr.length();
            return len;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("5"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithMapLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var m = new Map(3, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);
            m.setInt64Int64(3, 30);
            var count = m.length();
            return count;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithIfElseExpression) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            var result: int64;
            if (x > 3) {
                result = 10;
            } else {
                result = 20;
            }
            return result;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("10"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithFunctionCallResult) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 helper(a: int64, b: int64) {
            return a + b;
        }
        func :int64 main() {
            var sum = helper(10, 20);
            return sum;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("30"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithMethodCallResult) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s = "hello";
            var len = s.length();
            return len;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("5"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarInNestedBlocks) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var outer: int64 = 1;
            if (true) {
                var inner: int64 = 2;
                outer = outer + inner;
            }
            return outer;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarDeclarationBeforeUse) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            var y: int64 = x + 5;
            return y;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("15"), std::string::npos);
}

TEST_F(VariableDeclarationCLIIntegrationTest, VarWithComplexTypeAnnotation) {
    const auto result = compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr: int64[] = [1, 2, 3];
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            return arr[0] + m.getInt64Int64(1);
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("11"), std::string::npos);
}
