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

class GenericArrayCLIIntegrationTest : public ::testing::Test {
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
        std::string path = tempDir + "/hoo_generic_array_"
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
        return tempDir + "/hoo_generic_array_out_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
#ifdef _WIN32
        static int captureCounter = 0;
        std::string capturePath = tempDir + "/hoo_ga_capture_"
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
        EXPECT_NE(exec.output.find(expectedOutput), std::string::npos)
            << "Expected '" << expectedOutput << "' in output: " << exec.output;
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
// Creation and Destruction
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, NewEmptyArray) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            return a.length();
        }
    )", "0");
}

TEST_F(GenericArrayCLIIntegrationTest, NewArrayEmptyCheck) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            return a.empty();
        }
    )", "1");
}

// ============================================================================
// Type-Specific Push/Get Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PushGetInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            return a.getInt64(0);
        }
    )", "42");
}

TEST_F(GenericArrayCLIIntegrationTest, PushGetDouble) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(3.14);
            var d: double = a.getDouble(0);
            if (d > 3.13 && d < 3.15) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, PushGetBool) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushBool(true);
            a.pushBool(false);
            var b1: bool = a.getBool(0);
            var b2: bool = a.getBool(1);
            if (b1 && !b2) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, PushGetString) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushString("hello");
            var s: string = a.getString(0);
            return s.length();
        }
    )", "5");
}

// ============================================================================
// Length and Empty
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ArrayLengthMultiplePushes) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            return a.length();
        }
    )", "3");
}

TEST_F(GenericArrayCLIIntegrationTest, ArrayEmptyAfterPush) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var e1 = a.empty();
            a.pushInt64(1);
            var e2 = a.empty();
            return e1 * 10 + e2;
        }
    )", "10");
}

// ============================================================================
// Large Push Operation
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PushManyElements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 10) {
                a.pushInt64(i * 10);
                i = i + 1;
            }
            return a.length();
        }
    )", "10");
}

TEST_F(GenericArrayCLIIntegrationTest, PushVerifyAllElements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 10) {
                a.pushInt64(i * 10);
                i = i + 1;
            }
            var sum: int64 = 0;
            i = 0;
            while (i < 10) {
                sum = sum + a.getInt64(i);
                i = i + 1;
            }
            return sum;
        }
    )", "450");
}

// ============================================================================
// Clear Operation
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ClearMakesEmpty) {
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

TEST_F(GenericArrayCLIIntegrationTest, ClearThenReuse) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.clear();
            a.pushInt64(10);
            a.pushInt64(20);
            return a.getInt64(0) + a.getInt64(1);
        }
    )", "30");
}

// ============================================================================
// Sort Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, SortInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(5);
            a.pushInt64(3);
            a.pushInt64(9);
            a.pushInt64(1);
            a.sort();
            return a.getInt64(0) * 1000 + a.getInt64(1) * 100 + a.getInt64(2) * 10 + a.getInt64(3);
        }
    )", "1359");
}

TEST_F(GenericArrayCLIIntegrationTest, SortDouble) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(5.5);
            a.pushDouble(1.1);
            a.pushDouble(3.3);
            a.sort();
            var first: double = a.getDouble(0);
            var last: double = a.getDouble(2);
            if (first < 1.2 && last > 5.4) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, SortEmpty) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.sort();
            return a.length();
        }
    )", "0");
}

TEST_F(GenericArrayCLIIntegrationTest, SortSingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.sort();
            return a.getInt64(0);
        }
    )", "42");
}

// ============================================================================
// Reverse Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ReverseInt64) {
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

TEST_F(GenericArrayCLIIntegrationTest, ReverseEmpty) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.reverse();
            return a.length();
        }
    )", "0");
}

TEST_F(GenericArrayCLIIntegrationTest, ReverseSingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(99);
            a.reverse();
            return a.getInt64(0);
        }
    )", "99");
}

TEST_F(GenericArrayCLIIntegrationTest, SortThenReverse) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            a.sort();
            a.reverse();
            return a.getInt64(0) * 100 + a.getInt64(1) * 10 + a.getInt64(2);
        }
    )", "321");
}

// ============================================================================
// Shuffle Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ShufflePreservesLength) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            a.shuffle();
            return a.length();
        }
    )", "5");
}

TEST_F(GenericArrayCLIIntegrationTest, ShuffleSingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.shuffle();
            return a.getInt64(0);
        }
    )", "42");
}

TEST_F(GenericArrayCLIIntegrationTest, ShuffleThenSort) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 20) {
                a.pushInt64(i);
                i = i + 1;
            }
            a.shuffle();
            a.sort();
            var r = 1;
            i = 0;
            while (i < 20) {
                if (a.getInt64(i) != i) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, ShufflePreservesSum) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 10) {
                a.pushInt64(i + 1);
                i = i + 1;
            }
            a.shuffle();
            var sum: int64 = 0;
            i = 0;
            while (i < 10) {
                sum = sum + a.getInt64(i);
                i = i + 1;
            }
            return sum;
        }
    )", "55");
}

// ============================================================================
// Sort Range Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, SortRangeSubRange) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(5);
            a.pushInt64(4);
            a.pushInt64(3);
            a.pushInt64(2);
            a.pushInt64(1);
            a.sortRange(1, 4);
            var r = 1;
            if (a.getInt64(0) != 5) { r = 0; }
            if (a.getInt64(1) != 2) { r = 0; }
            if (a.getInt64(2) != 3) { r = 0; }
            if (a.getInt64(3) != 4) { r = 0; }
            if (a.getInt64(4) != 1) { r = 0; }
            return r;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, SortRangePreservesOutsideElements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(100);
            a.pushInt64(90);
            a.pushInt64(80);
            a.pushInt64(70);
            a.pushInt64(60);
            a.sortRange(1, 4);
            var r = 1;
            if (a.getInt64(0) != 100) { r = 0; }
            if (a.getInt64(1) != 70) { r = 0; }
            if (a.getInt64(2) != 80) { r = 0; }
            if (a.getInt64(3) != 90) { r = 0; }
            if (a.getInt64(4) != 60) { r = 0; }
            return r;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, SortRangeFullArray) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(5);
            a.pushInt64(3);
            a.pushInt64(1);
            a.pushInt64(4);
            a.pushInt64(2);
            a.sortRange(0, 5);
            var r = 1;
            var i: int64 = 0;
            while (i < 5) {
                if (a.getInt64(i) != i + 1) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )", "1");
}

// ============================================================================
// Binary Search Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchFindsExistingValue) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            a.pushInt64(40);
            var idx = a.binarySearch(20);
            return idx;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchReturnsNegativeForMissing) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            var idx = a.binarySearch(25);
            return idx;
        }
    )", "-1");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchFirstElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            var idx = a.binarySearch(10);
            return idx;
        }
    )", "0");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchLastElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            var idx = a.binarySearch(30);
            return idx;
        }
    )", "2");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchSingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            var idx = a.binarySearch(42);
            var missing = a.binarySearch(99);
            if (idx == 0 && missing == -1) { return 1; }
            return 0;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchAfterSort) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(50);
            a.pushInt64(10);
            a.pushInt64(40);
            a.pushInt64(20);
            a.pushInt64(30);
            a.sort();
            var r = 1;
            if (a.binarySearch(10) != 0) { r = 0; }
            if (a.binarySearch(30) != 2) { r = 0; }
            if (a.binarySearch(50) != 4) { r = 0; }
            if (a.binarySearch(25) != -1) { r = 0; }
            return r;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, BinarySearchEmptyArray) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var idx = a.binarySearch(42);
            return idx;
        }
    )", "-1");
}

// ============================================================================
// Push Chaining and Multiple Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PushChainingMultipleInt64) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            var sum: int64 = 0;
            var i: int64 = 0;
            while (i < 5) {
                sum = sum + a.getInt64(i);
                i = i + 1;
            }
            return sum;
        }
    )", "15");
}

TEST_F(GenericArrayCLIIntegrationTest, PushChainingStrings) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            a.pushString("!");
            var total: int64 = 0;
            var i: int64 = 0;
            while (i < 3) {
                var s: string = a.getString(i);
                total = total + s.length();
                i = i + 1;
            }
            return total;
        }
    )", "11");
}

TEST_F(GenericArrayCLIIntegrationTest, PushChainingBools) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushBool(true);
            a.pushBool(false);
            a.pushBool(true);
            var count: int64 = 0;
            var i: int64 = 0;
            while (i < 3) {
                var b: bool = a.getBool(i);
                if (b) { count = count + 1; }
                i = i + 1;
            }
            return count;
        }
    )", "2");
}

// ============================================================================
// Negative Values
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, NegativeInt64Values) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(-5);
            a.pushInt64(-1);
            a.pushInt64(-10);
            a.pushInt64(0);
            a.pushInt64(3);
            a.sort();
            var r = 1;
            if (a.getInt64(0) != -10) { r = 0; }
            if (a.getInt64(1) != -5) { r = 0; }
            if (a.getInt64(2) != -1) { r = 0; }
            if (a.getInt64(3) != 0) { r = 0; }
            if (a.getInt64(4) != 3) { r = 0; }
            return r;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, NegativeDoubleValues) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(-3.14);
            a.pushDouble(2.71);
            a.pushDouble(-1.0);
            a.sort();
            var first: double = a.getDouble(0);
            var last: double = a.getDouble(2);
            if (first > -3.15 && first < -3.13) { return 1; }
            if (last > 2.70 && last < 2.72) { return 2; }
            return 0;
        }
    )", "1");
}

// ============================================================================
// Mixed Types
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, MixedTypePush) {
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

TEST_F(GenericArrayCLIIntegrationTest, MixedTypeAccess) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.pushString("hello");
            var num: int64 = a.getInt64(0);
            var s: string = a.getString(1);
            return num + s.length();
        }
    )", "47");
}

// ============================================================================
// Type Mismatch Rejection
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, TypeMismatchPushInt64OnDoubleArray) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(1.0);
            a.pushInt64(42);
            return a.length();
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, TypeMismatchPushDoubleOnInt64Array) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushDouble(2.5);
            return a.length();
        }
    )", "1");
}

// ============================================================================
// Nested Arrays
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, NestedArrayBasic) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var outer = new Array();
            var inner1 = new Array();
            inner1.pushInt64(10);
            inner1.pushInt64(20);
            var inner2 = new Array();
            inner2.pushInt64(30);
            inner2.pushInt64(40);
            outer.pushObject(inner1);
            outer.pushObject(inner2);
            if (outer.length() != 2) { return 0; }
            var a1: Array = outer.getObject(0);
            var a2: Array = outer.getObject(1);
            if (a1.getInt64(0) != 10) { return 0; }
            if (a1.getInt64(1) != 20) { return 0; }
            if (a2.getInt64(0) != 30) { return 0; }
            if (a2.getInt64(1) != 40) { return 0; }
            return 1;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, NestedArrayInnerLength) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var outer = new Array();
            var inner = new Array();
            inner.pushInt64(1);
            inner.pushInt64(2);
            inner.pushInt64(3);
            outer.pushObject(inner);
            var a: Array = outer.getObject(0);
            return a.length();
        }
    )", "3");
}

// ============================================================================
// Class Instances in Array
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ClassInstanceArray) {
    compileAndRun(R"(
        import hoo;
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
            func :int64 manhattan() {
                return this.x + this.y;
            }
        }
        func :int64 main() {
            var a = new Array();
            var p1 = new Point(3, 4);
            var p2 = new Point(10, 20);
            a.pushObject(p1);
            a.pushObject(p2);
            if (a.length() != 2) { return 0; }
            var r1: Point = a.getObject(0);
            var r2: Point = a.getObject(1);
            if (r1.manhattan() != 7) { return 0; }
            if (r2.manhattan() != 30) { return 0; }
            return 1;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, ClassInstanceMutationViaArray) {
    compileAndRun(R"(
        import hoo;
        class Counter {
            var val: int64;
            constructor(start: int64) {
                this.val = start;
            }
            func :void increment() {
                this.val = this.val + 1;
            }
            func :int64 getVal() {
                return this.val;
            }
        }
        func :int64 main() {
            var a = new Array();
            var c = new Counter(10);
            a.pushObject(c);
            c.increment();
            c.increment();
            var ref: Counter = a.getObject(0);
            return ref.getVal();
        }
    )", "12");
}

// ============================================================================
// Large Array Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, LargeInt64Array) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 100) {
                a.pushInt64(i * 2);
                i = i + 1;
            }
            if (a.length() != 100) { return 0; }
            if (a.getInt64(0) != 0) { return 0; }
            if (a.getInt64(99) != 198) { return 0; }
            if (a.getInt64(50) != 100) { return 0; }
            return 1;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, SortThenAccessAllElements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 49;
            while (i >= 0) {
                a.pushInt64(i);
                i = i - 1;
            }
            a.sort();
            var r = 1;
            i = 0;
            while (i < 50) {
                if (a.getInt64(i) != i) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )", "1");
}

// ============================================================================
// Edge Cases: Empty and Single Element
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, EmptyArrayAllOperations) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            if (a.length() != 0) { return 0; }
            if (a.empty() != 1) { return 0; }
            a.sort();
            if (a.length() != 0) { return 0; }
            a.reverse();
            if (a.length() != 0) { return 0; }
            a.shuffle();
            if (a.length() != 0) { return 0; }
            a.sortRange(0, 0);
            if (a.length() != 0) { return 0; }
            return 1;
        }
    )", "1");
}

TEST_F(GenericArrayCLIIntegrationTest, SingleElementAllOperations) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.sort();
            if (a.getInt64(0) != 42) { return 0; }
            a.reverse();
            if (a.getInt64(0) != 42) { return 0; }
            a.shuffle();
            if (a.getInt64(0) != 42) { return 0; }
            a.sortRange(0, 1);
            if (a.getInt64(0) != 42) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Multiple Clear and Reuse Cycles
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, MultipleClearAndReuse) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var round: int64 = 0;
            while (round < 5) {
                a.clear();
                var i: int64 = 0;
                while (i < 10) {
                    a.pushInt64(i + round * 10);
                    i = i + 1;
                }
                if (a.length() != 10) { return 0; }
                if (a.getInt64(0) != round * 10) { return 0; }
                if (a.getInt64(9) != round * 10 + 9) { return 0; }
                round = round + 1;
            }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Comprehensive Mixed Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ComprehensiveMixedOperations) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(100);
            a.pushInt64(50);
            a.pushInt64(200);
            a.pushInt64(25);
            a.pushInt64(150);
            if (a.length() != 5) { return 0; }
            if (a.empty() != 0) { return 0; }
            a.sort();
            if (a.getInt64(0) != 25) { return 0; }
            if (a.getInt64(4) != 200) { return 0; }
            a.reverse();
            if (a.getInt64(0) != 200) { return 0; }
            if (a.getInt64(4) != 25) { return 0; }
            var idx = a.binarySearch(100);
            if (idx < 0) { return 0; }
            a.sort();
            if (a.getInt64(0) != 25) { return 0; }
            if (a.getInt64(1) != 50) { return 0; }
            if (a.getInt64(2) != 100) { return 0; }
            if (a.getInt64(3) != 150) { return 0; }
            if (a.getInt64(4) != 200) { return 0; }
            a.clear();
            if (a.length() != 0) { return 0; }
            if (a.empty() != 1) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Reverse Then Sort
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ReverseThenSort) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            a.reverse();
            a.sort();
            var r = 1;
            var i: int64 = 0;
            while (i < 5) {
                if (a.getInt64(i) != i + 1) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )", "1");
}

// ============================================================================
// Double Sort and Reverse
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, DoubleSortAndReverse) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(1.0);
            a.pushDouble(2.0);
            a.pushDouble(3.0);
            a.sort();
            a.reverse();
            var r = 1;
            if (a.getDouble(0) != 3.0) { r = 0; }
            if (a.getDouble(1) != 2.0) { r = 0; }
            if (a.getDouble(2) != 1.0) { r = 0; }
            return r;
        }
    )", "1");
}

// ============================================================================
// Bool Array Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, BoolArrayReverse) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushBool(true);
            a.pushBool(false);
            a.pushBool(true);
            a.pushBool(true);
            a.pushBool(false);
            a.reverse();
            var r = 1;
            if (a.getBool(0) != false) { r = 0; }
            if (a.getBool(1) != true) { r = 0; }
            if (a.getBool(2) != true) { r = 0; }
            if (a.getBool(3) != false) { r = 0; }
            if (a.getBool(4) != true) { r = 0; }
            return r;
        }
    )", "1");
}

// ============================================================================
// String Array Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, StringArrayLengthSum) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            a.pushString("foo");
            a.pushString("bar");
            var total: int64 = 0;
            var i: int64 = 0;
            while (i < 4) {
                var s: string = a.getString(i);
                total = total + s.length();
                i = i + 1;
            }
            return total;
        }
    )", "16");
}

TEST_F(GenericArrayCLIIntegrationTest, StringArrayClear) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            a.clear();
            return a.length();
        }
    )", "0");
}

// ============================================================================
// Push Object and Array (Nested)
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PushArrayNested) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var outer = new Array();
            var inner1 = new Array();
            inner1.pushInt64(10);
            inner1.pushInt64(20);
            var inner2 = new Array();
            inner2.pushInt64(30);
            inner2.pushInt64(40);
            outer.pushObject(inner1);
            outer.pushObject(inner2);
            if (outer.length() != 2) { return 0; }
            var a1: Array = outer.getObject(0);
            var a2: Array = outer.getObject(1);
            if (a1.length() != 2) { return 0; }
            if (a2.length() != 2) { return 0; }
            if (a1.getInt64(0) != 10) { return 0; }
            if (a1.getInt64(1) != 20) { return 0; }
            if (a2.getInt64(0) != 30) { return 0; }
            if (a2.getInt64(1) != 40) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Pop Specific Values
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PopReturnsCorrectValues) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            a.clear();
            a.pushInt64(100);
            a.pushInt64(200);
            if (a.getInt64(0) != 100) { return 0; }
            if (a.getInt64(1) != 200) { return 0; }
            if (a.length() != 2) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Sort with Duplicate Values
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, SortWithDuplicates) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(3);
            a.pushInt64(1);
            a.pushInt64(3);
            a.pushInt64(1);
            a.pushInt64(2);
            a.sort();
            var r = 1;
            if (a.getInt64(0) != 1) { r = 0; }
            if (a.getInt64(1) != 1) { r = 0; }
            if (a.getInt64(2) != 2) { r = 0; }
            if (a.getInt64(3) != 3) { r = 0; }
            if (a.getInt64(4) != 3) { r = 0; }
            return r;
        }
    )", "1");
}

// ============================================================================
// Push String and Get Different Indices
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, StringArrayMultipleIndices) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushString("banana");
            a.pushString("apple");
            a.pushString("cherry");
            a.pushString("date");
            a.pushString("fig");
            if (a.length() != 5) { return 0; }
            var s1: string = a.getString(0);
            var s2: string = a.getString(4);
            if (s1.length() != 6) { return 0; }
            if (s2.length() != 3) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Double Array Push and Access Multiple Indices
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, DoubleArrayMultipleIndices) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(1.1);
            a.pushDouble(2.2);
            a.pushDouble(3.3);
            a.pushDouble(4.4);
            var r = 1;
            if (a.getDouble(0) != 1.1) { r = 0; }
            if (a.getDouble(1) != 2.2) { r = 0; }
            if (a.getDouble(2) != 3.3) { r = 0; }
            if (a.getDouble(3) != 4.4) { r = 0; }
            return r;
        }
    )", "1");
}

// ============================================================================
// Push then Clear then Push again
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, PushClearPushCycle) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            if (a.length() != 3) { return 0; }
            a.clear();
            if (a.length() != 0) { return 0; }
            a.pushInt64(10);
            a.pushInt64(20);
            if (a.length() != 2) { return 0; }
            if (a.getInt64(0) != 10) { return 0; }
            if (a.getInt64(1) != 20) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Shuffle on Large Array Preserves All Elements
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ShuffleLargeArrayPreservesElements) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 50) {
                a.pushInt64(i);
                i = i + 1;
            }
            a.shuffle();
            if (a.length() != 50) { return 0; }
            var sum: int64 = 0;
            i = 0;
            while (i < 50) {
                sum = sum + a.getInt64(i);
                i = i + 1;
            }
            var expected: int64 = 0;
            i = 0;
            while (i < 50) {
                expected = expected + i;
                i = i + 1;
            }
            if (sum != expected) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Binary Search on Double Array via Sort
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, DoubleArraySortThenAccess) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushDouble(5.5);
            a.pushDouble(1.1);
            a.pushDouble(3.3);
            a.pushDouble(2.2);
            a.pushDouble(4.4);
            a.sort();
            var r = 1;
            if (a.getDouble(0) != 1.1) { r = 0; }
            if (a.getDouble(1) != 2.2) { r = 0; }
            if (a.getDouble(2) != 3.3) { r = 0; }
            if (a.getDouble(3) != 4.4) { r = 0; }
            if (a.getDouble(4) != 5.5) { r = 0; }
            return r;
        }
    )", "1");
}

// ============================================================================
// Array of Class Instances with Method Calls
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, ClassInstanceArrayRoundtrip) {
    compileAndRun(R"(
        import hoo;
        class Counter {
            var val: int64;
            constructor(start: int64) {
                this.val = start;
            }
            func :void increment() {
                this.val = this.val + 1;
            }
            func :int64 getVal() {
                return this.val;
            }
        }
        func :int64 main() {
            var a = new Array();
            var c1 = new Counter(10);
            var c2 = new Counter(20);
            a.pushObject(c1);
            a.pushObject(c2);
            c1.increment();
            c1.increment();
            var r1: Counter = a.getObject(0);
            var r2: Counter = a.getObject(1);
            if (r1.getVal() != 12) { return 0; }
            if (r2.getVal() != 20) { return 0; }
            return 1;
        }
    )", "1");
}

// ============================================================================
// Mixed int64 and String Operations
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, MixedInt64AndString) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(100);
            a.pushString("hello");
            a.pushInt64(200);
            a.pushString("world");
            var num1: int64 = a.getInt64(0);
            var s1: string = a.getString(1);
            var num2: int64 = a.getInt64(2);
            var s2: string = a.getString(3);
            return num1 + s1.length() + num2 + s2.length();
        }
    )", "310");
}

// ============================================================================
// Sort Range on Single Element
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, SortRangeSingleElement) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.pushInt64(42);
            a.sortRange(0, 1);
            return a.getInt64(0);
        }
    )", "42");
}

// ============================================================================
// Empty Array Operations Don't Crash
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, EmptyArrayPopSafe) {
    compileAndRun(R"(
        import hoo;
        func :int64 main() {
            var a = new Array();
            a.clear();
            return a.length();
        }
    )", "0");
}

// ============================================================================
// Static Methods Rejected
// ============================================================================

TEST_F(GenericArrayCLIIntegrationTest, StaticArrayMethodsRejected) {
    std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 42);
            return 0;
        }
    )";
    expectCompileFailure(source, "not supported as a static method");
}
