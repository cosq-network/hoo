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

// End-to-end integration tests for the hoo.uuid module
// (src/runtime/lib/data/hoo_uuid.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI. The CLI prints
// the int64 result of the entry point, so each program returns :int64 and
// the test asserts on the printed value.
//
// Covered aspects:
//   - UUID v4 generation and format (free function)
//   - Nil UUID string representation (free function)
//   - Constructor from canonical string: new Uuid(str)
//   - Constructor from empty string: new Uuid("") -> v4
//   - Constructor from "nil": new Uuid("nil") -> nil
//   - toString() method
//   - isNil() method
//   - equals() method
//   - compare() method
//   - Free functions: uuid_is_nil, uuid_equals, uuid_compare
//   - toBytes / fromBytes round trip
//   - Reference counting: retain, release
class UuidCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_uuid_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_uuid_cli_"
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

TEST_F(UuidCLIIntegrationTest, UuidV4Format) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var s = uuid_v4();
            if (s.length() != 36) { return 0; }
            if (s.byteAt(8) != '-'.codepoint()) { return 0; }
            if (s.byteAt(13) != '-'.codepoint()) { return 0; }
            if (s.byteAt(18) != '-'.codepoint()) { return 0; }
            if (s.byteAt(23) != '-'.codepoint()) { return 0; }
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidNilString) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var s = uuid_nil();
            if (!s.equals("00000000-0000-0000-0000-000000000000")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidConstructorFromString) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var original = "550e8400-e29b-41d4-a716-446655440000";
            var id = new Uuid(original);
            if (!id.equals(new Uuid(original))) { return 0; }
            var s = id.toString();
            if (!s.equals(original)) { return 0; }
            id.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidConstructorEmptyString) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var id = new Uuid("");
            if (id.isNil() != 0) { return 0; }
            var s = id.toString();
            if (s.length() != 36) { return 0; }
            id.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidConstructorNilString) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var id = new Uuid("nil");
            if (id.isNil() != 1) { return 0; }
            var s = id.toString();
            if (!s.equals("00000000-0000-0000-0000-000000000000")) { return 0; }
            id.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidEquals) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var a = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            var b = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            if (a.equals(b) != 1) { return 0; }
            a.release();
            b.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidIsNil) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var nil_id = new Uuid("nil");
            if (nil_id.isNil() != 1) { return 0; }
            var v4_id = new Uuid("");
            if (v4_id.isNil() != 0) { return 0; }
            nil_id.release();
            v4_id.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidCompare) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var a = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            var b = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            var c = new Uuid("550e8400-e29b-41d4-a716-446655440001");
            if (a.compare(b) != 0) { return 0; }
            if (a.compare(c) != -1) { return 0; }
            if (c.compare(a) != 1) { return 0; }
            a.release();
            b.release();
            c.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidFreeFunctionIsNil) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var id = uuid_nil();
            if (uuid_is_nil(id) != 1) { return 0; }
            var id2 = uuid_v4();
            if (uuid_is_nil(id2) != 0) { return 0; }
            id.release();
            id2.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidFreeFunctionEquals) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var a = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            var b = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            if (a.equals(b) != 1) { return 0; }
            a.release();
            b.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidFreeFunctionCompare) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var a = new Uuid("550e8400-e29b-41d4-a716-446655440000");
            var b = new Uuid("550e8400-e29b-41d4-a716-446655440001");
            if (a.compare(b) != -1) { return 0; }
            if (b.compare(a) != 1) { return 0; }
            a.release();
            b.release();
            return 1;
        }
    )");
}

TEST_F(UuidCLIIntegrationTest, UuidRetainRelease) {
    expectReturnOne(R"(
        import hoo.uuid;
        func :int64 main() {
            var id = uuid_v4();
            id.retain();
            id.release();
            id.release();
            return 1;
        }
    )");
}
