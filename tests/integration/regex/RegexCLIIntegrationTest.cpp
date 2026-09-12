// End-to-end CLI integration tests for the hoo.regex runtime module.
// Each test compiles a complete Hoo program to a .ha archive via the hoo
// executable and then runs it.  The CLI prints the int64 return value of
// main() as the final line, so programs return 1 on success, 0 on failure.

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

class RegexCLIIntegrationTest : public ::testing::Test {
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
        return temp + "/hoo_regex_cli_"
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
};

// ─────────────────────────────────────────────────────────────────────────
// Compilation
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, CompileBasic) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("[a-z]+");
            if (!re) { return 0; }
            re.release();
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, CompileFlagsCaseInsensitive) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = Regex.compile("hello", "i");
            if (!re) { return 0; }
            var ok = re.match("HELLO");
            re.release();
            if (ok != 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, CompileFlagsDotAll) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = Regex.compile("a.b", "s");
            if (!re) { return 0; }
            var ok = re.match("a\nb");
            re.release();
            if (ok != 1) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, CompileInvalidPatternReturnsNull) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("[");
            if (re) { re.release(); return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// match — full-string match
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, MatchExactAndPartial) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("hello");
            if (!re) { return 0; }
            if (re.match("hello") != 1) { re.release(); return 0; }
            if (re.match("hello world") != 0) { re.release(); return 0; }
            re.release();
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// search — partial / substring match
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, SearchPartialMatch) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("world");
            if (!re) { return 0; }
            if (re.search("hello world") != 1) { re.release(); return 0; }
            if (re.search("xyz") != 0) { re.release(); return 0; }
            re.release();
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// find — first match as string
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, FindReturnsString) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\w+");
            if (!re) { return 0; }
            var s = re.find("hello world");
            re.release();
            if (!s) { return 0; }
            if (!s.equals("hello")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, FindNoMatchReturnsNull) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            if (!re) { return 0; }
            var s = re.find("hello");
            re.release();
            if (s) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// find_all — all non-overlapping matches
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, FindAllCountAndContent) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            if (!re) { return 0; }
            var m = re.find_all("a1 b22 c333");
            re.release();
            if (!m) { return 0; }
            if (m.length() != 3) { return 0; }
            var s0: string = m.getString(0);
            var s1: string = m.getString(1);
            var s2: string = m.getString(2);
            if (!s0.equals("1")) { return 0; }
            if (!s1.equals("22")) { return 0; }
            if (!s2.equals("333")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, FindAllNoMatchesEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            if (!re) { return 0; }
            var m = re.find_all("hello");
            re.release();
            if (!m) { return 0; }
            if (m.length() != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// capture — groups from first match (index 0 = full match)
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, CaptureGroupsCountAndContent) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("(\\w+)@(\\w+)");
            if (!re) { return 0; }
            var g = re.capture("user@host.com");
            re.release();
            if (!g) { return 0; }
            if (g.length() != 3) { return 0; }
            var g0: string = g.getString(0);
            var g1: string = g.getString(1);
            var g2: string = g.getString(2);
            if (!g0.equals("user@host")) { return 0; }
            if (!g1.equals("user")) { return 0; }
            if (!g2.equals("host")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, CaptureNoMatchReturnsEmpty) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("(\\w+)@(\\w+)");
            if (!re) { return 0; }
            var g = re.capture("no-atom-here");
            re.release();
            if (!g) { return 0; }
            if (g.length() != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// group — single capture group by index
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, GroupByIndex) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("(\\w+)@(\\w+)");
            if (!re) { return 0; }
            var g1 = re.group("user@host", 1);
            var g2 = re.group("user@host", 2);
            re.release();
            if (!g1) { return 0; }
            if (!g2) { return 0; }
            if (!g1.equals("user")) { return 0; }
            if (!g2.equals("host")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, GroupNoMatchReturnsNull) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("(\\w+)@(\\w+)");
            if (!re) { return 0; }
            var g = re.group("no-atom-here", 1);
            re.release();
            if (g) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// replace — all non-overlapping matches
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, ReplaceAllMatches) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            if (!re) { return 0; }
            var s = re.replace("order 42 item 7", "X");
            re.release();
            if (!s) { return 0; }
            if (!s.equals("order X item X")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, ReplaceNoMatchUnchanged) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            if (!re) { return 0; }
            var s = re.replace("hello world", "X");
            re.release();
            if (!s) { return 0; }
            if (!s.equals("hello world")) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// split — split string by delimiter pattern
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, SplitByPattern) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var re = new Regex(",");
            if (!re) { return 0; }
            var p = re.split("a,b,c");
            re.release();
            if (!p) { return 0; }
            if (p.length() != 3) { return 0; }
            var s0: string = p.getString(0);
            var s1: string = p.getString(1);
            var s2: string = p.getString(2);
            if (!s0.equals("a")) { return 0; }
            if (!s1.equals("b")) { return 0; }
            if (!s2.equals("c")) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// Free functions — regex_match, regex_search, regex_replace, regex_split
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, FreeFunctionMatch) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var r = regex_match("[a-z]+", "hello");
            if (r != 1) { return 0; }
            var n = regex_match("[a-z]+", "123");
            if (n != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, FreeFunctionSearch) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var r = regex_search("world", "hello world");
            if (r != 1) { return 0; }
            var n = regex_search("xyz", "hello world");
            if (n != 0) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, FreeFunctionReplace) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var s = regex_replace("world", "hello world", "there");
            if (!s) { return 0; }
            if (!s.equals("hello there")) { return 0; }
            return 1;
        }
    )hoo"));
}

TEST_F(RegexCLIIntegrationTest, FreeFunctionSplit) {
    expectPass(compileAndRun(R"hoo(
        import hoo.regex;
        func :int64 main() {
            var p = regex_split(",", "a,b,c");
            if (!p) { return 0; }
            if (p.length() != 3) { return 0; }
            var s0: string = p.getString(0);
            var s1: string = p.getString(1);
            var s2: string = p.getString(2);
            if (!s0.equals("a")) { return 0; }
            if (!s1.equals("b")) { return 0; }
            if (!s2.equals("c")) { return 0; }
            return 1;
        }
    )hoo"));
}

// ─────────────────────────────────────────────────────────────────────────
// String output verification (println) — validates that captured/returned
// values are real strings, not just non-null pointers.
// ─────────────────────────────────────────────────────────────────────────

TEST_F(RegexCLIIntegrationTest, FindOutputPrintsCorrectString) {
    const auto result = compileAndRun(R"hoo(
        import hoo;
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\w+");
            var s = re.find("hello world");
            re.release();
            println(s);
            return 1;
        }
    )hoo");
    expectPass(result);
    expectOutputContains(result, "hello");
}

TEST_F(RegexCLIIntegrationTest, ReplaceOutputPrintsCorrectString) {
    const auto result = compileAndRun(R"hoo(
        import hoo;
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("\\d+");
            var s = re.replace("foo42bar", "NUM");
            re.release();
            println(s);
            return 1;
        }
    )hoo");
    expectPass(result);
    expectOutputContains(result, "fooNUMbar");
}

TEST_F(RegexCLIIntegrationTest, CaptureOutputPrintsGroupContent) {
    const auto result = compileAndRun(R"hoo(
        import hoo;
        import hoo.regex;
        func :int64 main() {
            var re = new Regex("(\\w+)@(\\w+)\\.com");
            var g = re.capture("alice@example.com");
            re.release();
            var g0: string = g.getString(0);
            var g1: string = g.getString(1);
            var g2: string = g.getString(2);
            println(g0);
            println(g1);
            println(g2);
            return 1;
        }
    )hoo");
    expectPass(result);
    expectOutputContains(result, "alice@example");
    expectOutputContains(result, "alice");
    expectOutputContains(result, "example");
}
