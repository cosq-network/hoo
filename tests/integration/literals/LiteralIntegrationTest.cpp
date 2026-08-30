#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>

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

class LiteralIntegrationTest : public ::testing::Test {
protected:
    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        for (char& c : tempDir) {
            if (c == '\\') c = '/';
        }
        hooExe = HOO_EXECUTABLE;
    }

    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string createSourceFile(const std::string& content) {
        static int counter = 0;
        std::string path = tempDir + "/test_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream f(path);
        f << content;
        f.close();
        return path;
    }

    ExecResult runHoo(const std::string& args) {
        std::string cmd = "\"" + hooExe + "\" " + args + " 2>&1";
        FILE* pipe = popen(cmd.c_str(), "r");
        ExecResult result;
        if (!pipe) {
            result.exitCode = -1;
            result.output = "popen failed";
            return result;
        }
        std::ostringstream out;
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            out << buf;
        }
        int status = pclose(pipe);
#ifdef _WIN32
        // _pclose on Windows returns the child's exit code directly.
        result.exitCode = status;
#else
        result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
        result.output = out.str();
        return result;
    }

    void compileAndRun(const std::string& source, const std::string& expectedOutput) {
        std::string src = createSourceFile(source);
        std::string archive = tempDir + "/out_" + std::to_string(std::time(nullptr)) + ".ha";

        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_EQ(build.exitCode, 0) << "Compilation failed: " << build.output;
        EXPECT_NE(build.output.find("successfully built"), std::string::npos) << build.output;

        auto exec = runHoo(archive);
        ASSERT_EQ(exec.exitCode, 0) << "Execution failed: " << exec.output;
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos) << "Expected '" << expectedOutput << "' in output: " << exec.output;
    }
};

// ===== INTEGER LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, IntegerLiteral_Zero) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 0;
            return x;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, IntegerLiteral_Positive) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 42;
            return x;
        }
    )", "42");
}

TEST_F(LiteralIntegrationTest, IntegerLiteral_Large) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 9223372036854775807;
            return x;
        }
    )", "9223372036854775807");
}

TEST_F(LiteralIntegrationTest, IntegerLiteral_InExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            return 10 + 20 + 12;
        }
    )", "42");
}

TEST_F(LiteralIntegrationTest, IntegerLiteral_WithExplicitType) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 100;
            return x;
        }
    )", "100");
}

// ===== FLOATING LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, FloatingLiteral_Basic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = 3.14;
            return 314;
        }
    )", "314");
}

TEST_F(LiteralIntegrationTest, FloatingLiteral_Zero) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = 0.0;
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, FloatingLiteral_LeadingDot) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = 0.5;
            return 5;
        }
    )", "5");
}

TEST_F(LiteralIntegrationTest, FloatingLiteral_Arithmetic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = 1.5 + 2.5;
            return 4;
        }
    )", "4");
}

// ===== DECIMAL LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, DecimalLiteral_LowercaseM) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: Decimal<10, 2> = 10.5m;
            return 105;
        }
    )", "105");
}

TEST_F(LiteralIntegrationTest, DecimalLiteral_UppercaseM) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: Decimal<10, 2> = 20.25M;
            return 2025;
        }
    )", "2025");
}

TEST_F(LiteralIntegrationTest, DecimalLiteral_LeadingDot) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: Decimal<10, 2> = .5m;
            return 50;
        }
    )", "50");
}

TEST_F(LiteralIntegrationTest, DecimalLiteral_IntegerOnly) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: Decimal<10, 0> = 100m;
            return 100;
        }
    )", "100");
}

// ===== F8 LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, F8Literal_Basic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: f8 = 1.5f8;
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, F8Literal_Zero) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: f8 = 0.0f8;
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, F8Literal_SmallValue) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: f8 = 0.25f8;
            return 0;
        }
    )", "0");
}

// ===== BIT LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, BitLiteral_Zero) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bit = 0b;
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, BitLiteral_One) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bit = 1b;
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, BitLiteral_InExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: bit = 1b;
            var b: bit = 0b;
            return 1;
        }
    )", "1");
}

// ===== STRING LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, StringLiteral_Empty) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "";
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, StringLiteral_Simple) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "hello";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, StringLiteral_WithEscape) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "hello\nworld";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, StringLiteral_WithQuotes) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "say \"hi\"";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, StringLiteral_WithBackslash) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = "path\\to\\file";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, MultilineString_Basic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string = """line1
line2""";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, StringLiteral_Concatenation) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: string = "hello";
            var b: string = " world";
            return 1;
        }
    )", "1");
}

// ===== INTERPOLATED STRING TESTS =====

TEST_F(LiteralIntegrationTest, InterpolatedString_Simple) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var name: string = "world";
            var s: string = "hello ${name}";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, InterpolatedString_MultipleVars) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: string = "foo";
            var b: string = "bar";
            var s: string = "${a}${b}";
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, InterpolatedString_WithExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 42;
            var s: string = "val=${x}";
            return 1;
        }
    )", "1");
}

// ===== CHARACTER LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, CharacterLiteral_Simple) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var c: char = 'a';
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, CharacterLiteral_Digit) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var c: char = '0';
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, CharacterLiteral_Escape) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var c: char = '\n';
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, CharacterLiteral_SingleQuote) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var c: char = '\'';
            return 1;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, CharacterLiteral_Space) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var c: char = ' ';
            return 1;
        }
    )", "1");
}

// ===== BOOLEAN LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, BooleanLiteral_True) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bool = true;
            if (x) {
                return 1;
            }
            return 0;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, BooleanLiteral_False) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: bool = false;
            if (x) {
                return 1;
            }
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, BooleanLiteral_InCondition) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (true && true) {
                return 1;
            }
            return 0;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, BooleanLiteral_Or) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (false || true) {
                return 1;
            }
            return 0;
        }
    )", "1");
}

TEST_F(LiteralIntegrationTest, BooleanLiteral_Not) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            if (!false) {
                return 1;
            }
            return 0;
        }
    )", "1");
}

// ===== NULL LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, NullLiteral_Assignment) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var s: string? = null;
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, NullLiteral_InAnyArray) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(42, 100);
            return 1;
        }
    )", "1");
}

// ===== THIS LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, ThisLiteral_MethodAccess) {
    compileAndRun(R"(
        import hoo;
        class Counter {
            var count: int64 = 0;

            constructor() {}

            func :int64 increment() {
                this.count = this.count + 1;
                return this.count;
            }
        }

        func :int64 main() {
            var c = new Counter();
            c.increment();
            c.increment();
            c.increment();
            return c.increment();
        }
    )", "4");
}

TEST_F(LiteralIntegrationTest, ThisLiteral_ReturnThis) {
    compileAndRun(R"(
        import hoo;
        class Builder {
            var value: int64 = 0;

            constructor() {}

            func :Builder withValue(v: int64) {
                this.value = v;
                return this;
            }
        }

        func :int64 main() {
            var b = new Builder();
            b.withValue(42);
            return 42;
        }
    )", "42");
}

// ===== ARRAY LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, ArrayLiteral_Integers) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [1, 2, 3];
            return arr.length();
        }
    )", "3");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_EmptyWithExplicitType) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr: int64[] = [];
            return 0;
        }
    )", "0");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_Floats) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [1.0, 2.0, 3.0];
            return arr.length();
        }
    )", "3");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_Booleans) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [true, false, true];
            return arr.length();
        }
    )", "3");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_Strings) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = ["a", "bb", "ccc"];
            return arr.length();
        }
    )", "3");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_Nested) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [[1, 2], [3, 4]];
            return arr.length();
        }
    )", "2");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_AccessElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [10, 20, 30];
            return arr[1];
        }
    )", "20");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_WithExpressions) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x = 10;
            var arr = [x, x + 10, x + 20];
            return arr[2];
        }
    )", "30");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_AnyType) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);
            return m.length();
        }
    )", "2");
}

TEST_F(LiteralIntegrationTest, ArrayLiteral_SingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var arr = [42];
            return arr[0];
        }
    )", "42");
}

// ===== MIXED LITERAL TESTS =====

TEST_F(LiteralIntegrationTest, MixedLiterals_InVariables) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 42;
            var b: double = 3.14;
            var c: bool = true;
            var d: string = "test";
            return a;
        }
    )", "42");
}

TEST_F(LiteralIntegrationTest, MixedLiterals_ArrayOfMixed) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 1;
            var b: double = 2.5;
            var c: bool = true;
            var d: string = "hello";
            return 5;
        }
    )", "5");
}

TEST_F(LiteralIntegrationTest, MixedLiterals_FunctionArguments) {
    compileAndRun(R"(
        import hoo;
        func :int64 add(a: int64, b: int64) {
            return a + b;
        }

        func :int64 main() {
            return add(10, 32);
        }
    )", "42");
}
