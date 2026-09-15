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

// End-to-end integration tests for the hoo string module
// (src/runtime/lib/text/hoo_string.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI. The CLI prints
// the int64 result of the entry point, so each program returns :int64 and
// the test asserts on the printed value.
//
// Covered aspects:
//   - Creation: string literals, empty string
//   - Query: length, isEmpty, indexOf, lastIndexOf, contains, startsWith, endsWith, byteAt
//   - Manipulation: concat, toUpper, toLower, trim, replace, substring, split
//   - Comparison: equals, compare
//   - Conversion: toInt64
//   - Reference counting: refcount, retain, release
class StringCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_string_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_string_cli_"
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

TEST_F(StringCLIIntegrationTest, StringLiteralLength) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("hello".length() != 5) { return 0; }
            if ("".length() != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringConcat) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var a = "Hello, ";
            var b = "World";
            var c = a.concat(b);
            if (!c.equals("Hello, World")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringToUpper) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var upper = "hello".toUpper();
            if (!upper.equals("HELLO")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringToLower) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var lower = "HELLO".toLower();
            if (!lower.equals("hello")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringTrim) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var trimmed = "  hello  ".trim();
            if (!trimmed.equals("hello")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringReplace) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var replaced = "one two three".replace("two", "TWO");
            if (!replaced.equals("one TWO three")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringSplitLength) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var parts = "a,b,c".split(",");
            if (parts.length() != 3) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringIndexOf) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("hello world".indexOf("world") != 6) { return 0; }
            if ("hello world".indexOf("xyz") != -1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringLastIndexOf) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("hello world hello".lastIndexOf("hello") != 12) { return 0; }
            if ("hello world hello".lastIndexOf("xyz") != -1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringContains) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("hello world".contains("world") != 1) { return 0; }
            if ("hello world".contains("xyz") != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringStartsWithEndsWith) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("hello".startsWith("he") != 1) { return 0; }
            if ("hello".startsWith("xyz") != 0) { return 0; }
            if ("hello".endsWith("lo") != 1) { return 0; }
            if ("hello".endsWith("xyz") != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringEquals) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("abc".equals("abc") != 1) { return 0; }
            if ("abc".equals("def") != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringIsEmpty) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("".isEmpty() != 1) { return 0; }
            if ("x".isEmpty() != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringSubstring) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var sub = "hello".substring(0, 3);
            if (!sub.equals("hel")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringToInt64) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("42".toInt64() != 42) { return 0; }
            if ("-7".toInt64() != -7) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringCompare) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("abc".compare("abc") != 0) { return 0; }
            if ("abc".compare("abd") != -1) { return 0; }
            if ("abd".compare("abc") != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringByteAt) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            if ("ABC".byteAt(0) != 65) { return 0; }
            if ("ABC".byteAt(2) != 67) { return 0; }
            if ("ABC".byteAt(10) != -1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(StringCLIIntegrationTest, StringRefcount) {
    expectReturnOne(R"(
        import hoo;
        func :int64 main() {
            var s = "hello";
            s.retain();
            s.release();
            s.release();
            return 1;
        }
    )");
}
