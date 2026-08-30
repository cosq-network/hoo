#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#define popen _popen
#define pclose _pclose
#define unlink _unlink
#define stat _stat
#else
#include <sys/wait.h>
#include <sys/stat.h>
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
        tempDir = std::filesystem::temp_directory_path().string();
        for (char& c : tempDir) {
            if (c == '\\') c = '/';
        }
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
    EXPECT_NE(r.output.find("hoo - Hoo v"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, ShortVersionFlag) {
    auto r = runHoo("-v");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("hoo - Hoo v"), std::string::npos);
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
#ifdef _WIN32
    auto r = runHoo("Z:\\nonexistent_hoo_file.hoo");
#else
    auto r = runHoo("/tmp/nonexistent_hoo_file.hoo");
#endif
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
    auto r = runHoo("--exec " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("42"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, RunSourceFile) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    auto r = runHoo("--exec " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("42"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, RunFailsOnSyntaxError) {
    std::string src = createTempFile("func :int64 main() { syntax error }\n");
    auto r = runHoo(src);
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("Build planning failed"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, CompileAndOutputArchive) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    std::string outPath = tempDir + "/hoo_cli_out_" + std::to_string(std::time(nullptr)) + ".ha";
    auto r = runHoo("-o " + outPath + " " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("successfully built"), std::string::npos);
#ifdef _WIN32
    struct _stat st;
    EXPECT_EQ(_stat(outPath.c_str(), &st), 0);
#else
    struct stat st;
    EXPECT_EQ(stat(outPath.c_str(), &st), 0);
#endif
    EXPECT_GT(st.st_size, 0);
}

TEST_F(HooCLIIntegrationTest, CompileAndOutputArchiveWithEqualsSyntax) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    std::string outPath = tempDir + "/hoo_cli_out_eq_" + std::to_string(std::time(nullptr)) + ".ha";
    auto r = runHoo("--output=" + outPath + " " + src);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("successfully built"), std::string::npos);
#ifdef _WIN32
    struct _stat st;
    EXPECT_EQ(_stat(outPath.c_str(), &st), 0);
#else
    struct stat st;
    EXPECT_EQ(stat(outPath.c_str(), &st), 0);
#endif
    EXPECT_GT(st.st_size, 0);
}

TEST_F(HooCLIIntegrationTest, RejectsOptionAsOutputPath) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    auto r = runHoo("-o --verbose " + src);
    EXPECT_NE(r.exitCode, 0);
    EXPECT_NE(r.output.find("requires an output file path"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, PassesArgumentsAfterDoubleDashToProgram) {
    std::string src = createTempFile(
        "import hoo.args;\n"
        "func :int64 main() { return args_count(); }\n");
    auto r = runHoo("--exec " + src + " -- first second");
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("2"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, GeneratedArchiveRunnable) {
    std::string src = createTempFile("func :int64 main() { return 42; }\n");
    std::string outPath = tempDir + "/hoo_cli_out2_" + std::to_string(std::time(nullptr)) + ".ha";
    auto r1 = runHoo("-o " + outPath + " " + src);
    EXPECT_EQ(r1.exitCode, 0);
    EXPECT_NE(r1.output.find("successfully built"), std::string::npos);

    auto r2 = runHoo(outPath);
    EXPECT_EQ(r2.exitCode, 0);
    EXPECT_NE(r2.output.find("42"), std::string::npos);
}

TEST_F(HooCLIIntegrationTest, RunArchiveFile) {
    std::string src = createTempFile("func :int64 main() { return 7; }\n");
    std::string outPath = tempDir + "/hoo_cli_out3_" + std::to_string(std::time(nullptr)) + ".ha";
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

TEST_F(HooCLIIntegrationTest, CrossFileLocalImports) {
    std::string b_src = createTempFile("func :int64 get_value() { return 100; }\n");
    
    // b_src is named something like /tmp/hoo_cli_int_1234_1.hoo
    // We need to import it by its normalized name.
    // However, the test framework creates temp files with arbitrary names.
    // So let's create a specific directory structure for this test.
    std::string testDir = tempDir + "/cross_file_test_" + std::to_string(std::time(nullptr));
    std::filesystem::create_directory(testDir);
    
    std::string a_path = testDir + "/a.hoo";
    std::string b_path = testDir + "/b.hoo";
    
    std::ofstream fb(b_path);
    fb << "func :int64 get_value() { return 100; }\n";
    fb.close();
    
    std::ofstream fa(a_path);
    fa << "import b;\n"
       << "func :int64 main() { return get_value(); }\n";
    fa.close();
    
    auto r = runHoo("--exec " + a_path);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_NE(r.output.find("100"), std::string::npos);
    
    std::filesystem::remove_all(testDir);
}
