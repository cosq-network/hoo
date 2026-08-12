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

// Integration tests for composite/built-in collection types: Array, Map,
// HashMap, Tensor, Future, Optional, Slice, any, AnyArray. Each test is a
// complete program compiled to a .ha archive and executed via the hoo CLI.
class CompositeTypesCLIIntegrationTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_composite_"
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
        return tempDir + "/hoo_composite_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
#ifdef _WIN32
        static int captureCounter = 0;
        std::string capturePath = tempDir + "/hoo_composite_capture_"
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

    void compileAndRun(const std::string& source, const std::string& expectedOutput) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();

        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_EQ(build.exitCode, 0) << build.output;

        auto exec = runHoo(archive);
        ASSERT_EQ(exec.exitCode, 0) << exec.output;
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos) << exec.output;
    }

    void expectCompileFailure(const std::string& source, const std::string& errorFragment) {
        std::string src = createSourceFile(source);
        std::string archive = createArchivePath();
        auto build = runHoo("-o " + archive + " " + src);
        ASSERT_NE(build.exitCode, 0) << "Expected compile failure but build succeeded";
        EXPECT_NE(build.output.find(errorFragment), std::string::npos)
            << "Expected error fragment \"" << errorFragment << "\" not found in: " << build.output;
    }
};

// ============================================================================
// Array
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, ArrayPushGetInt64) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            Array.pushInt64(a, 42);
            Array.pushInt64(a, 100);
            return Array.getInt64(a, 0) + Array.getInt64(a, 1);
        }
    )", "142");
}

TEST_F(CompositeTypesCLIIntegrationTest, ArrayPushGetString) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            Array.pushString(a, "hello");
            Array.pushString(a, "world");
            var s1 = Array.getString(a, 0);
            var s2 = Array.getString(a, 1);
            return s1.length() + s2.length();
        }
    )", "10");
}

TEST_F(CompositeTypesCLIIntegrationTest, ArrayLengthClearEmpty) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            Array.pushInt64(a, 1);
            Array.pushInt64(a, 2);
            var len = Array.length(a);
            Array.clear(a);
            var empty = Array.empty(a);
            return len * 10 + empty;
        }
    )", "21");
}

// ============================================================================
// Map
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, MapInt64KeyInt64Value) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(42, 100);
            m.setInt64Int64(7, 200);
            return m.getInt64Int64(42) + m.getInt64Int64(7);
        }
    )", "300");
}

TEST_F(CompositeTypesCLIIntegrationTest, MapStringKeyInt64Value) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(4, 1);
            m.setStringInt64("a", 10);
            m.setStringInt64("b", 20);
            return m.getInt64Int64(0) + m.length() * 10;
        }
    )", "20");
}

TEST_F(CompositeTypesCLIIntegrationTest, MapContainsAndRemove) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);
            var c = m.containsInt64(1);
            m.removeInt64(1);
            var len = m.length();
            return c * 10 + len;
        }
    )", "11");
}

TEST_F(CompositeTypesCLIIntegrationTest, MapStringKeyStringValue) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(4, 4);
            m.setStringString("name", "hoo");
            m.setStringString("version", "1.4");
            var s: string = m.getStringString("name");
            return s.length();
        }
    )", "3");
}

// ============================================================================
// HashMap
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, HashMapInt64KeySubscript) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            return m[10] + m.count();
        }
    )", "44");
}

TEST_F(CompositeTypesCLIIntegrationTest, HashMapRemoveAndCount) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            m.remove(10);
            return m.count();
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, HashMapInt64KeyStringValue) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var m: HashMap<int64, string> = new HashMap<int64, string>();
            m[1] = "hello";
            var s: string = m[1];
            return s.length();
        }
    )", "5");
}

TEST_F(CompositeTypesCLIIntegrationTest, HashMapAnyValueMixedTypes) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var m: HashMap<int64, any> = new HashMap<int64, any>();
            m[1] = 42;
            m[2] = "hello";
            return m.count();
        }
    )", "2");
}

// ============================================================================
// Tensor
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, TensorLiteralIndexing) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var values = [10, 20, 30]t;
            return values[0] + values[2];
        }
    )", "40");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorDeclaredZeroFilled) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var values: tensor<int64>[3];
            return values[0] + values[1] + values[2];
        }
    )", "0");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorElementwiseAdd) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var c = a + b;
            return c[0] + c[1] + c[2];
        }
    )", "21");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorElementwiseSub) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [10, 20, 30]t;
            var b = [1, 2, 3]t;
            var c = a - b;
            return c[0] + c[1] + c[2];
        }
    )", "54");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorScalarMul) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var c = a * 2;
            return c[0] + c[1] + c[2];
        }
    )", "12");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorMatrixMultiply) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [[1, 2], [3, 4]]t;
            var b = [[5, 6], [7, 8]]t;
            var c = a * b;
            return c[0] + c[3];
        }
    )", "69");
}

TEST_F(CompositeTypesCLIIntegrationTest, TensorElementwiseEquality) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [1, 2, 3]t;
            var eq = a == b;
            return eq[0] + eq[1] + eq[2];
        }
    )", "3");
}

// ============================================================================
// Future
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, FutureAsyncFunctionCompiles) {
    compileAndRun(R"(
        import hoo;

        async func:Future<int64> getVal() {
            return 42;
        }

        func :int64 main() {
            return 1;
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, FutureTypeAsParameter) {
    compileAndRun(R"(
        import hoo;

        func :int64 process(f: Future<int64>) {
            return 1;
        }

        func :int64 main() {
            return 1;
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, FutureTypeAsVariable) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var f: Future<int64>;
            return 1;
        }
    )", "1");
}

// ============================================================================
// Optional
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, OptionalVariableDeclaration) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var x: int64? = 42;
            return 1;
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, OptionalNullDeclaration) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var x: int64?;
            return 1;
        }
    )", "1");
}

// ============================================================================
// Slice
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, SliceTypeAsVariable) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var s: slice<byte>;
            return 1;
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, SliceTypeAsParameter) {
    compileAndRun(R"(
        import hoo;

        func :int64 process(s: slice<byte>) {
            return 1;
        }

        func :int64 main() {
            return 1;
        }
    )", "1");
}

// ============================================================================
// any
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, AnyReturnType) {
    compileAndRun(R"(
        import hoo;

        func :any getAny() {
            return 42;
        }

        func :int64 main() {
            var x = getAny();
            return 1;
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyInHashMapValue) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var m: HashMap<int64, any> = new HashMap<int64, any>();
            m[1] = 42;
            m[2] = "hello";
            return m.count();
        }
    )", "2");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyParameterRejected) {
    expectCompileFailure(R"(
        import hoo;

        func :int64 process(x: any) {
            return 1;
        }

        func :int64 main() {
            return process(42);
        }
    )", "'any' meta type is not allowed");
}

// ============================================================================
// AnyArray
// ============================================================================

TEST_F(CompositeTypesCLIIntegrationTest, AnyArrayPushAndLength) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new AnyArray();
            values.push(5);
            values.push(8);
            values[0] = 7;
            return values[0] + values.length();
        }
    )", "9");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyArrayPop) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new AnyArray();
            values.push(10);
            values.push(20);
            values.pop();
            return values.length();
        }
    )", "1");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyArrayMixedTypes) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new AnyArray();
            values.push(42);
            values.push("hello");
            return values.length();
        }
    )", "2");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyArrayStringElement) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new AnyArray();
            values.push("hello");
            values.push("world");
            var s: string = values[0];
            return s.length();
        }
    )", "5");
}

TEST_F(CompositeTypesCLIIntegrationTest, AnyArrayLiteral) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = [1, 2, 3];
            return values[0] + values[1] + values[2];
        }
    )", "6");
}
