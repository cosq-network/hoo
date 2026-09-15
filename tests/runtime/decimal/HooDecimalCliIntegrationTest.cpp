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

// End-to-end integration tests for the hoo Decimal (fixed-point) type
// (src/runtime/lib/data/hoo_decimal.cpp). Each test compiles a complete
// Hoo program to a .ha archive and executes it via the hoo CLI. main()
// returns :int64; assertions inside the program return 0 on mismatch
// and 1 on success, so the test asserts that the CLI prints "1".
//
// Covered aspects:
//   - decimal literal assignment (m suffix) and exact equality
//   - exact fixed-point addition / subtraction
//   - exact multiplication
//   - division of aligned values
//   - modulo of aligned values
//   - unary negation
//   - relational comparisons (<, >, <=, >=)
//   - scale widening (Decimal<38,2> value into Decimal<38,4>)
//   - passing decimals through function parameters and returns
class HooDecimalCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_decimal_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_decimal_cli_"
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

TEST_F(HooDecimalCliIntegrationTest, LiteralAssignmentAndEquality) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 19.99m;
            if (a != b) { return 0; }
            if (a != 19.99m) { return 0; }
            if (a == 19.98m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ExactAddition) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 8.00m;
            var s = a + b;
            if (s != 27.99m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ExactSubtraction) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var s = a - 9.99m;
            if (s != 10.00m) { return 0; }
            var t = 5.25m - 5.25m;
            if (t != 0.00m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ExactMultiplication) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var m = a * 2.00m;
            if (m != 39.98m) { return 0; }
            if (3.00m * 4.00m != 12.00m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ExactDivision) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var b: Decimal<38,2> = 8.00m;
            var q = b / 2.00m;
            if (q != 4.00m) { return 0; }
            if (10.00m / 4.00m != 2.50m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ExactModulo) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            if (19.97m % 2.00m != 1.97m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, UnaryNegation) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var n = -a;
            if (n != -19.99m) { return 0; }
            if (-n != a) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, StrictInequalityComparisons) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var zero: Decimal<38,2> = 0.00m;
            if (!(a > 10.00m)) { return 0; }
            if (!(a < 100.00m)) { return 0; }
            if (!(zero > -1.00m)) { return 0; }
            if (!(zero < 1.00m)) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, InclusiveInequalityComparisons) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var b: Decimal<38,2> = 8.00m;
            if (!(b <= 8.01m)) { return 0; }
            if (!(b <= 8.00m)) { return 0; }
            if (b >= 8.01m) { return 0; }
            if (!(b >= 8.00m)) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, ScaleWideningKeepsValue) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :int64 main() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 8.00m;
            var c: Decimal<38,4> = a + b;
            if (c != 27.9900m) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDecimalCliIntegrationTest, FunctionParameterAndReturn) {
    expectReturnOne(R"(
        import hoo.decimal;
        func :Decimal<38,2> add(a: Decimal<38,2>, b: Decimal<38,2>) { return a + b; }
        func :Decimal<38,2> mul(a: Decimal<38,2>, b: Decimal<38,2>) { return a * b; }
        func :int64 main() {
            var a: Decimal<38,2> = add(19.99m, 8.01m);
            var b: Decimal<38,2> = mul(3.00m, 4.00m);
            var r: int64 = 0;
            if (a == 28.00m) { r = r + 1; }
            if (b == 12.00m) { r = r + 10; }
            if (r != 11) { return 0; }
            return 1;
        }
    )");
}
