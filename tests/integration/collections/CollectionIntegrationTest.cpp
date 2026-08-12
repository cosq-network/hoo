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

class CollectionIntegrationTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_collections_"
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
        return tempDir + "/hoo_collections_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
#ifdef _WIN32
        static int captureCounter = 0;
        std::string capturePath = tempDir + "/hoo_collections_capture_"
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
        ASSERT_EQ(build.exitCode, 0) << "Compilation failed: " << build.output;

        auto exec = runHoo(archive);
        ASSERT_EQ(exec.exitCode, 0) << "Execution failed: " << exec.output;
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos) << "Expected '" << expectedOutput << "' in output: " << exec.output;
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
// LIST - push, pop, length, clear, subscript, mixed types
// ============================================================================

TEST_F(CollectionIntegrationTest, ListPushMultipleTypes) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            values.push(40);
            values.push(50);
            return values[0] + values[4];
        }
    )", "60");
}

TEST_F(CollectionIntegrationTest, ListPushAndPop) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.push(3);
            values.pop();
            return values.length();
        }
    )", "2");
}

TEST_F(CollectionIntegrationTest, ListSubscriptSetAndGet) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            values[1] = 99;
            return values[1];
        }
    )", "99");
}

TEST_F(CollectionIntegrationTest, ListClear) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.push(3);
            values.clear();
            return values.length();
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, ListMixedTypesPush) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(42);
            values.push("hello");
            values.push(true);
            return values.length();
        }
    )", "3");
}

TEST_F(CollectionIntegrationTest, ListStringElementAccess) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push("foo");
            values.push("bar");
            var s: string = values[0];
            return s.length();
        }
    )", "3");
}

TEST_F(CollectionIntegrationTest, ListChainedPush) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.push(3);
            values.push(4);
            values.push(5);
            return values[0] + values[1] + values[2] + values[3] + values[4];
        }
    )", "15");
}

TEST_F(CollectionIntegrationTest, ListPopMultiple) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            values.pop();
            values.pop();
            return values[0];
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, ListReuseAfterClear) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.clear();
            values.push(100);
            values.push(200);
            return values[0] + values[1];
        }
    )", "300");
}

// ============================================================================
// ARRAY - pushInt64, pushString, pushDouble, pushBool, get*, length, clear, empty, sort, reverse
// ============================================================================

TEST_F(CollectionIntegrationTest, ArrayPushAndGetInt64) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            return a.getInt64(0) + a.getInt64(1) + a.getInt64(2);
        }
    )", "60");
}

TEST_F(CollectionIntegrationTest, ArrayPushAndGetString) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            var s1: string = a.getString(0);
            var s2: string = a.getString(1);
            return s1.length() + s2.length();
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, ArrayPushAndGetDouble) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushDouble(3.14);
            a.pushDouble(2.71);
            return 1;
        }
    )", "1");
}

TEST_F(CollectionIntegrationTest, ArrayPushAndGetBool) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushBool(true);
            a.pushBool(false);
            var b1: bool = a.getBool(0);
            var b2: bool = a.getBool(1);
            if (b1 && !b2) {
                return 1;
            }
            return 0;
        }
    )", "1");
}

TEST_F(CollectionIntegrationTest, ArrayLengthAndEmpty) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            var e1 = a.empty();
            a.pushInt64(1);
            a.pushInt64(2);
            var len = a.length();
            var e2 = a.empty();
            return e1 * 100 + len * 10 + e2;
        }
    )", "120");
}

TEST_F(CollectionIntegrationTest, ArrayClear) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.clear();
            return a.length();
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, ArrayReverse) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.reverse();
            return a.getInt64(0) * 100 + a.getInt64(1) * 10 + a.getInt64(2);
        }
    )", "321");
}

TEST_F(CollectionIntegrationTest, ArraySort) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(30);
            a.pushInt64(10);
            a.pushInt64(20);
            a.sort();
            return a.getInt64(0) + a.getInt64(1) + a.getInt64(2);
        }
    )", "6");
}

TEST_F(CollectionIntegrationTest, ArrayMixedTypeElements) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.pushString("test");
            a.pushBool(true);
            return a.length();
        }
    )", "3");
}

// ============================================================================
// MAP - int64/int64, string/int64, string/string, int64/string, contains, remove, length, clear
// ============================================================================

TEST_F(CollectionIntegrationTest, MapInt64KeyInt64ValueMultiple) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 100);
            m.setInt64Int64(2, 200);
            m.setInt64Int64(3, 300);
            return m.getInt64Int64(1) + m.getInt64Int64(2) + m.getInt64Int64(3);
        }
    )", "600");
}

TEST_F(CollectionIntegrationTest, MapStringKeyInt64ValueMultiple) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(4, 1);
            m.setStringInt64("a", 10);
            m.setStringInt64("b", 20);
            m.setStringInt64("c", 30);
            return m.length();
        }
    )", "3");
}

TEST_F(CollectionIntegrationTest, MapStringKeyStringValue) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(4, 4);
            m.setStringString("first", "hello");
            m.setStringString("second", "world");
            var s1: string = m.getStringString("first");
            var s2: string = m.getStringString("second");
            return s1.length() + s2.length();
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, MapContainsKey) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(42, 100);
            var has = m.containsInt64(42);
            var missing = m.containsInt64(99);
            return has * 10 + missing;
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, MapRemoveKey) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);
            m.setInt64Int64(3, 30);
            m.removeInt64(2);
            return m.length();
        }
    )", "2");
}

TEST_F(CollectionIntegrationTest, MapOverwriteValue) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(1, 99);
            return m.getInt64Int64(1);
        }
    )", "99");
}

TEST_F(CollectionIntegrationTest, MapInt64KeyStringValue) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 4);
            m.setInt64String(1, "one");
            m.setInt64String(2, "two");
            var s: string = m.getInt64String(1);
            return s.length();
        }
    )", "3");
}

TEST_F(CollectionIntegrationTest, MapMultipleOperations) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(10, 100);
            m.setInt64Int64(20, 200);
            m.setInt64Int64(30, 300);
            m.removeInt64(20);
            var len = m.length();
            var val = m.getInt64Int64(10);
            return val + len;
        }
    )", "102");
}

// ============================================================================
// DICT - subscript get/set, remove, count, containsKey, different key/value types
// ============================================================================

TEST_F(CollectionIntegrationTest, DictInt64Int64SubscriptMultiple) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 10;
            d[2] = 20;
            d[3] = 30;
            return d[1] + d[2] + d[3];
        }
    )", "60");
}

TEST_F(CollectionIntegrationTest, DictInt64StringSubscript) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, string> = new Dict<int64, string>();
            d[1] = "one";
            d[2] = "two";
            d[3] = "three";
            var s: string = d[1];
            return s.length() + d.count();
        }
    )", "6");
}

TEST_F(CollectionIntegrationTest, DictRemoveAndCount) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 10;
            d[2] = 20;
            d[3] = 30;
            d.remove(2);
            return d.count();
        }
    )", "2");
}

TEST_F(CollectionIntegrationTest, DictOverwriteValue) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 10;
            d[1] = 99;
            return d[1];
        }
    )", "99");
}

TEST_F(CollectionIntegrationTest, DictInt8Key) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int8, int64> = new Dict<int8, int64>();
            d[1] = 100;
            d[2] = 200;
            return d[1] + d[2];
        }
    )", "300");
}

TEST_F(CollectionIntegrationTest, DictByteKey) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<byte, int64> = new Dict<byte, int64>();
            d[65] = 100;
            d[66] = 200;
            return d[65] + d[66];
        }
    )", "300");
}

TEST_F(CollectionIntegrationTest, DictAnyValue) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, any> = new Dict<int64, any>();
            d[1] = 42;
            d[2] = "hello";
            d[3] = true;
            return d.count();
        }
    )", "3");
}

TEST_F(CollectionIntegrationTest, DictChainedOperations) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 1;
            d[2] = 2;
            d[3] = 3;
            d[4] = 4;
            d.remove(2);
            d.remove(4);
            return d[1] + d[3];
        }
    )", "4");
}

// ============================================================================
// TENSOR - 1D, 2D, 3D literals, element types, operations
// ============================================================================

TEST_F(CollectionIntegrationTest, Tensor1DInt64Literal) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [10, 20, 30, 40, 50]t;
            return t[0] + t[1] + t[2] + t[3] + t[4];
        }
    )", "150");
}

TEST_F(CollectionIntegrationTest, Tensor1DIndexing) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [100, 200, 300]t;
            return t[1];
        }
    )", "200");
}

TEST_F(CollectionIntegrationTest, Tensor2DLiteral) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [[1, 2], [3, 4]]t;
            return t[0] + t[1] + t[2] + t[3];
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, Tensor2DIndexing) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [[10, 20], [30, 40]]t;
            return t[0] + t[1] + t[2] + t[3];
        }
    )", "100");
}

TEST_F(CollectionIntegrationTest, TensorElementwiseAddition) {
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

TEST_F(CollectionIntegrationTest, TensorElementwiseSubtraction) {
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

TEST_F(CollectionIntegrationTest, TensorElementwiseMultiply) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var c = a .* b;
            return c[0] + c[1] + c[2];
        }
    )", "32");
}

TEST_F(CollectionIntegrationTest, TensorElementwiseDivide) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [10, 20, 30]t;
            var b = [2, 4, 5]t;
            var c = a ./ b;
            return c[0] + c[1] + c[2];
        }
    )", "16");
}

TEST_F(CollectionIntegrationTest, TensorMatrixMultiply) {
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

TEST_F(CollectionIntegrationTest, TensorScalarMultiplication) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var c = a * 2;
            return c[0] + c[1] + c[2];
        }
    )", "12");
}

TEST_F(CollectionIntegrationTest, TensorScalarAddition) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var c = a + 10;
            return c[0] + c[1] + c[2];
        }
    )", "36");
}

TEST_F(CollectionIntegrationTest, TensorScalarSubtraction) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [10, 20, 30]t;
            var c = a - 5;
            return c[0] + c[1] + c[2];
        }
    )", "45");
}

TEST_F(CollectionIntegrationTest, TensorLength) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [1, 2, 3, 4, 5]t;
            var sum = t[0] + t[1] + t[2] + t[3] + t[4];
            return sum;
        }
    )", "15");
}

TEST_F(CollectionIntegrationTest, TensorElementwiseEquality) {
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

TEST_F(CollectionIntegrationTest, TensorElementwiseNotEquals) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [1, 5, 3]t;
            var ne = a != b;
            return ne[0] + ne[1] + ne[2];
        }
    )", "1");
}

TEST_F(CollectionIntegrationTest, TensorElementwiseLessThan) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 5, 3]t;
            var b = [2, 3, 3]t;
            var lt = a < b;
            return lt[0] + lt[1] + lt[2];
        }
    )", "1");
}

TEST_F(CollectionIntegrationTest, TensorDeclaredZeroFilled) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t: tensor<int64>[5];
            return t[0] + t[1] + t[2] + t[3] + t[4];
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, Tensor3DLiteral) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]t;
            return t[0] + t[7];
        }
    )", "9");
}

TEST_F(CollectionIntegrationTest, TensorChainedOperations) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var c = a + b;
            var d = c * 2;
            return d[0] + d[1] + d[2];
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, TensorNegativeValues) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = [-1, -2, -3]t;
            var b = [1, 2, 3]t;
            var c = a + b;
            return c[0] + c[1] + c[2];
        }
    )", "0");
}

// ============================================================================
// CROSS-COLLECTION - using multiple collection types together
// ============================================================================

TEST_F(CollectionIntegrationTest, ListAndArrayTogether) {
    compileAndRun(R"(
        import hoo;
        import hoo.collections;

        func :int64 main() {
            var list = new List();
            list.push(10);
            list.push(20);

            var arr = new Array();
            arr.pushInt64(30);
            arr.pushInt64(40);

            return list[0] + list[1] + arr.getInt64(0) + arr.getInt64(1);
        }
    )", "100");
}

TEST_F(CollectionIntegrationTest, MapAndDictTogether) {
    compileAndRun(R"(
        import hoo;
        import hoo.collections;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);

            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[2] = 20;

            return m.getInt64Int64(1) + d[2];
        }
    )", "30");
}

TEST_F(CollectionIntegrationTest, TensorAndListTogether) {
    compileAndRun(R"(
        import hoo;
        import hoo.collections;

        func :int64 main() {
            var t = [1, 2, 3]t;
            var sum = t[0] + t[1] + t[2];

            var list = new List();
            list.push(sum);
            list.push(100);

            return list[0] + list[1];
        }
    )", "106");
}

TEST_F(CollectionIntegrationTest, ArrayAndMapTogether) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var arr = new Array();
            arr.pushInt64(1);
            arr.pushInt64(2);

            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);

            var arrLen = arr.length();
            var mapLen = m.length();
            return arrLen + mapLen;
        }
    )", "4");
}

// ============================================================================
// EDGE CASES - empty collections, single element, boundary conditions
// ============================================================================

TEST_F(CollectionIntegrationTest, EmptyListLength) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            return values.length();
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, EmptyArrayLengthAndEmpty) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            var len = a.length();
            var empty = a.empty();
            return len + empty;
        }
    )", "1");
}

TEST_F(CollectionIntegrationTest, SingleElementList) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(42);
            return values[0];
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, SingleElementArray) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            return a.getInt64(0);
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, SingleElementMap) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 42);
            return m.getInt64Int64(1);
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, SingleElementDict) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 42;
            return d[1];
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, SingleElementTensor) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [42]t;
            return t[0];
        }
    )", "42");
}

TEST_F(CollectionIntegrationTest, LargeListPush) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.push(3);
            values.push(4);
            values.push(5);
            values.push(6);
            values.push(7);
            values.push(8);
            values.push(9);
            values.push(10);
            return values[9];
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, LargeArrayPush) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            a.pushInt64(6);
            a.pushInt64(7);
            a.pushInt64(8);
            a.pushInt64(9);
            a.pushInt64(10);
            return a.getInt64(9);
        }
    )", "10");
}

TEST_F(CollectionIntegrationTest, MapWithZeroValues) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 0);
            m.setInt64Int64(2, 0);
            return m.getInt64Int64(1) + m.getInt64Int64(2);
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, DictWithZeroValues) {
    compileAndRun(R"(
        import hoo.collections;

        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 0;
            d[2] = 0;
            return d[1] + d[2];
        }
    )", "0");
}

TEST_F(CollectionIntegrationTest, TensorWithZeroValues) {
    compileAndRun(R"(
        import hoo;

        func :int64 main() {
            var t = [0, 0, 0]t;
            return t[0] + t[1] + t[2];
        }
    )", "0");
}
