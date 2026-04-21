#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

#include "ho/HoCLI.h"

namespace fs = std::filesystem;

class HoCLITest : public ::testing::Test {
protected:
    void SetUp() override {
        originalCwd = fs::current_path();
        testDir = originalCwd / "ho_cli_test_temp";
        fs::create_directory(testDir);
        fs::current_path(testDir);
    }

    void TearDown() override {
        fs::current_path(originalCwd);
        fs::remove_all(testDir);
    }

    fs::path originalCwd;
    fs::path testDir;
};

TEST_F(HoCLITest, ShowsHelpWithNoArgs) {
    std::vector<std::string> args;
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ShowsHelpWithDashH) {
    std::vector<std::string> args = {"-h"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ShowsHelpWithDashDashHelp) {
    std::vector<std::string> args = {"--help"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ShowsVersionWithDashV) {
    std::vector<std::string> args = {"-v"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ShowsVersionWithDashDashVersion) {
    std::vector<std::string> args = {"--version"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ShowsHelpWhenNoInputFile) {
    std::vector<std::string> args;
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, ReturnsErrorOnUnknownOption) {
    std::vector<std::string> args = {"--unknown"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_FALSE(opts.errorMessage.empty());
    EXPECT_TRUE(opts.errorMessage.find("Unknown option") != std::string::npos);
}

TEST_F(HoCLITest, ReturnsErrorOnMultipleInputFiles) {
    std::vector<std::string> args = {"file1.hoo", "file2.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_FALSE(opts.errorMessage.empty());
    EXPECT_TRUE(opts.errorMessage.find("Multiple input files") != std::string::npos);
}

TEST_F(HoCLITest, AcceptsHooExtension) {
    std::ofstream testFile("test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"test.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.inputFile == "test.hoo");

    std::string error = validateHoInputFile(opts);
    EXPECT_TRUE(error.empty());
}

TEST_F(HoCLITest, AcceptsHoExtension) {
    std::ofstream testFile("test.ho");
    testFile << "bytecode content";
    testFile.close();

    std::vector<std::string> args = {"test.ho"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.inputFile == "test.ho");

    std::string error = validateHoInputFile(opts);
    EXPECT_TRUE(error.empty());
}

TEST_F(HoCLITest, ReturnsErrorOnInvalidExtension) {
    std::vector<std::string> args = {"script.txt"};
    HoOptions opts = parseHoArgs(args);

    std::string error = validateHoInputFile(opts);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find(".hoo or .ho") != std::string::npos);
}

TEST_F(HoCLITest, ReturnsErrorOnAbsolutePath) {
    std::vector<std::string> args = {"/tmp/test.hoo"};
    HoOptions opts = parseHoArgs(args);

    std::string error = validateHoInputFile(opts);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("relative path") != std::string::npos);
}

TEST_F(HoCLITest, AcceptsSubdirectoryPath) {
    fs::create_directory("subdir");
    std::ofstream testFile("subdir/test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"subdir/test.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.inputFile == "subdir/test.hoo");

    std::string error = validateHoInputFile(opts);
    EXPECT_TRUE(error.empty());
}

TEST_F(HoCLITest, AcceptsNestedSubdirectoryPath) {
    fs::create_directory("level1");
    fs::create_directory("level1/level2");
    std::ofstream testFile("level1/level2/test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"level1/level2/test.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.inputFile == "level1/level2/test.hoo");

    std::string error = validateHoInputFile(opts);
    EXPECT_TRUE(error.empty());
}

TEST_F(HoCLITest, ReturnsErrorOnPathEscapingCwd) {
    std::string relativePath = "..";
    fs::path testFilePath = testDir.parent_path() / "outside_test.hoo";
    std::ofstream testFile(testFilePath);
    testFile << "func main() {}";
    testFile.close();

    relativePath = "../outside_test.hoo";
    std::vector<std::string> args = {relativePath};
    HoOptions opts = parseHoArgs(args);

    std::string error = validateHoInputFile(opts);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("under the current working directory") != std::string::npos);
}

TEST_F(HoCLITest, ReturnsErrorOnNonExistentFile) {
    std::vector<std::string> args = {"nonexistent.hoo"};
    HoOptions opts = parseHoArgs(args);

    std::string error = validateHoInputFile(opts);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("does not exist") != std::string::npos);
}

TEST_F(HoCLITest, AcceptsBuildModeWithHooFile) {
    std::ofstream testFile("test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"--build", "test.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.buildMode);
    EXPECT_TRUE(opts.inputFile == "test.hoo");

    std::string error = validateHoInputFile(opts);
    EXPECT_TRUE(error.empty());
}

TEST_F(HoCLITest, ReturnsErrorOnBuildModeWithHoFile) {
    std::ofstream testFile("test.ho");
    testFile << "bytecode";
    testFile.close();

    std::vector<std::string> args = {"--build", "test.ho"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_TRUE(opts.buildMode);

    std::string error = validateHoInputFile(opts);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("--build") != std::string::npos);
}

TEST_F(HoCLITest, BuildModeDefaultsToFalse) {
    std::ofstream testFile("test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"test.hoo"};
    HoOptions opts = parseHoArgs(args);
    EXPECT_FALSE(opts.buildMode);
}

TEST_F(HoCLITest, RunModeWithHooFile) {
    std::ofstream testFile("test.hoo");
    testFile << "func main() {}";
    testFile.close();

    std::vector<std::string> args = {"test.hoo"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}

TEST_F(HoCLITest, RunModeWithHoFile) {
    std::ofstream testFile("test.ho");
    testFile << "bytecode";
    testFile.close();

    std::vector<std::string> args = {"test.ho"};
    int result = runHo(args);
    EXPECT_EQ(result, 0);
}
