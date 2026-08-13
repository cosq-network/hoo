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

class BufferCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult {
        std::string output;
        int exitCode;
    };

    std::string tempDir;
    std::string hooExe;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path().string();
        hooExe = HOO_EXECUTABLE;
    }

    std::string createSource(const std::string& source) {
        static int counter = 0;
        const std::string path = tempDir + "/hoo_buffer_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_buffer_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
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
};

TEST_F(BufferCLIIntegrationTest, FactoryFromBytes) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            if (buf.length() != 5) { return 0; }
            if (buf.byteAt(0) != 72) { return 0; }
            if (buf.byteAt(4) != 111) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, NewBufferEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            if (buf.length() != 0) { return 0; }
            if (buf.capacity() == 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, AppendString) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            var s = "Hello";
            buf.append(s, s.length());
            if (buf.length() != 5) { return 0; }
            if (buf.byteAt(0) != 72) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, AppendBuffer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var b1 = new Buffer();
            var s1 = "Hello";
            b1.append(s1, s1.length());
            var b2 = new Buffer();
            var s2 = " World";
            b2.append(s2, s2.length());
            b1.appendBuffer(b2);
            if (b1.length() != 11) { return 0; }
            if (b1.byteAt(5) != 32) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, ByteAtAndSetByte) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("abc", 3);
            if (buf.byteAt(0) != 97) { return 0; }
            if (buf.byteAt(1) != 98) { return 0; }
            var old = buf.setByte(1, 88);
            if (old != 98) { return 0; }
            if (buf.byteAt(1) != 88) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, CopyBuffer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("abc", 3);
            var cp = buf.copy();
            if (cp.length() != 3) { return 0; }
            if (cp.byteAt(0) != 97) { return 0; }
            cp.setByte(0, 90);
            if (buf.byteAt(0) != 97) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, ClearBuffer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            if (buf.length() != 5) { return 0; }
            buf.clear();
            if (buf.length() != 0) { return 0; }
            if (buf.capacity() == 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, SubBufferAlias) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello, World!", 13);
            var sub = buf.sub(7, 12);
            if (sub.length() != 5) { return 0; }
            if (sub.byteAt(0) != 87) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, DataPointer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("abc", 3);
            var ptr = buf.data();
            if (ptr == 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, EmptyBufferByteAtReturnsMinusOne) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            var b = buf.byteAt(0);
            if (b != -1) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, CapacityGrowsWithAppend) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            var cap0 = buf.capacity();
            var s = "HelloWorldHelloWorld";
            buf.append(s, s.length());
            var cap1 = buf.capacity();
            if (cap1 < s.length()) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, SetByteOutOfBoundsIsNoop) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("abc", 3);
            buf.setByte(99, 65);
            if (buf.byteAt(0) != 97) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, MultipleAppends) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            var a = "Hello";
            var b = " ";
            var c = "World";
            buf.append(a, a.length());
            buf.append(b, b.length());
            buf.append(c, c.length());
            if (buf.length() != 11) { return 0; }
            if (buf.byteAt(5) != 32) { return 0; }
            if (buf.byteAt(6) != 87) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(BufferCLIIntegrationTest, SubOutOfRangeReturnsEmptyBuffer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var sub = buf.sub(3, 10);
            if (sub.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}
