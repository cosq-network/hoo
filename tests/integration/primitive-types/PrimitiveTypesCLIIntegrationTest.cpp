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

// End-to-end tests that exercise every built-in primitive type through the hoo
// CLI. Each test is a complete, self-contained Hoo program with an entry point.
// The program is compiled to a .ha archive with `hoo -o out.ha src.hoo` and the
// archive is then executed with `hoo out.ha`. The CLI prints the int64 result of
// the entry point, so each program's main returns :int64 and the test asserts
// on the printed value.
class PrimitiveTypesCLIIntegrationTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_prim_"
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
        return tempDir + "/hoo_prim_out_"
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

    // Compile the given program to a .ha archive and execute the archive.
    // Asserts the build succeeds, the archive runs successfully, and the
    // printed entry point result contains expectedOutput.
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
};

TEST_F(PrimitiveTypesCLIIntegrationTest, Int64Arithmetic) {
    compileAndRun(R"(
        import hoo;

        func :int64 add(x: int64, y: int64) { return x + y; }
        func :int64 mul(x: int64, y: int64) { return x * y; }

        func :int64 main() {
            var a: int64 = add(20, 22);
            var b: int64 = mul(6, 7);
            var c: int64 = 42;
            c = c - 2;
            var d: int64 = c % 6;
            var e: int64 = 21 / 2;
            if (a == 42 && b == 42 && c == 40 && d == 4 && e == 10) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, Int8ArithmeticWrapping) {
    compileAndRun(R"(
        import hoo;

        func :int8 add(x: int8, y: int8) { return x + y; }

        func :int64 main() {
            var a: int8 = add(100, 30);
            var b: int8 = add(60, 60);
            var c: int64 = a + b;
            if (a == -126 && b == 120 && c == -6) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, ByteArithmeticWrapping) {
    compileAndRun(R"(
        import hoo;

        func :byte add(x: byte, y: byte) { return x + y; }

        func :int64 main() {
            var a: byte = add(250, 10);
            var b: byte = add(200, 100);
            var d: byte = 12 * 5;
            if (a == 4 && b == 44 && d == 60) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, BoolLogicOperators) {
    compileAndRun(R"(
        import hoo;

        func :bool andOp(x: bool, y: bool) { return x && y; }
        func :bool orOp(x: bool, y: bool) { return x || y; }
        func :bool notOp(x: bool) { return !x; }

        func :int64 main() {
            var a: bool = andOp(true, false);
            var b: bool = orOp(false, true);
            var c: bool = notOp(false);
            var r: int64 = 0;
            if (a == false) { r = r + 1; }
            if (b == true) { r = r + 10; }
            if (c == true) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, BitTypeLogicOperators) {
    compileAndRun(R"(
        import hoo;

        func :bit bitAnd(x: bit, y: bit) { return x && y; }
        func :bit bitOr(x: bit, y: bit) { return x || y; }
        func :bit bitNot(x: bit) { return !x; }

        func :int64 main() {
            var a: bit = bitAnd(1b, 1b);
            var b: bit = bitOr(0b, 1b);
            var c: bit = bitNot(0b);
            var r: int64 = 0;
            if (a == 1b) { r = r + 1; }
            if (b == 1b) { r = r + 10; }
            if (c == 1b) { r = r + 100; }
            return r;
        }
    )", "111");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, ShiftBitwiseOperations) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a: int64 = 20;
            var b: int64 = a % 6;
            var c: int64 = 21 / 2;
            var d: int64 = 1;
            d <<= 3;
            var e: int64 = 64;
            e >>= 2;
            var f: int64 = 1 << 2;
            var g: int64 = 16 >> 2;
            return b + c * 10 + d * 100 + e * 1000 + f + g * 10000;
        }
    )", "56906");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, FloatArithmetic) {
    compileAndRun(R"(
        import hoo;

        func :float add(x: float, y: float) { return x + y; }
        func :float mul(x: float, y: float) { return x * y; }

        func :int64 main() {
            var a: float = add(1.5, 2.5);
            var b: float = mul(3.0, 4.0);
            var r: int64 = 0;
            if (a == 4.0) { r = r + 1; }
            if (b > 11.9 && b < 12.1) { r = r + 10; }
            return r;
        }
    )", "11");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, DoubleArithmetic) {
    compileAndRun(R"(
        import hoo;

        func :double add(x: double, y: double) { return x + y; }
        func :double div(x: double, y: double) { return x / y; }

        func :int64 main() {
            var a: double = add(1.5, 2.5);
            var b: double = div(21.0, 3.0);
            var r: int64 = 0;
            if (a == 4.0) { r = r + 1; }
            if (b == 7.0) { r = r + 10; }
            return r;
        }
    )", "11");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, F64Arithmetic) {
    compileAndRun(R"(
        import hoo;

        func :f64 add(x: f64, y: f64) { return x + y; }

        func :int64 main() {
            var a: f64 = add(2.25, 3.75);
            if (a == 6.0) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, F8Arithmetic) {
    compileAndRun(R"(
        import hoo;

        func :f8 add(x: f8, y: f8) { return x + y; }

        func :int64 main() {
            var a: f8 = add(1.5f8, 2.5f8);
            if (a > 3.9f8) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, CharacterType) {
    compileAndRun(R"(
        import hoo.character;

        func :Character getChar() { return 'A'; }

        func :int64 main() {
            var ch: Character = getChar();
            return ch.codepoint() + ch.length();
        }
    )", "66");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, StringOperations) {
    compileAndRun(R"(
        import hoo;

        func :string join(a: string, b: string) { return a.concat(b); }

        func :int64 main() {
            var s: string = join("Hello", "!");
            var t: string = s.toLower();
            var len: int64 = t.length();
            var empty: int64 = "".isEmpty();
            if (t.equals("hello!")) { return len + empty + 10; }
            return 0;
        }
    )", "17");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, BufferOperations) {
    compileAndRun(R"(
        import hoo.buffer;

        func :Buffer makeBuffer() { return Buffer.fromBytes("abc", 3); }

        func :int64 main() {
            var b: Buffer = makeBuffer();
            var cp: Buffer = b.copy();
            var len: int64 = cp.length();
            var last: int64 = cp.byteAt(2);
            cp.setByte(0, 90);
            var first: int64 = cp.byteAt(0);
            return len + last + first;
        }
    )", "192");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, VoidFunctionCall) {
    compileAndRun(R"(
        import hoo;

        func :void doWork(x: int64) { var y: int64 = x; y = y + 1; }

        func :int64 main() {
            doWork(5);
            return 1;
        }
    )", "1");
}

TEST_F(PrimitiveTypesCLIIntegrationTest, DecimalArithmetic) {
    compileAndRun(R"(
        import hoo.decimal;

        func :Decimal<38,2> add(a: Decimal<38,2>, b: Decimal<38,2>) { return a + b; }
        func :Decimal<38,2> mul(a: Decimal<38,2>, b: Decimal<38,2>) { return a * b; }

        func :int64 main() {
            var a: Decimal<38,2> = add(19.99m, 8.01m);
            var b: Decimal<38,2> = mul(3.00m, 4.00m);
            var r: int64 = 0;
            if (a == 28.00m) { r = r + 1; }
            if (b == 12.00m) { r = r + 10; }
            return r;
        }
    )", "11");
}
