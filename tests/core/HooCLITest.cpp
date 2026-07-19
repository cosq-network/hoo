#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <fstream>

#include "core/HooCLI.h"
#include "hvm/HOModule.h"

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

    void setBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) {
        binaryFiles[filename] = data;
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
    const std::vector<uint8_t>* getWrittenBinaryFile(const std::string& filename) const {
        auto it = writtenBinaryFiles.find(filename);
        if (it != writtenBinaryFiles.end()) {
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
    args.push_back(const_cast<char*>("hoo"));

    int result = cli->run(1, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(cli->getIOProvider()->getStderr().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("No input file") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorWhenFileNotFound) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
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
    args.push_back(const_cast<char*>("hoo"));
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
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--version"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 0);
    EXPECT_FALSE(cli->getIOProvider()->getStdout().empty());
    EXPECT_TRUE(cli->getIOProvider()->getStdout().find("hoo - Hoo v") != std::string::npos);
}

TEST_F(HooCLITest, ShowsHelp) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
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
    args.push_back(const_cast<char*>("hoo"));
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
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("script.txt"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Invalid file extension") != std::string::npos);
}

TEST_F(HooCLITest, ReturnsErrorOnUnknownCompileFlag) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("-c"));
    args.push_back(const_cast<char*>("script.ho"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Unknown option") != std::string::npos);
}

TEST_F(HooCLITest, RejectsStdinCompileFlag) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("-c"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("Unknown option") != std::string::npos);
}

TEST_F(HooCLITest, RejectsHoOutputPath) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("-o"));
    args.push_back(const_cast<char*>("out.ho"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(4, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("source compilation always produces .ha output") != std::string::npos);
}

TEST_F(HooCLITest, RejectsHoOutputPathLongForm) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--output=out.ho"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("source compilation always produces .ha output") != std::string::npos);
}

TEST_F(HooCLITest, AcceptsHaOutputPath) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("-o"));
    args.push_back(const_cast<char*>("out.ha"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(4, args.data());

    // Should not fail on output path validation (may fail on compilation, but not on CLI args)
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("source compilation always produces .ha output") == std::string::npos);
}

TEST_F(HooCLITest, RejectsOptionAsOutputPath) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("-o"));
    args.push_back(const_cast<char*>("--verbose"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(4, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("requires an output file path") != std::string::npos);
}

// ============================================================================
// .ho input rejection tests
// ============================================================================

TEST_F(HooCLITest, RejectsHoInputFile) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("module.ho"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find(".ho is an internal intermediate format") != std::string::npos);
}

TEST_F(HooCLITest, RejectsHoInputWithExecFlag) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--exec"));
    args.push_back(const_cast<char*>("module.ho"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find(".ho is an internal intermediate format") != std::string::npos);
}

// ============================================================================
// Output path validation tests
// ============================================================================

TEST_F(HooCLITest, RejectsOutputEqualsHoFormat) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    fakeIO->setFile("test.hoo", "func main() {}");
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--output=app.ho"));
    args.push_back(const_cast<char*>("test.hoo"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("source compilation always produces .ha output") != std::string::npos);
}

// ============================================================================
// REPL validation tests
// ============================================================================

TEST_F(HooCLITest, RejectsReplWithHaFile) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--repl"));
    args.push_back(const_cast<char*>("app.ha"));

    int result = cli->run(3, args.data());

    EXPECT_EQ(result, 1);
    EXPECT_TRUE(cli->getIOProvider()->getStderr().find("REPL preload input must be a .hoo source file") != std::string::npos);
}

// ============================================================================
// Help text includes archive info
// ============================================================================

TEST_F(HooCLITest, HelpTextMentionsHaArchive) {
    auto fakeIO = std::make_unique<FakeIOProvider>();
    auto cli = std::make_unique<HooCLI>(std::move(fakeIO));

    std::vector<char*> args;
    args.push_back(const_cast<char*>("hoo"));
    args.push_back(const_cast<char*>("--help"));

    int result = cli->run(2, args.data());

    EXPECT_EQ(result, 0);
    std::string help = cli->getIOProvider()->getStdout();
    EXPECT_TRUE(help.find(".ha") != std::string::npos);
    EXPECT_TRUE(help.find("Archive") != std::string::npos || help.find("archive") != std::string::npos);
}


