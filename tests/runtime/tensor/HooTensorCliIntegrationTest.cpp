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

// End-to-end integration tests for the hoo Tensor type
// (src/runtime/lib/mem/hoo_tensor.cpp). Each test compiles a complete
// Hoo program to a .ha archive and executes it via the hoo CLI. main()
// returns :int64; assertions inside the program return 0 on mismatch
// and 1 on success, so the test asserts that the CLI prints "1".
//
// Covered aspects:
//   - 1D/2D/3D tensor literals ("[...]t") and flat element indexing
//   - declared tensors (tensor<int64>[n]) are zero-filled
//   - negative element literals
//   - elementwise + and - between tensors
//   - elementwise .* (ELEMENT_MULTIPLY) and ./ (ELEMENT_DIVIDE)
//   - matrix multiplication via "*"
//   - scalar combinations: tensor + scalar, tensor - scalar, scalar - tensor,
//     tensor * scalar
//   - elementwise == and relational < comparisons
//   - double-valued tensor literals
class HooTensorCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_tensor_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_tensor_cli_"
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

TEST_F(HooTensorCliIntegrationTest, Tensor1DLiteralAndFlatIndexing) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var t = [10, 20, 30, 40, 50]t;
            if (t[0] != 10) { return 0; }
            if (t[4] != 50) { return 0; }
            if (t[0] + t[1] + t[2] + t[3] + t[4] != 150) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, Tensor2DLiteralFlatIndexing) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var t = [[1, 2], [3, 4]]t;
            if (t[0] + t[1] + t[2] + t[3] != 10) { return 0; }
            if (t[3] != 4) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, Tensor3DLiteralFlatIndexing) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var t = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]t;
            if (t[0] != 1) { return 0; }
            if (t[7] != 8) { return 0; }
            if (t[0] + t[3] + t[7] != 13) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, NegativeElementsRoundTrip) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var neg = [-1, 0, 1]t;
            if (neg[0] != -1) { return 0; }
            if (neg[1] != 0) { return 0; }
            if (neg[2] != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseAddition) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var c = a + b;
            if (c[0] != 5) { return 0; }
            if (c[1] != 7) { return 0; }
            if (c[2] != 9) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseSubtraction) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [10, 20, 30]t;
            var b = [1, 2, 3]t;
            var c = a - b;
            if (c[0] + c[1] + c[2] != 54) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseMultiply) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [4, 5, 6]t;
            var c = a .* b;
            if (c[0] + c[1] + c[2] != 32) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseDivision) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [4, 8, 12]t;
            var b = [2, 4, 8]t;
            var q = a ./ b;
            if (q[0] != 2) { return 0; }
            if (q[2] != 1) { return 0; }
            if (q[0] + q[1] + q[2] != 5) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, MatrixMultiply2x2) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [[1, 2], [3, 4]]t;
            var b = [[5, 6], [7, 8]]t;
            var c = a * b;
            if (c[0] != 19) { return 0; }
            if (c[3] != 50) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ScalarRightAddition) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 2, 3]t;
            var c = a + 10;
            if (c[0] != 11) { return 0; }
            if (c[2] != 13) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ScalarRightSubtraction) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [10, 20, 30]t;
            var c = a - 5;
            if (c[0] + c[1] + c[2] != 45) { return 0; }
            var s = a - 4;
            if (s[0] != 6) { return 0; }
            if (s[2] != 26) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ScalarLeftSubtraction) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [4, 8, 12]t;
            var left = 100 - a;
            if (left[0] != 96) { return 0; }
            if (left[2] != 88) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ScalarRightMultiplication) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 2, 3]t;
            var c = a * 2;
            if (c[0] != 2) { return 0; }
            if (c[1] + c[2] != 10) { return 0; }
            var big = [4, 8, 12]t;
            var m = big * 2;
            if (m[2] != 24) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseEqualityOperations) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 2, 3]t;
            var b = [1, 2, 3]t;
            var eq = a == b;
            if (eq[0] + eq[1] + eq[2] != 3) { return 0; }
            var c = [1, 5, 3]t;
            var ne = a != c;
            if (ne[0] + ne[1] + ne[2] != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, ElementwiseLessThanOperations) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = [1, 5, 3]t;
            var b = [2, 3, 3]t;
            var lt = a < b;
            if (lt[0] + lt[1] + lt[2] != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, DeclaredTensorIsZeroFilled) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var z: tensor<int64>[4];
            if (z[0] + z[1] + z[2] + z[3] != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooTensorCliIntegrationTest, DoubleValuedTensorLiteral) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var d = [1.5, 2.5, 3.5]t;
            if (d[0] != 1.5) { return 0; }
            if (d[2] != 3.5) { return 0; }
            var s = d[0] + d[1];
            if (s != 4.0) { return 0; }
            return 1;
        }
    )");
}
