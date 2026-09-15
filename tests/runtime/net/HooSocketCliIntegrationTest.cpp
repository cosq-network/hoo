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

// End-to-end integration tests for the hoo.net Socket class
// (src/runtime/lib/io/hoo_net.cpp hoo_net_socket_* functions). Each test
// compiles a complete Hoo program to a .ha archive and executes it via
// the hoo CLI. main() returns :int64; assertions inside the program
// return 0 on mismatch and 1 on success, so the test asserts that the
// CLI prints "1".
//
// All tests are offline-safe: connect targets 127.0.0.1:9 (the discard
// port) which locally and immediately refuses; server-side aspects use
// the loopback interface only. Covered aspects:
//   - new Socket() constructs and release() frees without crashing
//   - connect to a refused endpoint fails with a non-zero status
//   - lastError() reports a non-empty message after the failure
//   - setTimeout(500) succeeds on a fresh socket
//   - bind on 127.0.0.1 with port 0 picks an ephemeral port
//   - listen succeeds after bind
//   - localPort returns the bound port (1..65535) after listen
//   - close() returns success for a socket that never connected
//   - receive() on a failed socket does not crash the runtime
class HooSocketCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_socket_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_socket_cli_"
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

TEST_F(HooSocketCliIntegrationTest, ConnectToRefusedEndpointFails) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var refused: int64 = s.connect("127.0.0.1", 9);
            s.release();
            if (refused == 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, LastErrorReportsRefusedConnection) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var refused: int64 = s.connect("127.0.0.1", 9);
            var msg: string = s.lastError();
            s.release();
            if (refused == 0) { return 0; }
            if (msg.length() == 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, SetTimeoutSucceedsOnFreshSocket) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var a: int64 = s.setTimeout(500);
            s.release();
            if (a != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, BindListenAcquiresEphemeralLoopbackPort) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var b: int64 = s.bind("127.0.0.1", 0);
            if (b != 0) { return 0; }
            var l: int64 = s.listen(5);
            if (l != 0) { return 0; }
            var lp: int64 = s.localPort();
            if (lp <= 0 || lp > 65535) { return 0; }
            var c: int64 = s.close();
            if (c != 0) { return 0; }
            s.release();
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, CloseOnNeverConnectedSocketSucceeds) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var c: int64 = s.close();
            s.release();
            if (c != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, ReceiveAfterRefusedConnectDoesNotCrash) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            var refused: int64 = s.connect("127.0.0.1", 9);
            var ignore = s.receive(64);
            s.release();
            if (refused == 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooSocketCliIntegrationTest, ReleaseFreesHandleWithoutCrash) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var s = new Socket();
            s.release();
            return 1;
        }
    )");
}
