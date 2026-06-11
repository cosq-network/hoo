#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#define unlink _unlink
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef HOO_EXECUTABLE
#error "HOO_EXECUTABLE must be defined via CMake -D"
#endif

class HooCLIIntegrationTest : public ::testing::Test {
protected:
    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = testing::TempDir();
        hooExe = HOO_EXECUTABLE;
    }

    std::string createTempFile(const std::string& content, const std::string& ext = ".hoo") {
        static int counter = 0;
        std::string path = tempDir + "/hoo_cli_int_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter)
            + ext;
        std::ofstream f(path);
        f << content;
        f.close();
        return path;
    }

    struct ExecResult {
        std::string output;
        int exitCode;
    };

    ExecResult runHoo(const std::string& args) {
        std::string cmd = "\"" + hooExe + "\" " + args + " 2>&1";
        FILE* pipe = popen(cmd.c_str(), "r");
        ExecResult result;
        if (!pipe) {
            result.exitCode = -1;
            result.output = "popen failed";
            return result;
        }
        std::ostringstream out;
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            out << buf;
        }
        int status = pclose(pipe);
#ifdef _WIN32
        result.exitCode = status;
#else
        result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
        result.output = out.str();
        return result;
    }
};

TEST_F(HooCLIIntegrationTest, HelpFlag) {
    auto r = runHoo("--help");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("Usage:"), std::string::npos);
    EXPECT_NE(r.output.find("Options:"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, ShortHelpFlag) {
    auto r = runHoo("-h");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("Usage:"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, VersionFlag) {
    auto r = runHoo("--version");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("hoo version"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, ShortVersionFlag) {
    auto r = runHoo("-v");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("hoo version"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, UnknownOption) {
    auto r = runHoo("--bogus");
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Unknown option"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, NoInputFile) {
    auto r = runHoo("");
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("No input file"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, FileNotFound) {
    auto r = runHoo("/tmp/nonexistent_hoo_file.hoo");
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Cannot open file"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, InvalidExtension) {
    auto r = runHoo("input.txt");
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Invalid file extension"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, CompileAndRunSource) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    auto r = runHoo(src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("42"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, CompileOnly) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    auto r = runHoo("-c " + src);
    EXPECT_EQ(r.exitCode, 0);
}

TEST_F(HooCLIIntegrationTest, CompileSyntaxError) {
    std::string src = createTempFile("func :int64 main() { syntax error }\n");
    auto r = runHoo("-c " + src);
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Compilation failed"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, CompileAndOutputBytecode) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    std::string outPath = tempDir + "/hoo_cli_out_" + std::to_string(std::time(nullptr)) + ".ho";
    auto r = runHoo("-o " + outPath + " " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("bytecode saved"), std::string::npos);
    struct stat st;
    EXPECT_EQ(stat(outPath.c_str(), &st), 0);
    EXPECT_GT(st.st_size, 0);
}

TEST_F(HooCLIIntegrationTest, CompileOnlyWithBytecodeFile) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    std::string outPath = tempDir + "/hoo_cli_out2_" + std::to_string(std::time(nullptr)) + ".ho";
    runHoo("-o " + outPath + " " + src);
    auto r = runHoo("-c " + outPath);
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Cannot use compilation flags"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, RunBytecodeFile) {
    std::string src = createTempFile("func :int64 main() { return 7; }\n");
    std::string outPath = tempDir + "/hoo_cli_out3_" + std::to_string(std::time(nullptr)) + ".ho";
    runHoo("-o " + outPath + " " + src);
    auto r = runHoo(outPath);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("7"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, MultipleInputFilesError) {
    std::string a = createTempFile("func :int64 main() { return 1; }\n");
    std::string b = createTempFile("func :int64 main() { return 2; }\n");
    auto r = runHoo(a + " " + b);
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Multiple input files"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, VerboseFlag) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    auto r = runHoo("--verbose " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("VERBOSE"), std::string::npos);
    EXPECT_NE(r.output.find("Module name:"), std::string::npos);
}
