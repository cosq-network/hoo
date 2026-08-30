#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#define popen _popen
#define pclose _pclose
#define unlink _unlink
#define stat _stat
#else
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef HOO_EXECUTABLE
#error "HOO_EXECUTABLE must be defined via CMake -D"
#endif

// End-to-end integration tests for every expression operator in the Hoo
// language. Each test is a complete, self-contained Hoo program compiled to a
// .ha archive and executed via the hoo CLI. The CLI prints the int64 result of
// the entry point, so each program's main returns :int64 and the test asserts
// on the printed value.
//
// Tests are grouped by operator category:
//   - Arithmetic (binary): +, -, *, /, %
//   - Element-wise (binary): *., /.
//   - Bitwise shifts: <<, >>
//   - Comparison: ==, !=, <, <=, >, >=
//   - Logical: &&, ||, !
//   - Unary: -, !
//   - Assignment: =, +=, -=, *=, /=, %=, <<=, >>=
//   - Increment/Decrement: ++, --
class ExpressionOperatorsIntegrationTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_expr_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".hoo";
        std::ofstream f(path);
        f << content;
        f.close();
        return path;
    }

    std::string createArchivePath() {
        static int counter = 0;
        return tempDir + "/hoo_expr_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
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

    void compileAndRun(const std::string& source, const std::string& expectedOutput) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();

        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_EQ(build.exitCode, 0) << build.output;
        EXPECT_NE(build.output.find("successfully built"), std::string::npos) << build.output;

        auto exec = runHoo(archive);
        ASSERT_EQ(exec.exitCode, 0) << exec.output;
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos) << exec.output;
    }

    void compileAndRunExpectError(const std::string& source, const std::string& errorSubstring) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();

        auto build = runHoo("-o " + archive + " " + src);
        EXPECT_NE(build.exitCode, 0) << "Expected compilation to fail but it succeeded";
        EXPECT_NE(build.output.find(errorSubstring), std::string::npos)
            << "Expected error containing '" << errorSubstring << "' but got: " << build.output;
    }
};

// ============================================================================
// Arithmetic Binary Operators: +, -, *, /, %
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, AdditionInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 10 + 20;
            var b: int64 = -5 + 3;
            if (a == 30 && b == -2) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, SubtractionInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 50 - 20;
            var b: int64 = 3 - 10;
            if (a == 30 && b == -7) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, MultiplicationInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 6 * 7;
            var b: int64 = -3 * 4;
            if (a == 42 && b == -12) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, DivisionInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 100 / 10;
            var b: int64 = 7 / 2;
            var c: int64 = -15 / 3;
            if (a == 10 && b == 3 && c == -5) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, ModuloInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 17 % 5;
            var b: int64 = 20 % 3;
            if (a == 2 && b == 2) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, ArithmeticInt8) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int8 = 10 + 20;
            var b: int8 = 50 - 30;
            var c: int8 = 6 * 7;
            var d: int8 = 21 / 3;
            var e: int8 = 17 % 5;
            var r: int64 = 0;
            if (a == 30) { r = r + 1; }
            if (b == 20) { r = r + 10; }
            if (c == 42) { r = r + 100; }
            if (d == 7) { r = r + 1000; }
            if (e == 2) { r = r + 10000; }
            return r;
        }
    )", "11111");
}

TEST_F(ExpressionOperatorsIntegrationTest, ArithmeticByte) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: byte = 100 + 50;
            var b: byte = 200 - 100;
            var c: byte = 12 * 5;
            if (a == 150 && b == 100 && c == 60) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, ArithmeticF64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: f64 = 10.5 + 2.5;
            var b: f64 = 20.0 - 5.5;
            var c: f64 = 3.0 * 4.0;
            var d: f64 = 15.0 / 3.0;
            var r: int64 = 0;
            if (a == 13.0) { r = r + 1; }
            if (b == 14.5) { r = r + 10; }
            if (c == 12.0) { r = r + 100; }
            if (d == 5.0) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, ArithmeticFloat) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: float = 10.0 + 2.0;
            var b: float = 20.0 - 5.0;
            var c: float = 3.0 * 4.0;
            var r: int64 = 0;
            if (a == 12.0) { r = r + 1; }
            if (b == 15.0) { r = r + 10; }
            if (c == 12.0) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, ArithmeticDouble) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: double = 10.5 + 2.5;
            var b: double = 20.0 - 5.5;
            var c: double = 3.0 * 4.0;
            var d: double = 15.0 / 3.0;
            var r: int64 = 0;
            if (a == 13.0) { r = r + 1; }
            if (b == 14.5) { r = r + 10; }
            if (c == 12.0) { r = r + 100; }
            if (d == 5.0) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

// ============================================================================
// Bit Shift Operators: <<, >>
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, LeftShiftInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 1 << 0;
            var b: int64 = 1 << 1;
            var c: int64 = 1 << 5;
            var d: int64 = 3 << 2;
            var r: int64 = 0;
            if (a == 1) { r = r + 1; }
            if (b == 2) { r = r + 10; }
            if (c == 32) { r = r + 100; }
            if (d == 12) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, RightShiftInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 32 >> 5;
            var b: int64 = 100 >> 2;
            var c: int64 = 1 >> 1;
            var r: int64 = 0;
            if (a == 1) { r = r + 1; }
            if (b == 25) { r = r + 10; }
            if (c == 0) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LeftShiftInt8) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int8 = 1 << 3;
            var b: int8 = 16 >> 2;
            var r: int64 = 0;
            if (a == 8) { r = r + 1; }
            if (b == 4) { r = r + 10; }
            return r;
        }
    )", "11");
}

// ============================================================================
// Comparison Operators: ==, !=, <, <=, >, >=
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, EqualityInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (5 == 5) { r = r + 1; }
            if (!(5 == 3)) { r = r + 10; }
            if (5 != 3) { r = r + 100; }
            if (!(5 != 5)) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LessThanInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (3 < 5) { r = r + 1; }
            if (!(5 < 3)) { r = r + 10; }
            if (!(5 < 5)) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LessEqualInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (3 <= 5) { r = r + 1; }
            if (5 <= 5) { r = r + 10; }
            if (!(5 <= 3)) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, GreaterThanInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (5 > 3) { r = r + 1; }
            if (!(3 > 5)) { r = r + 10; }
            if (!(5 > 5)) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, GreaterEqualInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (5 >= 3) { r = r + 1; }
            if (5 >= 5) { r = r + 10; }
            if (!(3 >= 5)) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, ComparisonF64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (3.5 < 5.0) { r = r + 1; }
            if (5.0 > 3.5) { r = r + 10; }
            if (5.0 == 5.0) { r = r + 100; }
            if (3.5 != 5.0) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, ComparisonBool) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (true == true) { r = r + 1; }
            if (true != false) { r = r + 10; }
            if (!(true == false)) { r = r + 100; }
            return r;
        }
    )", "111");
}

// ============================================================================
// Logical Operators: &&, ||, !
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, LogicalAnd) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (true && true) { r = r + 1; }
            if (!(true && false)) { r = r + 10; }
            if (!(false && true)) { r = r + 100; }
            if (!(false && false)) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LogicalOr) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (true || true) { r = r + 1; }
            if (true || false) { r = r + 10; }
            if (false || true) { r = r + 100; }
            if (!(false || false)) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LogicalNot) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            var a: bool = false;
            var b: bool = true;
            var c: bool = true;
            if (a == false) { r = r + 1; }
            var notA: bool = !a;
            if (notA == true) { r = r + 10; }
            var notB: bool = !b;
            if (notB == false) { r = r + 100; }
            var notC: bool = !c;
            var notNotC: bool = !notC;
            if (notNotC == true) { r = r + 1000; }
            return r;
        }
    )", "1111");
}

TEST_F(ExpressionOperatorsIntegrationTest, LogicalCombined) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if ((true && false) || true) { r = r + 1; }
            if (!(true && (false || false))) { r = r + 10; }
            if ((1 < 2) && (3 > 2)) { r = r + 100; }
            return r;
        }
    )", "111");
}

// ============================================================================
// Unary Operators: -, !
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = -10;
            var b: int64 = -(-5);
            var c: int64 = -(3 + 2);
            var r: int64 = 0;
            if (a == -10) { r = r + 1; }
            if (b == 5) { r = r + 10; }
            if (c == -5) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusF64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: f64 = -3.14;
            var b: f64 = -(-2.5);
            var r: int64 = 0;
            if (a == -3.14) { r = r + 1; }
            if (b == 2.5) { r = r + 10; }
            return r;
        }
    )", "11");
}

TEST_F(ExpressionOperatorsIntegrationTest, UnaryNotBool) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: bool = !true;
            var b: bool = !false;
            var r: int64 = 0;
            if (a == false) { r = r + 1; }
            if (b == true) { r = r + 10; }
            return r;
        }
    )", "11");
}

// ============================================================================
// Simple Assignment: =
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, SimpleAssignmentInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 0;
            x = 42;
            var y: int64 = x;
            if (x == 42 && y == 42) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, SimpleAssignmentChained) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 0;
            var b: int64 = 0;
            var c: int64 = 99;
            b = c;
            a = b;
            if (a == 99 && b == 99 && c == 99) { return 1; }
            return 0;
        }
    )", "1");
}

// ============================================================================
// Compound Assignment: +=, -=, *=, /=, %=
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, CompoundAddAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            x += 5;
            if (x == 15) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundSubtractAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 20;
            x -= 7;
            if (x == 13) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundMultiplyAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 6;
            x *= 7;
            if (x == 42) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundDivideAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 100;
            x /= 4;
            if (x == 25) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundModuloAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 17;
            x %= 5;
            if (x == 2) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundAssignInt8) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int8 = 10;
            a += 5;
            var b: int8 = 20;
            b -= 3;
            var c: int8 = 6;
            c *= 7;
            var r: int64 = 0;
            if (a == 15) { r = r + 1; }
            if (b == 17) { r = r + 10; }
            if (c == 42) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundAssignF64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: f64 = 10.0;
            var b: f64 = 20.0;
            var c: f64 = 3.0;
            var d: f64 = 15.0;
            a = a + 2.5;
            b = b - 5.5;
            c = c * 4.0;
            d = d / 3.0;
            var r: int64 = 0;
            if (a == 12.5) { r = r + 1; }
            if (b == 14.5) { r = r + 2; }
            if (c == 12.0) { r = r + 4; }
            if (d == 5.0) { r = r + 8; }
            return r;
        }
    )", "15");
}

// ============================================================================
// Bit Shift Compound Assignment: <<=, >>=
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, CompoundLeftShiftAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 1;
            x <<= 5;
            if (x == 32) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundRightShiftAssign) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 64;
            x >>= 3;
            if (x == 8) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, CompoundShiftAssignInt8) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int8 = 1;
            a <<= 4;
            var b: int8 = 32;
            b >>= 2;
            var r: int64 = 0;
            if (a == 16) { r = r + 1; }
            if (b == 8) { r = r + 10; }
            return r;
        }
    )", "11");
}

// ============================================================================
// Increment/Decrement: ++, --
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, PostIncrement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            var y: int64 = x++;
            if (x == 6 && y == 5) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, PostDecrement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 5;
            var y: int64 = x--;
            if (x == 4 && y == 5) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, IncrementInExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            var sum: int64 = x++ + 5;
            if (sum == 15 && x == 11) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, DecrementInExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            var diff: int64 = x-- - 3;
            if (diff == 7 && x == 9) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, MultipleIncrements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 0;
            x++;
            x++;
            x++;
            if (x == 3) { return 1; }
            return 0;
        }
    )", "1");
}

// ============================================================================
// Mixed Expression Precedence
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, PrecedenceMultiplicationBeforeAddition) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 2 + 3 * 4;
            if (x == 14) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, PrecedenceWithParentheses) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = (2 + 3) * 4;
            if (x == 20) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, PrecedenceComparisonAndLogical) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var r: int64 = 0;
            if (1 + 2 == 3 && 4 * 5 > 10) { r = r + 1; }
            if (3 < 5 || 10 > 20) { r = r + 10; }
            return r;
        }
    )", "11");
}

TEST_F(ExpressionOperatorsIntegrationTest, PrecedenceShiftVsArithmetic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 1 + 1 << 2;
            var b: int64 = 1 << 2 + 1;
            var r: int64 = 0;
            if (a == 8) { r = r + 1; }
            if (b == 8) { r = r + 10; }
            return r;
        }
    )", "11");
}

TEST_F(ExpressionOperatorsIntegrationTest, ComplexMixedExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = ((10 + 5) * 2 - 4) / 2;
            var y: int64 = 1 << 3;
            var z: int64 = 100 % 7;
            var r: int64 = 0;
            if (x == 13) { r = r + 1; }
            if (y == 8) { r = r + 10; }
            if (z == 2) { r = r + 100; }
            return r;
        }
    )", "111");
}

// ============================================================================
// Unary Minus on Various Types
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusInt8) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int8 = -42;
            if (x == -42) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusByte) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: byte = 10;
            var negX: byte = 0;
            negX = negX - x;
            var r: int64 = 0;
            if (negX == 246) { r = r + 1; }
            return r;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusFloat) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: float = -3.5;
            if (x == -3.5) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, UnaryMinusDouble) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: double = -7.25;
            if (x == -7.25) { return 1; }
            return 0;
        }
    )", "1");
}

// ============================================================================
// Assignment with Expressions on Right-Hand Side
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, AssignmentWithArithmeticExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 0;
            x = 10 + 20 * 3;
            if (x == 70) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, AssignmentWithComparisonExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 0;
            if (5 > 3) { x = 1; }
            if (x == 1) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, AssignmentWithLogicalExpression) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: bool = true && false;
            var b: bool = true || false;
            var r: int64 = 0;
            if (a == false) { r = r + 1; }
            if (b == true) { r = r + 10; }
            return r;
        }
    )", "11");
}

// ============================================================================
// Parenthesized Expressions
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, ParenthesizedSimple) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = (42);
            if (x == 42) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, ParenthesizedNested) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = (((10)));
            if (x == 10) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(ExpressionOperatorsIntegrationTest, ParenthesizedOverridesPrecedence) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a: int64 = 2 + 3 * 4;
            var b: int64 = (2 + 3) * 4;
            var r: int64 = 0;
            if (a == 14) { r = r + 1; }
            if (b == 20) { r = r + 10; }
            return r;
        }
    )", "11");
}

// ============================================================================
// All Operators Combined
// ============================================================================

TEST_F(ExpressionOperatorsIntegrationTest, AllOperatorsCombined) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var x: int64 = 10;
            x += 5;
            x *= 2;
            x -= 10;
            x /= 3;
            x %= 4;
            x <<= 2;
            x >>= 1;

            var y: int64 = 0;
            if (x == 6) { y = y + 1; }

            var a: bool = (10 > 5) && (3 < 7);
            var b: bool = (1 == 2) || (4 >= 4);
            if (a == true) { y = y + 2; }
            if (b == true) { y = y + 4; }

            var c: int64 = -5;
            var notFalse: bool = false;
            notFalse = !notFalse;
            if (notFalse == true) { y = y + 8; }
            if (c == -5) { y = y + 16; }

            var d: int64 = 1;
            d++;
            if (d == 2) { y = y + 32; }

            return y;
        }
    )", "62");
}
