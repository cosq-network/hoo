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

// End-to-end integration tests for the hoo.path module
// (src/runtime/lib/system/hoo_fs.cpp hoo_path_* C-ABI functions).
// Each test compiles a complete Hoo program to a .ha archive and executes
// it via the hoo CLI. main() returns :int64; assertions inside the
// program return 0 on mismatch and 1 on success, so the CLI output
// contains the return value which the test asserts on.
//
// Covered aspects:
//   - path_join combines components with the platform separator
//   - path_basename / path_filename return the final component
//   - path_extension returns a dotted extension ("csv")
//   - path_stem strips the extension
//   - path_dirname / path_parent return the directory part
//   - path_has_extension int64 flag semantics
//   - path_is_relative / path_is_absolute detection
//   - path_normalize removes "." and ".." components
//   - path_separator returns the platform directory separator character
class HooPathCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_path_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_path_cli_"
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

TEST_F(HooPathCliIntegrationTest, JoinCombinesComponents) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var joined = path_join("dir", "file.txt");
            if (!joined.contains("dir")) { return 0; }
            if (!joined.contains("file.txt")) { return 0; }
            if (joined.length() <= ("file.txt".length())) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, BasenameReturnsFinalComponent) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var base = path_basename("C:/Users/hoo/report.txt");
            if (!base.equals("report.txt")) { return 0; }
            var base2 = path_basename("logs/app.log");
            if (!base2.equals("app.log")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, FilenameMatchesBasename) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var f = path_filename("C:/Users/hoo/data.json");
            if (!f.equals("data.json")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, ExtensionIncludesLeadingDot) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var ext = path_extension("C:/dir/data.report.csv");
            if (!ext.equals(".csv")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, StemStripsExtension) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var stem = path_stem("C:/dir/report.txt");
            if (!stem.equals("report")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, DirnameReturnsDirectoryPart) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var dir = path_dirname("C:/Users/hoo/file.txt");
            if (!dir.contains("Users")) { return 0; }
            if (!dir.contains("hoo")) { return 0; }
            if (dir.contains("file.txt")) { return 0; }
            var parent = path_parent("C:/a/b/c.txt");
            if (!parent.contains("a")) { return 0; }
            if (!parent.contains("b")) { return 0; }
            if (parent.contains("c.txt")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, HasExtensionFlagSemantics) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            if (path_has_extension("C:/dir/x.txt") != 1) { return 0; }
            if (path_has_extension("C:/dir/noext") != 0) { return 0; }
            if (path_has_extension("readme") != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, IsRelativeDetection) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            if (path_is_relative("relative/dir") != 1) { return 0; }
            if (path_is_relative("file.hoo") != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, IsAbsoluteDetection) {
    std::string runtime_expectations;
    #ifdef _WIN32
    runtime_expectations =
        "if (path_is_absolute(\"C:/Users/hoo\") != 1) { return 0; }";
    #else
    runtime_expectations =
        "if (path_is_absolute(\"/usr/local\") != 1) { return 0; }";
    #endif
    expectReturnOne("import hoo.path;\nfunc :int64 main() {\n"
                    + runtime_expectations + "\n"
                    "    return 1;\n}\n");
}

TEST_F(HooPathCliIntegrationTest, NormalizeRemovesDotAndDotDot) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var n = path_normalize("./dir/sub/../leaf.txt");
            if (!n.contains("leaf.txt")) { return 0; }
            if (!n.contains("dir")) { return 0; }
            if (n.contains("..")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooPathCliIntegrationTest, SeparatorReturnsPlatformChar) {
    std::string sepCheck;
    #ifdef _WIN32
    sepCheck = "92";  // '\\'
    #else
    sepCheck = "47";  // '/'
    #endif
    expectReturnOne("import hoo.path;\n"
                    "func :int64 main() {\n"
                    "    var sep = path_separator();\n"
                    "    if (sep != " + sepCheck + ") { return 0; }\n"
                    "    return 1;\n"
                    "}\n");
}

TEST_F(HooPathCliIntegrationTest, AbsoluteContainsWorkingDirComponent) {
    expectReturnOne(R"(
        import hoo.path;
        func :int64 main() {
            var abs = path_absolute("some/rel/file.hoo");
            if (abs.length() <= "file.hoo".length()) { return 0; }
            if (!abs.contains("file.hoo")) { return 0; }
            return 1;
        }
    )");
}
