#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
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

class ArgsCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_args_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_args_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
#else
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
#else
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
#endif
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

    ExecResult compileAndRun(const std::string& source, const std::string& programArgs = "") {
        const std::string sourcePath = createSource(source);
        const std::string archivePath = createArchive();
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\" -- " + programArgs);
    }
};

TEST_F(ArgsCLIIntegrationTest, RawPositionalArguments) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            return args.count() * 10 + args.get(0).length() + args.get(1).length();
        }
    )", "alpha beta");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("29"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, RawNamedArgumentsAndValues) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            return args.has("output") * 10 + args.value("output").length();
        }
    )", "--output=result.txt");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("20"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, RawLongAndShortOptions) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            return args.has("o") * 10 + args.has("verbose");
        }
    )", "-o result.txt --verbose");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("11"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, RawDoubleDashStopsOptionParsing) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            return args.has("not-a-flag") * 10 + args.get(0).length();
        }
    )", "-- --not-a-flag");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("12"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ProgramNameIsAvailable) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            if args.programName().length() > 0 {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ParserDefaults) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("file", "-f", "--file", "", "default.txt");
            args.addInt("count", "-c", "--count", "", 7);
            args.addFloat("threshold", "-t", "--threshold", "", 0.5);
            args.addFlag("verbose", "-v", "--verbose", "");
            args.parse();
            if (args.getString("file").length() == 11 && args.getInt("count") == 7
                && args.getFloat("threshold") == 0.5 && args.getBool("verbose") == 0) {
                return 1;
            }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ParserNamedArguments) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("file", "-f", "--file", "", "");
            args.addInt("count", "-c", "--count", "", 0);
            args.addFloat("threshold", "-t", "--threshold", "", 0.0);
            args.addFlag("verbose", "-v", "--verbose", "");
            if args.parse() {
                if (args.getString("file").length() == 7 && args.getInt("count") == 5
                    && args.getFloat("threshold") == -0.25 && args.getBool("verbose") == 1) {
                    return 1;
                }
            }
            return 0;
        }
    )", "--file=out.txt --count 5 --threshold -0.25 --verbose");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ParserShortOptions) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("file", "-f", "", "", "");
            args.addInt("count", "-c", "", "", 0);
            args.parse();
            return args.getString("file").length() + args.getInt("count");
        }
    )", "-f short.txt -c 8");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("17"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ParserPositionalArguments) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addPositional("input", "Input file");
            args.addPositional("output", "Output file");
            args.parse();
            return args.getString("input").length() + args.getString("output").length();
        }
    )", "input.txt output.txt");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("19"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, RequiredArgumentPresent) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("input", "-i", "--input", "", "");
            args.setRequired("input", true);
            return args.parse();
        }
    )", "--input=data.txt");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, RequiredArgumentMissing) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("input", "-i", "--input", "", "");
            args.setRequired("input", true);
            return args.parse();
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, InvalidTypedValueFailsParsing) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 7);
            return args.parse();
        }
    )", "--count=not-a-number");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, MissingTypedValueFailsParsing) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 7);
            return args.parse();
        }
    )", "--count");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("0"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, NegativeTypedValues) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addInt("count", "-c", "--count", "", 0);
            args.addFloat("threshold", "-t", "--threshold", "", 0.0);
            if args.parse() {
                if (args.getInt("count") == -5 && args.getFloat("threshold") == -0.25) {
                    return 1;
                }
            }
            return 0;
        }
    )", "--count -5 --threshold -0.25");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, HelpTextContainsDefinitions) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("file", "-f", "--file", "Input file", "default.txt");
            args.addFlag("verbose", "-v", "--verbose", "Verbose mode");
            var help = args.helpText();
            return help.length() > 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ClearAndReuse) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.addString("first", "-f", "--first", "", "one");
            args.parse();
            args.clear();
            args.addString("second", "-s", "--second", "", "two");
            args.parse();
            return args.getString("second").length();
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("3"), std::string::npos);
}

TEST_F(ArgsCLIIntegrationTest, ReleaseIsSafeForCompletedProgram) {
    const auto result = compileAndRun(R"(
        import hoo.args;
        func :int64 main() {
            var args = new Args();
            args.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}
