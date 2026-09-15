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

// End-to-end integration tests for the hoo.ai module
// (src/runtime/lib/core/hoo_ai.{h,cpp} surfaced as free functions).
// Each test compiles a complete Hoo program to a .ha archive and
// executes it via the hoo CLI. main() returns :int64; assertions
// inside the program return 0 on mismatch and 1 on success, so the
// test asserts that the CLI prints "1".
//
// Covered aspects:
//   - ai_abi_version reports the v2 tensor ABI version
//   - ai_last_status starts clean and stays clean on query-only paths
//   - ai_has_feature reports implemented and feature-negative names
//     correctly (including unknown names returning 0)
//   - ai_dtype_supported mirrors the dtype capability table
class HooAiCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_ai_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_ai_cli_"
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

    void expectReturnOne(const std::string& source) {
        const auto result = compileAndRun(source);
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
    }

    ExecResult compileAndRun(const std::string& source) {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = createArchive();
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\"");
    }
};

TEST_F(HooAiCliIntegrationTest, AbiVersionIsTwo) {
    expectReturnOne(R"(
        import hoo.ai;
        func :int64 main() {
            var abi: int64 = ai_abi_version();
            if (abi != 2) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooAiCliIntegrationTest, LastStatusStartsClean) {
    expectReturnOne(R"(
        import hoo.ai;
        func :int64 main() {
            var status: int64 = ai_last_status();
            if (status != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooAiCliIntegrationTest, FeatureStringsReportTruth) {
    expectReturnOne(R"(
        import hoo.ai;
        func :int64 main() {
            if (ai_has_feature("tensor_abi_v2") != 1) { return 0; }
            if (ai_has_feature("tensor_dynamic_rank") != 1) { return 0; }
            if (ai_has_feature("tensor_f32") != 1) { return 0; }
            if (ai_has_feature("tensor_int32") != 1) { return 0; }
            if (ai_has_feature("tensor_f16") != 0) { return 0; }
            if (ai_has_feature("tensor_bf16") != 0) { return 0; }
            if (ai_has_feature("tensor_nonexistent") != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooAiCliIntegrationTest, DtypeSupportTableMatchesRuntime) {
    expectReturnOne(R"(
        import hoo.ai;
        func :int64 main() {
            var dt_bit: int64 = ai_dtype_supported(8);
            var dt_f64: int64 = ai_dtype_supported(2);
            var dt_f32: int64 = ai_dtype_supported(16);
            var dt_i32: int64 = ai_dtype_supported(19);
            var dt_f16: int64 = ai_dtype_supported(17);
            var dt_bf16: int64 = ai_dtype_supported(18);
            var dt_junk: int64 = ai_dtype_supported(999);
            if (dt_bit + 1 != 2) { return 0; }
            if (dt_f32 + dt_i32 != 2) { return 0; }
            if (dt_f16 != 0) { return 0; }
            if (dt_bf16 != 0) { return 0; }
            if (dt_junk != 0) { return 0; }
            if (dt_f64 != 1) { return 0; }
            return 1;
        }
    )");
}
