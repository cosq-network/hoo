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

// End-to-end integration tests for overload resolution in Hoo programs
// (compiler overload mangling in src/core/SymbolMangler.cpp + the runtime
// registry in src/runtime/lib/core/hoo_overload.cpp resolved via
// CALL_OVERLOADED). Each test compiles a complete Hoo program to a .ha
// archive and executes it via the hoo CLI. main() returns :int64;
// assertions inside the program return 0 on mismatch and 1 on success,
// so the test asserts that the CLI prints "1".
//
// Covered aspects:
//   - user-defined free-function overloads distinguished by parameter types
//     (int64+int64 versus string+string) both link and dispatch correctly
//   - arity-distinct user overloads in one module
//   - Math.abs / Math.min / Math.max / Math.sign overload-registry dispatch
//     across int64 and double argument types
//   - overload resolution keeps values exact (no cross-variant corruption)
class HooOverloadCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_overload_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_overload_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
        #else
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
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

    void expectReturnOne(const std::string& source) {
        const auto result = compileAndRun(source);
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
    }
};

TEST_F(HooOverloadCliIntegrationTest, UserFunctionOverloadsByParameterType) {
    expectReturnOne(R"(
        func :int64 combine(a: int64, b: int64) { return a + b; }
        func :string combine(a: string, b: string) { return a.concat(b); }
        func :int64 main() {
            var n = combine(10, 20);
            var s: string = combine("hov", "er");
            if (n != 30) { return 0; }
            if (!s.equals("hover")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooOverloadCliIntegrationTest, UserFunctionOverloadsByArity) {
    expectReturnOne(R"(
        func :int64 pick(kind: int64) { return kind; }
        func :int64 pick(kind: int64, extra: int64) { return kind * 10 + extra; }
        func :int64 main() {
            if (pick(3) != 3) { return 0; }
            if (pick(3, 7) != 37) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooOverloadCliIntegrationTest, MethodOverloadsInUserClass) {
    expectReturnOne(R"(
        import hoo;
        class Calc {
            constructor() {}
            func :int64 scale(v: int64) { return v * 2; }
            func :int64 scale(v: int64, factor: int64) { return v * factor; }
        }
        func :int64 main() {
            var calc = new Calc();
            if (calc.scale(21) != 42) { return 0; }
            if (calc.scale(6, 7) != 42) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooOverloadCliIntegrationTest, MathAbsOverloadRegistryDispatch) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var n = Math.abs(-42);
            var d = Math.abs(-3.5);
            if (n != 42) { return 0; }
            if (d != 3.5) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooOverloadCliIntegrationTest, MathMinMaxSignRegistryDispatch) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var lo = Math.min(10, 20);
            var hi = Math.max(10, 20);
            var los = Math.min(2.5, 7.5);
            var his = Math.max(2.5, 7.5);
            var sgn = Math.sign(-7);
            if (lo != 10) { return 0; }
            if (hi != 20) { return 0; }
            if (los != 2.5) { return 0; }
            if (his != 7.5) { return 0; }
            if (sgn != -1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooOverloadCliIntegrationTest, OverloadValuesStayExactUnderChaining) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 combine(a: int64, b: int64) { return a - b; }
        func :string combine(a: string, b: string) { return b.concat(a); }
        func :int64 main() {
            if (combine(50, 8) != 42) { return 0; }
            var s: string = combine("2455", "4");
            if (!s.equals("42455")) { return 0; }
            var m = Math.abs(-7);
            if (m != 7) { return 0; }
            return 1;
        }
    )");
}
