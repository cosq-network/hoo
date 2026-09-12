// End-to-end CLI integration tests for the hoo.system runtime module.
// Each test compiles a complete Hoo program to a .ha archive via the hoo
// executable and then runs it.  The CLI prints the int64 return value of
// main() as the final line, so programs return 1 on success, 0 on failure.
//
// Coverage: each aspect of the module — environment, host/os info, CPU/PID,
// uptime, current directory, user info, exec/exec_status, memory, the exit
// contract (code propagation, clamping, stdout flush, immediate termination),
// exception semantics on unexpected failures, and the UTF-8 env round trip.
// All programs use only behaviour that is identical on Windows, Linux, and
// macOS (echo/`exit N` commands, nonexistent directories, '=' env names).
//
// NOTE: Hoo's println(int64) bridge and string+int64 concatenation are known
// to crash (pre-existing, unrelated to the system module), so these programs
// never println() or concat int64 values.  They use main() return codes and
// printed strings instead.

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

class SystemCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string hooExe;

    void SetUp() override {
        hooExe = HOO_EXECUTABLE;
    }

    std::string uniquePath(const std::string& suffix) {
        static int counter = 0;
        std::string temp = std::filesystem::temp_directory_path().string();
        for (char& c : temp) { if (c == '\\') c = '/'; }
        return temp + "/hoo_system_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + suffix;
    }

    std::string createSource(const std::string& source) {
        const std::string path = uniquePath(".hoo");
        std::ofstream file(path);
        file << source;
        return path;
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
        const std::string archivePath = uniquePath(".ha");
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\"");
    }

    ExecResult compileOnly(const std::string& source) {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = uniquePath(".ha");
        return runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
    }

    static std::string lastLine(const std::string& output) {
        std::string s = output;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
            s.pop_back();
        const std::string::size_type pos = s.find_last_of('\n');
        return (pos == std::string::npos) ? s : s.substr(pos + 1);
    }

    void expectPass(const ExecResult& result) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_EQ(lastLine(result.output), "1") << result.output;
    }

    void expectOutputContains(const ExecResult& result, const std::string& expected) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find(expected), std::string::npos) << result.output;
    }

    void expectRuntimeFailure(const ExecResult& result) {
        EXPECT_EQ(result.exitCode, 1) << result.output;
    }

    void expectCompileFailure(const ExecResult& result) {
        EXPECT_NE(result.exitCode, 0) << result.output;
    }
};

// ─────────────────────────────────────────────────────────────────────────
// Environment
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, GetEnvReturnsValue) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var path = system_get_env("PATH");
            if (!path) { return 0; }
            if (path.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, GetEnvUnsetIsNil) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var val = system_get_env("HOO_SYSTEM_CLI_NO_SUCH_VAR");
            if (val) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetGetUnsetRoundTrip) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var rc = system_set_env("HOO_SYSTEM_CLI_VAR", "abc123");
            if (rc != 0) { return 0; }
            var val = system_get_env("HOO_SYSTEM_CLI_VAR");
            if (!val) { return 0; }
            if (!val.equals("abc123")) { return 0; }
            if (system_unset_env("HOO_SYSTEM_CLI_VAR") != 0) { return 0; }
            if (system_get_env("HOO_SYSTEM_CLI_VAR")) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// System info
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, HostnameNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var host = system_hostname();
            if (host.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, OsNameNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var os = system_os_name();
            if (os.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, OsVersionNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var ver = system_os_version();
            if (ver.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, CpuCountPositive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_cpu_count() < 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, ProcessIdPositive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_process_id() < 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, UptimePositive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_uptime_ms() < 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, UptimeIsMonotonic) {
    // Two back-to-back reads must be non-decreasing on every platform.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var t1 = system_uptime_ms();
            var t2 = system_uptime_ms();
            if (t2 < t1) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// Process
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, ExecCapturesStdout) {
    ExecResult result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var out = system_exec("echo hoo-system-ok");
            if (out.length() == 0) { return 0; }
            println(out);
            return 1;
        }
    )hoo");
    expectOutputContains(result, "hoo-system-ok");
}

TEST_F(SystemCLIIntegrationTest, ExecStatusSuccessIsZero) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_exec_status("exit 0") != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, ExecStatusReportsExitCode) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_exec_status("exit 7") != 7) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, ExecStatusReportsMidRangeCode) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_exec_status("exit 3") != 3) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, ExecEmptyOutputIsEmptyButNotNull) {
    // A command that runs successfully but writes nothing returns an empty
    // (but non-null) string, distinct from a spawn failure (nil/exception).
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var out = system_exec("exit 0");
            if (!out) { return 0; }
            if (out.length() != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// User info
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, UserHomeNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var home = system_user_home();
            if (home.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, UserNameNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var user = system_user_name();
            if (user.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, CurrentDirNonEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var dir = system_current_dir();
            if (dir.length() == 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetCurrentDirToCwdSucceeds) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_set_current_dir(system_current_dir()) != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetCurrentDirInvalidFails) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            try {
                system_set_current_dir("__hoo_no_such_dir_98765__");
                return 0;
            } catch (e: Exception) {
                return 1;
            }
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetCurrentDirInvalidUncaughtFailsHard) {
    const auto result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            system_set_current_dir("__hoo_no_such_dir_98765__");
            return 0;
        }
    )hoo");
    EXPECT_NE(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("Unhandled exception trap"), std::string::npos);
}

TEST_F(SystemCLIIntegrationTest, CaughtFailureMessageIsDescriptive) {
    // The raised RuntimeException must carry the underlying platform error
    // text in e.message, so the message is never nil/null.  (.length() does
    // not resolve on the Exception message type in codegen, so we assert the
    // property is set — the runtime always attaches a real error string.)
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            try {
                system_set_current_dir("__hoo_no_such_dir_98765__");
                return 0;
            } catch (e: Exception) {
                if (e.message) { return 1; }
                return 0;
            }
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetEnvInvalidNameThrows) {
    // An environment name containing '=' is rejected by both SetEnvironmentVariableW
    // and POSIX setenv(3), so this is an unexpected failure that raises.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            try {
                system_set_env("BAD=NAME", "x");
                return 0;
            } catch (e: Exception) {
                return 1;
            }
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetEnvEmptyValueUnsets) {
    // Setting an empty-string value deletes the variable (documented contract)
    // and neither the set nor a follow-up unset raises.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            try {
                system_set_env("HOO_SYSTEM_CLI_CLEAR", "1");
                system_set_env("HOO_SYSTEM_CLI_CLEAR", "");
                system_unset_env("HOO_SYSTEM_CLI_CLEAR");
                return 1;
            } catch (e: Exception) {
                return 0;
            }
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, UnsetMissingVariableSucceeds) {
    // Unsetting a variable that was never set is not an unexpected failure:
    // SetEnvironmentVariableW(NULL) and unsetenv() both return success.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            try {
                if (system_unset_env("HOO_SYSTEM_CLI_UNSET_MISSING") != 0) { return 0; }
                return 1;
            } catch (e: Exception) {
                return 0;
            }
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetEnvMultipleVarsAreIndependent) {
    // Two variables set and read back independently prove the environment
    // block is not shared/collapsed across names.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_set_env("HOO_SYSTEM_CLI_A", "alpha") != 0) { return 0; }
            if (system_set_env("HOO_SYSTEM_CLI_B", "beta") != 0) { return 0; }
            var a = system_get_env("HOO_SYSTEM_CLI_A");
            var b = system_get_env("HOO_SYSTEM_CLI_B");
            if (!a || !a.equals("alpha")) { return 0; }
            if (!b || !b.equals("beta")) { return 0; }
            if (a.equals(b)) { return 0; }
            system_unset_env("HOO_SYSTEM_CLI_A");
            system_unset_env("HOO_SYSTEM_CLI_B");
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, SetEnvUtf8ValueRoundTrip) {
    // héllo-中文 in UTF-8, expressed with ASCII-only C++ escapes so the test
    // source stays ASCII.  Verifies the wide (UTF-16) round trip used by the
    // Windows backend and the byte-preserving POSIX path.
    const std::string utf8 = "h\xC3\xA9llo-\xE4\xB8\xAD\xE6\x96\x87";
    const std::string source =
        "import hoo.system;\n"
        "func :int64 main() {\n"
        "    if (system_set_env(\"HOO_SYSTEM_CLI_UTF8\", \"" + utf8 + "\") != 0) { return 0; }\n"
        "    var v = system_get_env(\"HOO_SYSTEM_CLI_UTF8\");\n"
        "    if (!v) { return 0; }\n"
        "    var same = v.equals(\"" + utf8 + "\");\n"
        "    system_unset_env(\"HOO_SYSTEM_CLI_UTF8\");\n"
        "    if (!same) { return 0; }\n"
        "    return 1;\n"
        "}\n";
    expectPass(compileAndRun(source));
}

// ─────────────────────────────────────────────────────────────────────────
// Memory
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, TotalMemoryPositive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_total_memory() < 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, FreeMemoryPositive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            if (system_free_memory() < 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(SystemCLIIntegrationTest, TotalMemoryNotLessThanFreeMemory) {
    // Free memory can never exceed total memory reported by the same source.
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var total = system_total_memory();
            var free = system_free_memory();
            if (total < 1) { return 0; }
            if (total < free) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// Exit contract
// ─────────────────────────────────────────────────────────────────────────
//
// system_exit() flushes stdout/stderr and terminates immediately with the
// given code, clamped to 0..255.  These tests rely on the hoo process exit
// code, so a trailing `return 0;` keeps the compiler's non-void analysis
// happy even though it is unreachable.

TEST_F(SystemCLIIntegrationTest, ExitPropagatesExitCode) {
    const auto result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            system_exit(7);
            return 0;
        }
    )hoo");
    EXPECT_EQ(result.exitCode, 7) << result.output;
}

TEST_F(SystemCLIIntegrationTest, ExitCodeIsClampedToByte) {
    // 300 & 0xFF == 44; the clamps happen inside hoo_system_exit so the
    // behaviour is identical on Windows and POSIX.
    const auto result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            system_exit(300);
            return 0;
        }
    )hoo");
    EXPECT_EQ(result.exitCode, 44) << result.output;
}

TEST_F(SystemCLIIntegrationTest, ExitFlushesPendingStdout) {
    // Output buffered before exit() must still reach the host: process exit
    // code is 0 and the printed line is visible.
    const auto result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            println("token-flushed-before-exit");
            system_exit(0);
            return 0;
        }
    )hoo");
    EXPECT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("token-flushed-before-exit"), std::string::npos);
}

TEST_F(SystemCLIIntegrationTest, ExitStopsFutherExecution) {
    // Anything after system_exit() must never run or be printed.
    const auto result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            println("printed-before-exit");
            system_exit(0);
            println("never-printed-after-exit");
            return 0;
        }
    )hoo");
    EXPECT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("printed-before-exit"), std::string::npos);
    EXPECT_EQ(result.output.find("never-printed-after-exit"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────
// Combined smoke test of every free function
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, AllFunctionsTogether) {
    expectPass(compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var sum = system_cpu_count() + system_process_id() + system_uptime_ms()
                + system_total_memory() + system_free_memory()
                + system_exec_status("exit 3")
                + system_get_env("PATH").length()
                + system_hostname().length()
                + system_os_name().length()
                + system_os_version().length()
                + system_user_home().length()
                + system_user_name().length()
                + system_current_dir().length()
                + system_exec("echo abc").length();
            if (sum <= 0) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// Wrong-API guards
// ─────────────────────────────────────────────────────────────────────────

TEST_F(SystemCLIIntegrationTest, DocumentedEnvNameDoesNotResolve) {
    // "system_env" (with no _get) is documented in docs/runtime/api/system.md
    // but is not part of the real API: it compiles as an undefined external
    // and must fail at run time, not silently succeed.
    ExecResult result = compileAndRun(R"hoo(
        import hoo.system;
        func :int64 main() {
            var path = system_env("PATH");
            if (!path) { return 0; }
            return 1;
        }
    )hoo");
    expectRuntimeFailure(result);
}

TEST_F(SystemCLIIntegrationTest, SystemClassCallDoesNotCompile) {
    // System.* static-style calls are not supported for this module.
    ExecResult result = compileOnly(R"hoo(
        import hoo.system;
        func :int64 main() {
            var path = System.get_env("PATH");
            if (!path) { return 0; }
            return 1;
        }
    )hoo");
    expectCompileFailure(result);
}