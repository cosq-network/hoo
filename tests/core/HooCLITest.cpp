#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <map>

#include "core/HooCLI.h"

using namespace hooc;

class FakeIOProvider : public IOProvider {
public:
    std::optional<std::string> readFile(const std::string& filename) override {
        auto it = files.find(filename);
        if (it != files.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool writeFile(const std::string& filename, const std::string& content) override {
        writtenFiles[filename] = content;
        return true;
    }

    std::optional<std::vector<uint8_t>> readBinaryFile(const std::string& filename) override {
        auto it = binaryFiles.find(filename);
        if (it != binaryFiles.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) override {
        writtenBinaryFiles[filename] = data;
        return true;
    }

    std::string readStdin() override {
        return stdinContent;
    }

    void writeStdout(const std::string& output) override {
        stdoutOutput += output;
    }

    void writeStderr(const std::string& output) override {
        stderrOutput += output;
    }

    void setFile(const std::string& filename, const std::string& content) {
        files[filename] = content;
    }

    void setStdinContent(const std::string& content) {
        stdinContent = content;
    }

    std::string getStdout() const override { return stdoutOutput; }
    std::string getStderr() const override { return stderrOutput; }
    const std::string* getWrittenFile(const std::string& filename) const {
        auto it = writtenFiles.find(filename);
        if (it != writtenFiles.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void clearOutputs() {
        stdoutOutput.clear();
        stderrOutput.clear();
    }

private:
    std::map<std::string, std::string> files;
    std::map<std::string, std::string> writtenFiles;
    std::map<std::string, std::vector<uint8_t>> binaryFiles;
    std::map<std::string, std::vector<uint8_t>> writtenBinaryFiles;
    std::string stdinContent;
    std::string stdoutOutput;
    std::string stderrOutput;
};

class HooCLITest : public ::testing::Test {
protected:
};

TEST_F(HooCLITest, ReturnsErrorWhenNoInputFile) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));

    int result = cli->run(1, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(cli->getIOProvider()->getStderr().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("No input file") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorWhenFileNotFound) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("nonexistent.hoo"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(cli->getIOProvider()->getStderr().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Cannot open file") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorWhenFileIsEmpty) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("empty.hoo", "");

    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("empty.hoo"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(cli->getIOProvider()->getStderr().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("File is empty") != std::string::npos);
}

TEST_F(HooCLITest, ShowsVersion) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("--version"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_FALSE(cli->getIOProvider()->getStdout().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStdout().find("hooc version") != std::string::npos);
}

TEST_F(HooCLITest, ShowsHelp) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("--help"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_FALSE(cli->getIOProvider()->getStdout().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStdout().find("Usage:") != std::string::npos);
    EXPECT_TRUE(cli->getIOProvider()->getStdout().find("Options:") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorOnMultipleInputFiles) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("file1.hoo"));
    args.push_back(const_cast<char*>("file2.hoo"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Multiple input files") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorOnInvalidExtension) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("script.txt"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Invalid file extension") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorOnBytecodeWithCompileFlags) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("-c"));
    args.push_back(const_cast<char*>("script.ho"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Cannot use compilation flags") != std::string::npos);
}

TEST_F(HooCLITest, CompileOnlyMode) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("--verbose"));
    args.push_back(const_cast<char*>("-c"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(4, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Compile-only mode") != std::string::npos);
}

TEST_F(HooCLITest, OutputOptionReservedForFuture) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("-o"));
    args.push_back(const_cast<char*>("out.ho"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(4, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("AOT build and output emission") != std::string::npos);
}

TEST_F(HooCLITest, BytecodeFileExecutionReservedForFuture) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("script.ho", "bytecode content");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hooc"));
    args.push_back(const_cast<char*>("script.ho"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("AOT JIT execution") != std::string::npos);
}
