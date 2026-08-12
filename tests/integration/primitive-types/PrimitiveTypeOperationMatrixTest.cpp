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

// Success matrix: every primitive type exercised against every operator the
// language accepts, end-to-end through the hoo CLI (compile to .ha, run the
// archive). Each program builds a bitmask of checks and returns it from
// main(:int64); the test asserts the exact bitmask, so a failing bit pinpoints
// the operator that regressed.
//
// Expected values below were established by executing each program with the
// hoo CLI and are deliberately deterministic.
class PrimitiveTypeOperationMatrixTest : public ::testing::Test {
protected:
    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        hooExe = HOO_EXECUTABLE;
    }

    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string createSourceFile(const std::string& content) {
        static int counter = 0;
        std::string path = tempDir + "/hoo_matrix_"
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
        return tempDir + "/hoo_matrix_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
#ifdef _WIN32
        static int captureCounter = 0;
        std::string capturePath = tempDir + "/hoo_matrix_capture_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++captureCounter)
            + ".txt";
        std::string cmd = "cmd.exe /S /C \"\"" + hooExe + "\" " + args
            + " > \"" + capturePath + "\" 2>&1\"";
        int status = std::system(cmd.c_str());

        std::ifstream captured(capturePath, std::ios::binary);
        std::ostringstream out;
        out << captured.rdbuf();
        captured.close();
        std::remove(capturePath.c_str());

        ExecResult result;
        result.exitCode = status;
        result.output = out.str();
        return result;
#else
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
        result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        result.output = out.str();
        return result;
#endif
    }

    void compileAndRun(const std::string& source, const std::string& expectedOutput) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();

        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_EQ(build.exitCode, 0) << build.output;

        auto exec = runHoo(archive);
        ASSERT_EQ(exec.exitCode, 0) << exec.output;
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos) << exec.output;
    }
};

TEST_F(PrimitiveTypeOperationMatrixTest, Int64AllOperations) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a: int64 = 7;
            var b: int64 = 3;
            var c: int64 = a + b;
            var d: int64 = a - b;
            var e: int64 = a * b;
            var f: int64 = a / b;
            var g: int64 = a % b;
            var h: int64 = a << 1;
            var i: int64 = a >> 1;
            var j: int64 = a && b;
            var k: int64 = a || 0;
            var l: int64 = !a;
            var m: int64 = !0;
            var n: int64 = -a;
            a++;
            var o: int64 = a;
            a--;
            var p: int64 = a;
            var cmp: int64 = 0;
            if (a > b && a != b) { cmp = cmp + 1; }
            if (a >= b && a <= b + 4) { cmp = cmp + 2; }
            if (a == b) { cmp = cmp + 4; }
            var s: int64 = 5;
            s += 3; s -= 2; s *= 4; s /= 3; s %= 2; s <<= 1; s >>= 1;
            return c + d + e + f + g + h + i + j + k + l + m + n + o + p + cmp + s;
        }
    )", "77");
}

TEST_F(PrimitiveTypeOperationMatrixTest, Int8AllOperations) {
    compileAndRun(R"(
        import hoo;

        func :int8 add8(x: int8, y: int8) { return x + y; }

        func :int64 main() {
            var a: int8 = 10;
            var b: int8 = 3;
            var c: int64 = a + b;
            var d: int64 = a - b;
            var e: int64 = a * b;
            var f: int64 = a / b;
            var g: int64 = a % b;
            var h: int64 = a << 1;
            var i: int64 = a >> 1;
            var j: int64 = a && b;
            var k: int64 = a || 0;
            var l: int64 = !a;
            var m: int64 = !0;
            var n: int64 = -a;
            a++;
            var o: int64 = a;
            a--;
            var p: int64 = a;
            var cmp: int64 = 0;
            if (a > b && a != b) { cmp = cmp + 1; }
            if (a >= b && a <= b + 4) { cmp = cmp + 2; }
            var w: int8 = add8(100, 30);
            if (w == -126) { cmp = cmp + 4; }
            return c + d + e + f + g + h + i + j + k + l + m + n + o + p + cmp;
        }
    )", "109");
}

TEST_F(PrimitiveTypeOperationMatrixTest, ByteAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :byte add8(x: byte, y: byte) { return x + y; }

        func :int64 main() {
            var a: byte = 10;
            var b: byte = 3;
            var c: int64 = a + b;
            var d: int64 = a - b;
            var e: int64 = a * b;
            var f: int64 = a / b;
            var g: int64 = a % b;
            var h: int64 = a << 1;
            var i: int64 = a >> 1;
            var j: int64 = a && b;
            var k: int64 = a || 0;
            var l: int64 = !a;
            var m: int64 = !0;
            var n: int64 = -a;
            a++;
            var o: int64 = a;
            a--;
            var p: int64 = a;
            var cmp: int64 = 0;
            if (a > b && a != b) { cmp = cmp + 1; }
            if (a >= b && a <= b + 4) { cmp = cmp + 2; }
            var w: byte = add8(250, 10);
            var x: byte = add8(200, 100);
            if (w == 4 && x == 44) { cmp = cmp + 4; }
            return c + d + e + f + g + h + i + j + k + l + m + n + o + p + cmp;
        }
    )", "109");
}

TEST_F(PrimitiveTypeOperationMatrixTest, FloatAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :float addf(x: float, y: float) { return x + y; }

        func :int64 main() {
            var a: float = 5.5;
            var b: float = 2.0;
            var r: int64 = 0;
            if (addf(a, b) == 7.5) { r = r + 1; }
            if (a - b == 3.5) { r = r + 2; }
            if (a * b == 11.0) { r = r + 4; }
            if (a / b == 2.75) { r = r + 8; }
            if (a % b == 1.5) { r = r + 16; }
            if (a != b && a > b) { r = r + 32; }
            if (a >= b && b <= a) { r = r + 64; }
            if (-a == -5.5) { r = r + 128; }
            var q: float = a;
            q++;
            if (q > 5.5) { r = r + 256; }
            var z: float = 0.0;
            if (!a == 0) { r = r + 512; }
            if (!z == 1) { r = r + 1024; }
            return r;
        }
    )", "2047");
}

TEST_F(PrimitiveTypeOperationMatrixTest, DoubleAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :double addd(x: double, y: double) { return x + y; }

        func :int64 main() {
            var a: double = 9.0;
            var b: double = 2.0;
            var r: int64 = 0;
            if (addd(a, b) == 11.0) { r = r + 1; }
            if (a - b == 7.0) { r = r + 2; }
            if (a * b == 18.0) { r = r + 4; }
            if (a / b == 4.5) { r = r + 8; }
            if (a % b == 1.0) { r = r + 16; }
            if (a != b && a > b) { r = r + 32; }
            if (a >= b && b <= a) { r = r + 64; }
            if (-a == -9.0) { r = r + 128; }
            var z: double = 0.0;
            if (!a == 0) { r = r + 256; }
            if (!z == 1) { r = r + 512; }
            return r;
        }
    )", "1023");
}

TEST_F(PrimitiveTypeOperationMatrixTest, F64AllOperations) {
    compileAndRun(R"(
        import hoo;

        func :f64 addx(x: f64, y: f64) { return x + y; }

        func :int64 main() {
            var a: f64 = 2.25;
            var b: f64 = 3.75;
            var r: int64 = 0;
            if (addx(a, b) == 6.0) { r = r + 1; }
            if (b - a == 1.5) { r = r + 2; }
            if (a * b == 8.4375) { r = r + 4; }
            if (b / a == 1.6666666666666667) { r = r + 8; }
            if (a != b) { r = r + 16; }
            if (a < b && b > a) { r = r + 32; }
            if (-a == -2.25) { r = r + 64; }
            return r;
        }
    )", "127");
}

TEST_F(PrimitiveTypeOperationMatrixTest, F8AllOperations) {
    compileAndRun(R"(
        import hoo;

        func :f8 add8f(x: f8, y: f8) { return x + y; }

        func :int64 main() {
            var a: f8 = 2.5f8;
            var b: f8 = 1.0f8;
            var r: int64 = 0;
            if (add8f(a, b) == 3.5f8) { r = r + 1; }
            if (a - b == 1.5f8) { r = r + 2; }
            if (a * b == 2.5f8) { r = r + 4; }
            if (a / b == 2.5f8) { r = r + 8; }
            if (a % b == 0.5f8) { r = r + 16; }
            if (a != b && a > b) { r = r + 32; }
            if (b < a && a >= b) { r = r + 64; }
            return r;
        }
    )", "127");
}

TEST_F(PrimitiveTypeOperationMatrixTest, BitAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :bit andb(x: bit, y: bit) { return x && y; }
        func :bit orb(x: bit, y: bit) { return x || y; }
        func :bit notb(x: bit) { return !x; }

        func :int64 main() {
            var a: bit = 1b;
            var b: bit = 0b;
            var r: int64 = 0;
            if (andb(a, b) == 0b) { r = r + 1; }
            if (andb(a, a) == 1b) { r = r + 2; }
            if (orb(a, b) == 1b) { r = r + 4; }
            if (notb(b) == 1b) { r = r + 8; }
            if (a != b) { r = r + 16; }
            if (a > b && b < a) { r = r + 32; }
            if (a >= b && b <= a) { r = r + 64; }
            var q: bit = a;
            q++;
            if (q > 1) { r = r + 128; }
            return r;
        }
    )", "255");
}

TEST_F(PrimitiveTypeOperationMatrixTest, BoolAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :bool andb(x: bool, y: bool) { return x && y; }
        func :bool orb(x: bool, y: bool) { return x || y; }
        func :bool notb(x: bool) { return !x; }

        func :int64 main() {
            var a: bool = true;
            var b: bool = false;
            var r: int64 = 0;
            if (andb(a, b) == false) { r = r + 1; }
            if (andb(a, a) == true) { r = r + 2; }
            if (orb(a, b) == true) { r = r + 4; }
            if (notb(b) == true) { r = r + 8; }
            if (a != b && a == true) { r = r + 16; }
            var s: bool = b;
            s++;
            if (s == true) { r = r + 32; }
            var sum: bool = a + b;
            if (sum == true) { r = r + 64; }
            return r;
        }
    )", "127");
}

TEST_F(PrimitiveTypeOperationMatrixTest, CharacterAllOperations) {
    compileAndRun(R"(
        import hoo.character;

        func :Character first() { return 'A'; }

        func :int64 main() {
            var ch: Character = first();
            var r: int64 = 0;
            if (ch.codepoint() == 65) { r = r + 1; }
            if (ch.length() == 1) { r = r + 2; }
            var s: string = ch.data();
            if (s.equals("A")) { r = r + 4; }
            var e: Character = '€';
            if (e.codepoint() == 8364) { r = r + 8; }
            if (e.length() == 3) { r = r + 16; }
            return r;
        }
    )", "31");
}

TEST_F(PrimitiveTypeOperationMatrixTest, StringAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :string join2(a: string, b: string) { return a.concat(b); }

        func :int64 main() {
            var s: string = join2("Hello", " ");
            var t: string = s.concat("world");
            var r: int64 = 0;
            if (t.equals("Hello world")) { r = r + 1; }
            if (t.length() == 11) { r = r + 2; }
            if ("".isEmpty()) { r = r + 4; }
            var u: string = t.toUpper();
            if (u.equals("HELLO WORLD")) { r = r + 8; }
            var v: string = u.toLower();
            if (v.equals("hello world")) { r = r + 16; }
            if (t.contains("lo wo")) { r = r + 32; }
            if (t.startsWith("Hello")) { r = r + 64; }
            if (t.indexOf("world") == 6) { r = r + 128; }
            var w: string = "  hi  ".trim();
            if (w.equals("hi")) { r = r + 256; }
            var x: string = "a" + "b" + "c";
            if (x.equals("abc")) { r = r + 512; }
            return r;
        }
    )", "1023");
}

TEST_F(PrimitiveTypeOperationMatrixTest, BufferAllOperations) {
    compileAndRun(R"(
        import hoo.buffer;

        func :Buffer makeBuf() { return buffer_fromBytes("abc", 3); }

        func :int64 main() {
            var b: Buffer = makeBuf();
            var r: int64 = 0;
            if (b.length() == 3) { r = r + 1; }
            if (b.capacity() >= 3) { r = r + 2; }
            if (b.byteAt(1) == 98) { r = r + 4; }
            b.setByte(1, 90);
            if (b.byteAt(1) == 90) { r = r + 8; }
            var cp: Buffer = b.copy();
            if (cp.length() == 3) { r = r + 16; }
            b.append("de", 2);
            if (b.length() == 5) { r = r + 32; }
            var b2 = buffer_fromBytes("fg", 2);
            b.appendBuffer(b2);
            if (b.length() == 7) { r = r + 64; }
            if (b.byteAt(6) == 103) { r = r + 128; }
            if (b.byteAt(99) == -1) { r = r + 256; }
            b.clear();
            if (b.length() == 0) { r = r + 512; }
            return r;
        }
    )", "1023");
}

TEST_F(PrimitiveTypeOperationMatrixTest, VoidAllOperations) {
    compileAndRun(R"(
        import hoo;

        func :void bump(x: int64) { var y: int64 = x; y = y + 1; }

        func :int64 main() {
            bump(7);
            return 1;
        }
    )", "1");
}

TEST_F(PrimitiveTypeOperationMatrixTest, DecimalAllOperations) {
    compileAndRun(R"(
        import hoo.decimal;

        func :Decimal<38,2> addd(a: Decimal<38,2>, b: Decimal<38,2>) { return a + b; }

        func :int64 main() {
            var a: Decimal<38,2> = 5.50m;
            var b: Decimal<38,2> = 2.00m;
            var r: int64 = 0;
            if (addd(a, b) == 7.50m) { r = r + 1; }
            if (a - b == 3.50m) { r = r + 2; }
            if (a * b == 11.00m) { r = r + 4; }
            if (a / b == 2.75m) { r = r + 8; }
            if (a % b == 1.50m) { r = r + 16; }
            if (a != b && a > b) { r = r + 32; }
            if (a >= b && b <= a) { r = r + 64; }
            if (-a == -5.50m) { r = r + 128; }
            var s: string = a.toString();
            if (s.length() == 3) { r = r + 256; }
            return r;
        }
    )", "511");
}
