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

// End-to-end integration tests for the hoo.process module
// (src/runtime/lib/system/hoo_process.cpp). Each test compiles a complete
// Hoo program to a .ha archive and executes it via the hoo CLI. main()
// returns :int64; assertions inside the program return 0 on mismatch
// and 1 on success, so the test asserts that the CLI prints "1".
//
// Covered aspects (Windows cmd.exe semantics; POSIX equivalents are
// generated via the test-side #ifdef lambdas where shells differ):
//   - process_capture returns the child's stdout text
//   - process_capture returns an empty string for no output
//   - process_spawn + process_wait reports the child's exit code
//   - process_wait reports success exit (0) for zero-exit children
//   - process_self_pid is positive and stable across calls
//   - process_kill with signal 0 confirms a running child exists
//   - process_kill with a terminating signal actually ends the child
class HooProcessCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_process_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_process_cli_"
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

    // Platform-specific shell fragments for the child programs.
    std::string echoToken() {
        #ifdef _WIN32
        return "cmd /c echo hoo-proc-ok";
        #else
        return "sh -c echo hoo-proc-ok";
        #endif
    }
    std::string spawnerToken() {
        #ifdef _WIN32
        return "cmd";
        #else
        return "sh";
        #endif
    }
};

TEST_F(HooProcessCliIntegrationTest, CaptureReturnsChildStdout) {
    expectReturnOne("import hoo.process;\n"
                    "func :int64 main() {\n"
                    "    var out = process_capture(\"" + echoToken() + "\");\n"
                    "    if (!out.contains(\"hoo-proc-ok\")) { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooProcessCliIntegrationTest, CaptureEmptyOutputIsEmptyString) {
    expectReturnOne(R"(
        import hoo.process;
        func :int64 main() {
            var out = process_capture("cmd /c exit 0");
            if (out.length() != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooProcessCliIntegrationTest, SpawnAndWaitReportsExitCode) {
    #ifdef _WIN32
    const std::string child = "args.pushString(\"/c\");\n    args.pushString(\"exit 7\")";
    #else
    const std::string child = "args.pushString(\"-c\");\n    args.pushString(\"exit 7\")";
    #endif
    expectReturnOne("import hoo.process;\nimport hoo;\n"
                    "func :int64 main() {\n"
                    "    var args = new Array();\n"
                    "    " + child + ";\n"
                    "    var pid = process_spawn(\"" + spawnerToken() + "\", args);\n"
                    "    if (pid <= 0) { return 0; }\n"
                    "    var code = process_wait(pid);\n"
                    "    if (code != 7) { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooProcessCliIntegrationTest, SpawnAndWaitZeroExitIsZero) {
    #ifdef _WIN32
    const std::string child = "args.pushString(\"/c\");\n    args.pushString(\"exit 0\")";
    #else
    const std::string child = "args.pushString(\"-c\");\n    args.pushString(\"exit 0\")";
    #endif
    expectReturnOne("import hoo.process;\nimport hoo;\n"
                    "func :int64 main() {\n"
                    "    var args = new Array();\n"
                    "    " + child + ";\n"
                    "    var pid = process_spawn(\"" + spawnerToken() + "\", args);\n"
                    "    var code = process_wait(pid);\n"
                    "    if (code != 0) { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooProcessCliIntegrationTest, SelfPidIsPositiveAndStable) {
    expectReturnOne(R"(
        import hoo.process;
        func :int64 main() {
            var me = process_self_pid();
            if (me <= 0) { return 0; }
            var me2 = process_self_pid();
            if (me != me2) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooProcessCliIntegrationTest, KillSignalZeroConfirmsRunningChild) {
    std::string sleeper = "";
    #ifdef _WIN32
    sleeper = "args.pushString(\"/c\");\n    args.pushString(\"ping -n 5 127.0.0.1\")";
    #else
    sleeper = "args.pushString(\"-c\");\n    args.pushString(\"sleep 5\")";
    #endif
    expectReturnOne("import hoo.process;\nimport hoo;\n"
                    "func :int64 main() {\n"
                    "    var args = new Array();\n"
                    "    " + sleeper + ";\n"
                    "    var pid = process_spawn(\"" + spawnerToken() + "\", args);\n"
                    "    var exists = process_kill(pid, 0);\n"
                    "    if (exists != 0) { return 0; }\n"
                    "    var gone = process_kill(pid, 9);\n"
                    "    if (gone != 0) { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooProcessCliIntegrationTest, KillTerminatesChildProcess) {
    std::string sleeper = "";
    #ifdef _WIN32
    sleeper = "args.pushString(\"/c\");\n    args.pushString(\"ping -n 8 127.0.0.1\")";
    #else
    sleeper = "args.pushString(\"-c\");\n    args.pushString(\"sleep 8\")";
    #endif
    expectReturnOne("import hoo.process;\nimport hoo;\n"
                    "func :int64 main() {\n"
                    "    var args = new Array();\n"
                    "    " + sleeper + ";\n"
                    "    var pid = process_spawn(\"" + spawnerToken() + "\", args);\n"
                    "    var killed = process_kill(pid, 9);\n"
                    "    if (killed != 0) { return 0; }\n"
                    "    var code = process_wait(pid);\n"
                    "    if (code == 0) { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooProcessCliIntegrationTest, WaitOnInvalidPidFailsCleanly) {
    // A pid that cannot exist must fail process_wait without crashing.
    expectReturnOne(R"(
        import hoo.process;
        func :int64 main() {
            var code = process_wait(2147483647);
            if (code != -1) { return 0; }
            return 1;
        }
    )");
}
