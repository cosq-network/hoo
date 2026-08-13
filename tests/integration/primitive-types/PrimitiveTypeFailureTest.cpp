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

// Failure matrix: invalid operations on primitive types that the compiler
// rejects at build time or that fail at runtime. Each test compiles the
// program to a .ha archive, then either asserts the build fails with the
// expected diagnostic, or asserts the archive execution fails.
class PrimitiveTypeFailureTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_fail_"
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
        return tempDir + "/hoo_fail_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
#ifdef _WIN32
        static int captureCounter = 0;
        std::string capturePath = tempDir + "/hoo_fail_capture_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++captureCounter)
            + ".txt";
        std::string cmd = "cmd.exe /S /C \"" + hooExe + "\" " + args
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

    void expectCompileFailure(const std::string& source, const std::string& errorFragment) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();
        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_NE(build.exitCode, 0) << "Expected compile failure but build succeeded";
        EXPECT_NE(build.output.find(errorFragment), std::string::npos)
            << "Expected error fragment \"" << errorFragment << "\" not found in: " << build.output;
    }

    void expectRunFailure(const std::string& source, const std::string& errorFragment) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();
        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_EQ(build.exitCode, 0) << "Expected build to succeed but it failed: " << build.output;
        auto exec = runHoo(archive);
        ASSERT_NE(exec.exitCode, 0) << "Expected run failure but archive executed successfully";
        if (!errorFragment.empty()) {
            EXPECT_NE(exec.output.find(errorFragment), std::string::npos)
                << "Expected error fragment \"" << errorFragment << "\" not found in: " << exec.output;
        }
    }
};

TEST_F(PrimitiveTypeFailureTest, Int64MethodCallOnValue) {
    expectCompileFailure(R"(
        import hoo;
        func :int64 main() { var x: int64 = 5; var r: int64 = x.length(); return 0; }
    )", "Cannot resolve method");
}

TEST_F(PrimitiveTypeFailureTest, DecimalShiftNotSupported) {
    expectCompileFailure(R"(
        import hoo.decimal;
        func :int64 main() { var a: Decimal<38,2> = 1.5m; var b: Decimal<38,2> = a << 1; return 0; }
    )", "Decimal operands must both be Decimal types");
}

TEST_F(PrimitiveTypeFailureTest, UndefinedVariable) {
    expectCompileFailure(R"(
        import hoo;
        func :int64 main() { return undefinedVar; }
    )", "Undefined variable");
}

TEST_F(PrimitiveTypeFailureTest, AnyVariableDeclaration) {
    expectCompileFailure(R"(
        import hoo;
        func :int64 main() { var a: any = 5; return 0; }
    )", "'any' meta type is not allowed");
}

TEST_F(PrimitiveTypeFailureTest, BufferSliceIsKeyword) {
    expectCompileFailure(R"(
        import hoo.buffer;
        func :int64 main() { var b = Buffer.fromBytes("abc", 3); var s = b.slice(1, 2); return 0; }
    )", "Parse errors");
}

TEST_F(PrimitiveTypeFailureTest, StringSubstringNotLinked) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var s: string = "abc"; var t: string = s.substring(0, 1); return 0; }
    )", "Symbols not found");
}

TEST_F(PrimitiveTypeFailureTest, Int64DivisionByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: int64 = 5; var b: int64 = 0; var c: int64 = a / b; return c; }
    )", "Division by zero");
}

TEST_F(PrimitiveTypeFailureTest, Int64ModuloByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: int64 = 5; var b: int64 = 0; var c: int64 = a % b; return c; }
    )", "Modulo by zero");
}

TEST_F(PrimitiveTypeFailureTest, Int8DivisionByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: int8 = 5; var b: int8 = 0; var c: int8 = a / b; return 0; }
    )", "8-bit division by zero");
}

TEST_F(PrimitiveTypeFailureTest, Int8ModuloByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: int8 = 5; var b: int8 = 0; var c: int8 = a % b; return 0; }
    )", "8-bit remainder by zero");
}

TEST_F(PrimitiveTypeFailureTest, ByteDivisionByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: byte = 5; var b: byte = 0; var c: byte = a / b; return 0; }
    )", "8-bit unsigned division by zero");
}

TEST_F(PrimitiveTypeFailureTest, ByteModuloByZero) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var a: byte = 5; var b: byte = 0; var c: byte = a % b; return 0; }
    )", "8-bit unsigned remainder by zero");
}

TEST_F(PrimitiveTypeFailureTest, WrongArgumentCount) {
    expectRunFailure(R"(
        import hoo;
        func :int64 f() { return 1; }
        func :int64 main() { return f(2); }
    )", "Symbols not found");
}

TEST_F(PrimitiveTypeFailureTest, ArgumentTypeMismatchCrashes) {
    expectRunFailure(R"(
        import hoo;
        func :string f(x: string) { return x; }
        func :int64 main() { var r: string = f(42); return 0; }
    )", "");
}

TEST_F(PrimitiveTypeFailureTest, Int64ToStringAssignmentCrashes) {
    expectRunFailure(R"(
        import hoo;
        func :int64 main() { var x: int64 = 5; var s: string = x; return 0; }
    )", "");
}
