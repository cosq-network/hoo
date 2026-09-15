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

// End-to-end integration tests for the global libuv event loop exposed
// through hoo.concurrency (src/runtime/lib/concurrency/hoo_event_loop.cpp
// surfaced as free functions). Each test compiles a complete Hoo program
// to a .ha archive and executes it via the hoo CLI. main() returns
// :int64; there are no active handles in these programs, so run() and
// run_nowait() both return immediately and every test asserts that the
// CLI prints "1".
//
// Covered aspects:
//   - init / run_nowait / run / destroy complete a full cycle safely
//   - repeated init and run_nowait calls are idempotent-safe
//   - run on a loop with no active handles returns promptly
//   - destroy tears the loop down so a later init re-initializes
//   - run after destroy (NULL loop) is a safe no-op
class HooEventLoopCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_event_loop_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_event_loop_cli_"
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

TEST_F(HooEventLoopCliIntegrationTest, InitRunNowaitDestroyCycle) {
    expectReturnOne(R"(
        import hoo.concurrency;
        func :int64 main() {
            event_loop_init();
            event_loop_run_nowait();
            event_loop_destroy();
            return 1;
        }
    )");
}

TEST_F(HooEventLoopCliIntegrationTest, RepeatedInitAndRunNowaitAreSafe) {
    expectReturnOne(R"(
        import hoo.concurrency;
        func :int64 main() {
            event_loop_init();
            event_loop_init();
            event_loop_run_nowait();
            event_loop_run_nowait();
            event_loop_destroy();
            return 1;
        }
    )");
}

TEST_F(HooEventLoopCliIntegrationTest, RunWithNoHandlesReturnsPromptly) {
    expectReturnOne(R"(
        import hoo.concurrency;
        func :int64 main() {
            event_loop_init();
            event_loop_run();
            event_loop_destroy();
            return 1;
        }
    )");
}

TEST_F(HooEventLoopCliIntegrationTest, RunAfterDestroyIsSafeNoOp) {
    expectReturnOne(R"(
        import hoo.concurrency;
        func :int64 main() {
            event_loop_init();
            event_loop_destroy();
            event_loop_run();
            event_loop_run_nowait();
            return 1;
        }
    )");
}

TEST_F(HooEventLoopCliIntegrationTest, DestroyThenReinitFreshLoop) {
    expectReturnOne(R"(
        import hoo.concurrency;
        func :int64 main() {
            event_loop_init();
            event_loop_run_nowait();
            event_loop_destroy();
            event_loop_init();
            event_loop_run_nowait();
            event_loop_destroy();
            return 1;
        }
    )");
}
