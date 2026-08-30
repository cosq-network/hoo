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

// End-to-end integration tests for the hoo.character module
// (src/runtime/lib/character). Each test is a complete Hoo program compiled to
// a .ha archive and executed via the hoo CLI. The CLI prints the int64 result
// of the entry point, so each program returns :int64 and the test asserts on
// the printed value.
//
// Covered aspects:
//   - Creation: new Character(codepoint), character_from_utf8(str)
//   - Methods: codepoint(), length(), data(), print(), release()
//   - Character literals: 'A', 'é', '€', '😀'
//   - Iteration: for-in over a string yields Characters
//   - Character arrays
//   - UTF-8 handling: multi-byte sequences, first-scalar extraction,
//     empty-string null handle
//   - Codepoint validation: invalid scalar values map to U+FFFD
class CharacterCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_character_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_character_cli_"
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

TEST_F(CharacterCLIIntegrationTest, NewCharacterCodepointRoundTrip) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            if (new Character(65).codepoint() != 65) { return 0; }
            if (new Character(233).codepoint() != 233) { return 0; }
            if (new Character(8364).codepoint() != 8364) { return 0; }
            if (new Character(128512).codepoint() != 128512) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterByteLengths) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            if (new Character(65).length() != 1) { return 0; }
            if (new Character(233).length() != 2) { return 0; }
            if (new Character(8364).length() != 3) { return 0; }
            if (new Character(128512).length() != 4) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterDataReturnsUtf8) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var a = new Character(65);
            var aData = a.data();
            if (!aData.equals("A")) { return 0; }
            var e = new Character(233);
            var eData = e.data();
            if (!eData.equals("é")) { return 0; }
            var euro = new Character(8364);
            var euroData = euro.data();
            if (!euroData.equals("€")) { return 0; }
            var grin = new Character(128512);
            var grinData = grin.data();
            if (!grinData.equals("😀")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterLiteralCodepoints) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var a = 'A';
            if (a.codepoint() != 65) { return 0; }
            var e = 'é';
            if (e.codepoint() != 233) { return 0; }
            var euro = '€';
            if (euro.codepoint() != 8364) { return 0; }
            var grin = '😀';
            if (grin.codepoint() != 128512) { return 0; }
            if (grin.length() != 4) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterFromUtf8TakesFirstScalar) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var a = character_from_utf8("abcde");
            if (a.codepoint() != 97) { return 0; }
            var euro = character_from_utf8("€x");
            if (euro.codepoint() != 8364) { return 0; }
            var grin = character_from_utf8("😀tail");
            if (grin.codepoint() != 128512) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterFromUtf8EmptyStringYieldsNullHandle) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var ch = character_from_utf8("");
            if (ch.length() != 0) { return 0; }
            if (ch.codepoint() != 0) { return 0; }
            var d = ch.data();
            if (d.length() != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, ForInIterationYieldsCharacters) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var count = 0;
            var sum = 0;
            for c in "A€😀" {
                count += 1;
                sum += c.codepoint();
            }
            if (count != 3) { return 0; }
            if (sum != 65 + 8364 + 128512) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, CharacterArray) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var a = ['A', 'B', 'C'];
            if (a.length() != 3) { return 0; }
            var first = a[0];
            if (first.codepoint() != 65) { return 0; }
            var last = a[2];
            if (last.codepoint() != 67) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, PrintMethodOutputsCharacter) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var ch = new Character(65);
            ch.print();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("A"), std::string::npos) << result.output;
}

TEST_F(CharacterCLIIntegrationTest, InvalidCodepointsMapToReplacement) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            if (new Character(-1).codepoint() != 65533) { return 0; }
            if (new Character(55296).codepoint() != 65533) { return 0; }
            if (new Character(56319).codepoint() != 65533) { return 0; }
            if (new Character(1114112).codepoint() != 65533) { return 0; }
            return 1;
        }
    )");
}

TEST_F(CharacterCLIIntegrationTest, ReleaseMethodReleasesHandle) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var ch = new Character(65);
            var cp = ch.codepoint();
            ch.release();
            if (cp != 65) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
}

TEST_F(CharacterCLIIntegrationTest, MultipleMethodCallsOnSameHandle) {
    expectReturnOne(R"(
        import hoo;
        import hoo.character;
        func :int64 main() {
            var ch = new Character(128512);
            var len = ch.length();
            var cp = ch.codepoint();
            if (len != 4) { return 0; }
            if (cp != 128512) { return 0; }
            if (ch.codepoint() != cp) { return 0; }
            return 1;
        }
    )");
}
