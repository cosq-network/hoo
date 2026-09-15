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

// End-to-end integration tests for the hoo.io module
// (src/runtime/lib/io/hoo_io.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI, asserting on
// the stdout produced by print/println. The CLI prints the int64 result
// of the entry point, so main() returns :int64 and the expected value is
// also part of the expected output.
//
// Covered aspects:
//   - print writes text to stdout without a trailing newline
//   - println writes text followed by a newline to stdout
//   - consecutive print calls concatenate on one line (no implicit newline)
//   - printing string variables and strings produced via concat
//   - combination of print/println and the entry point return code
class HooIoCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_io_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_io_cli_"
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

    void expectOutputContains(const std::string& source, const std::string& fragment) {
        const auto result = compileAndRun(source);
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find(fragment), std::string::npos) << result.output;
    }
};

TEST_F(HooIoCliIntegrationTest, PrintWritesTextWithoutNewline) {
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            print("io-print-flush");
            return 1;
        }
    )", "io-print-flush");
}

TEST_F(HooIoCliIntegrationTest, PrintlnWritesLineWithNewline) {
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            println("io-println-first");
            println("io-println-second");
            return 1;
        }
    )", "io-println-second");
}

TEST_F(HooIoCliIntegrationTest, ConsecutivePrintsConcatenateOnOneLine) {
    // print() must not inject a newline between calls: "ab" + "cd" + "ef"
    // must arrive as a single sequence in the captured output.
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            print("ab");
            print("cd");
            print("ef");
            return 1;
        }
    )", "abcdef");
}

TEST_F(HooIoCliIntegrationTest, PrintThenPrintlnSequence) {
    // print("ab"); print("cd"); println("ef") produces the full "abcdef"
    // followed by the println newline.
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            print("ab");
            print("cd");
            println("ef");
            return 1;
        }
    )", "abcdef");
}

TEST_F(HooIoCliIntegrationTest, PrintStringVariable) {
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            var s: string = "var-out";
            print(s);
            return 1;
        }
    )", "var-out");
}

TEST_F(HooIoCliIntegrationTest, PrintlnPrintsConcatResult) {
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            var s: string = "x".concat("y").concat("z");
            println(s);
            return 1;
        }
    )", "xyz");
}

TEST_F(HooIoCliIntegrationTest, InterleavedPrintPrintlnOrdering) {
    expectOutputContains(R"(
        import hoo;
        func :int64 main() {
            print("A1-");
            println("A2");
            print("B1-");
            println("B2");
            return 1;
        }
    )", "A2\nB1-B2");
}
