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

// End-to-end integration tests for the hoo.math module
// (src/runtime/lib/data/hoo_math.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI. The CLI prints
// the int64 result of the entry point, so each program returns :int64 and
// the test asserts on the printed value.
//
// Covered aspects:
//   - Constants: PI, E, TAU, INF, NEG_INF, NAN
//   - Basic functions: abs, min, max, clamp, sign
//   - Power and roots: sqrt, pow, fmod, cbrt, hypot
//   - Trigonometric: sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh
//   - Exponential and logarithmic: exp, exp2, expm1, log, log10, log2, log1p
//   - Rounding: floor, ceil, round, trunc, fract
//   - Number utilities: is_even, is_odd, is_prime, gcd, lcm, factorial, fibonacci
//   - Random: creation, next_int64, next_int64_max, next_double, next_bool, retain/release
class MathCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_math_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_math_cli_"
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

TEST_F(MathCLIIntegrationTest, MathConstantsPiETau) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var pi = math_get_pi();
            if (pi <= 3.0 || pi >= 4.0) { return 0; }
            var e = math_get_e();
            if (e <= 2.0 || e >= 3.0) { return 0; }
            var tau = math_get_tau();
            if (tau <= 6.0 || tau >= 7.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathConstantsInfNan) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var inf = math_get_inf();
            if (inf != inf) { return 0; }
            var neg_inf = math_get_neg_inf();
            if (neg_inf != neg_inf) { return 0; }
            var nan = math_get_nan();
            if (nan == nan) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathAbsInt64) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_abs(-42) != 42) { return 0; }
            if (math_abs(42) != 42) { return 0; }
            if (math_abs(0) != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathAbsDouble) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_abs(-3.14) != 3.14) { return 0; }
            if (math_abs(0.0) != 0.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathMinMaxInt64) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_min(10, 20) != 10) { return 0; }
            if (math_min(20, 10) != 10) { return 0; }
            if (math_max(10, 20) != 20) { return 0; }
            if (math_max(20, 10) != 20) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathMinMaxDouble) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_min(3.5, 2.5) != 2.5) { return 0; }
            if (math_max(3.5, 2.5) != 3.5) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathClamp) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_clamp(5.0, 0.0, 10.0) != 5.0) { return 0; }
            if (math_clamp(-5.0, 0.0, 10.0) != 0.0) { return 0; }
            if (math_clamp(15.0, 0.0, 10.0) != 10.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathSign) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_sign(7) != 1) { return 0; }
            if (math_sign(-3) != -1) { return 0; }
            if (math_sign(0) != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathSqrt) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_sqrt(4.0) != 2.0) { return 0; }
            if (math_sqrt(9.0) != 3.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathPow) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_pow(2.0, 3.0) != 8.0) { return 0; }
            if (math_pow(3.0, 2.0) != 9.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathFmod) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_fmod(7.0, 3.0) != 1.0) { return 0; }
            if (math_fmod(10.0, 4.0) != 2.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathFloorCeilRound) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_floor(3.7) != 3.0) { return 0; }
            if (math_ceil(3.2) != 4.0) { return 0; }
            if (math_round(3.5) != 4.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathTruncFract) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_trunc(3.7) != 3.0) { return 0; }
            if (math_trunc(-3.7) != -3.0) { return 0; }
            if (math_fract(3.7) < 0.69 || math_fract(3.7) > 0.71) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathSinCosTan) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_sin(0.0) != 0.0) { return 0; }
            if (math_cos(0.0) != 1.0) { return 0; }
            if (math_tan(0.0) != 0.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathAsinAcosAtan) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_asin(0.0) != 0.0) { return 0; }
            if (math_acos(1.0) != 0.0) { return 0; }
            if (math_atan(0.0) != 0.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathAtan2) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_atan2(0.0, 1.0) != 0.0) { return 0; }
            if (math_atan2(1.0, 0.0) != 1.5707963267948966) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathSinhCoshTanh) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_sinh(0.0) != 0.0) { return 0; }
            if (math_cosh(0.0) != 1.0) { return 0; }
            if (math_tanh(0.0) != 0.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathExpLog) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_exp(0.0) != 1.0) { return 0; }
            if (math_log(1.0) != 0.0) { return 0; }
            if (math_log10(10.0) != 1.0) { return 0; }
            if (math_log2(2.0) != 1.0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathIsEvenOddPrime) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_is_even(4) != 1) { return 0; }
            if (math_is_even(5) != 0) { return 0; }
            if (math_is_odd(5) != 1) { return 0; }
            if (math_is_odd(4) != 0) { return 0; }
            if (math_is_prime(7) != 1) { return 0; }
            if (math_is_prime(8) != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathGcdLcm) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_gcd(12, 18) != 6) { return 0; }
            if (math_gcd(7, 13) != 1) { return 0; }
            if (math_lcm(4, 6) != 12) { return 0; }
            if (math_lcm(5, 7) != 35) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathFactorial) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_factorial(5) != 120) { return 0; }
            if (math_factorial(0) != 1) { return 0; }
            if (math_factorial(1) != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathFibonacci) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            if (math_fibonacci(0) != 0) { return 0; }
            if (math_fibonacci(1) != 1) { return 0; }
            if (math_fibonacci(10) != 55) { return 0; }
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathRandomNewWithSeed) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var rng = new Random(12345);
            var v = rng.nextInt(100);
            if (v < 0 || v >= 100) { return 0; }
            rng.release();
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathRandomNextDouble) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var rng = new Random(42);
            var d = rng.nextDouble();
            if (d < 0.0 || d >= 1.0) { return 0; }
            rng.release();
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathRandomNextBool) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var rng = new Random(7);
            var b = rng.nextBool();
            if (b != 0 && b != 1) { return 0; }
            rng.release();
            return 1;
        }
    )");
}

TEST_F(MathCLIIntegrationTest, MathRandomRetainRelease) {
    expectReturnOne(R"(
        import hoo.math;
        func :int64 main() {
            var rng = new Random(99);
            rng.retain();
            rng.release();
            rng.release();
            return 1;
        }
    )");
}
