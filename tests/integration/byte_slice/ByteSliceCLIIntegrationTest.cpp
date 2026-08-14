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

class ByteSliceCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_byte_slice_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_byte_slice_cli_"
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

TEST_F(ByteSliceCLIIntegrationTest, Base64EncodeSliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_base64_encode_slice(view);
            byte_slice_release(view);
            if (!encoded.equals("SGVsbG8=")) { return 0; }
            if (encoded.length() != 8) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, HexEncodeSliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_hex_encode_slice(view);
            byte_slice_release(view);
            if (!encoded.equals("48656c6c6f")) { return 0; }
            if (encoded.length() != 10) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, Sha256SliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var digest = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!digest.equals("185f8db32271fe25f561a6fc938b2e264306ec304eda518007d1764826381969")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, Sha1SliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var digest = hashing_sha1_slice(view);
            byte_slice_release(view);
            if (!digest.equals("f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, Md5SliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var digest = hashing_md5_slice(view);
            byte_slice_release(view);
            if (!digest.equals("8b1a9953c4611296a827abf8c47804d7")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, Crc32SliceMatchesKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var crc = hashing_crc32_slice(view);
            byte_slice_release(view);
            if (crc != 4157704578) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, SliceOfEmptyBufferEncodesToEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = new Buffer();
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_base64_encode_slice(view);
            byte_slice_release(view);
            if (encoded.length() != 0) { return 0; }

            var view2 = byte_slice_from_buffer(buf);
            var hexed = encoding_hex_encode_slice(view2);
            byte_slice_release(view2);
            if (hexed.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, SliceOverSubRangeMatchesKnownValues) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello, World!", 13);
            var sub = buf.sub(7, 12);
            var view = byte_slice_from_buffer(sub);
            var encoded = encoding_base64_encode_slice(view);
            byte_slice_release(view);
            if (!encoded.equals("V29ybGQ=")) { return 0; }

            var view2 = byte_slice_from_buffer(sub);
            var crc = hashing_crc32_slice(view2);
            byte_slice_release(view2);
            if (crc != 4223024711) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, SliceWithBinaryBytesMatchesHex) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("ABCD", 4);
            buf.setByte(0, 0);
            buf.setByte(1, 1);
            buf.setByte(2, 2);
            buf.setByte(3, 255);
            var view = byte_slice_from_buffer(buf);
            var hexed = encoding_hex_encode_slice(view);
            byte_slice_release(view);
            if (!hexed.equals("000102ff")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, SliceRetainsBackingBuffer) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            buf = Buffer.fromBytes("Goodbye", 7);
            var digest = hashing_md5_slice(view);
            byte_slice_release(view);
            if (!digest.equals("8b1a9953c4611296a827abf8c47804d7")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, MultipleSequentialSlices) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view1 = byte_slice_from_buffer(buf);
            var a = encoding_hex_encode_slice(view1);
            byte_slice_release(view1);
            var view2 = byte_slice_from_buffer(buf);
            var b = encoding_base64_encode_slice(view2);
            byte_slice_release(view2);
            var view3 = byte_slice_from_buffer(buf);
            var c = hashing_sha1_slice(view3);
            byte_slice_release(view3);
            var view4 = byte_slice_from_buffer(buf);
            var d = hashing_crc32_slice(view4);
            byte_slice_release(view4);
            if (!a.equals("48656c6c6f")) { return 0; }
            if (!b.equals("SGVsbG8=")) { return 0; }
            if (!c.equals("f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0")) { return 0; }
            if (d != 4157704578) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(ByteSliceCLIIntegrationTest, LargeBufferDigestsMatchKnownValues) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
            var buf = Buffer.fromBytes(data, 256);
            var i = 0;
            while (i < 256) {
                buf.setByte(i, i);
                i = i + 1;
            }
            var view = byte_slice_from_buffer(buf);
            var digest = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!digest.equals("40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880")) { return 0; }

            var view2 = byte_slice_from_buffer(buf);
            var digest2 = hashing_sha1_slice(view2);
            byte_slice_release(view2);
            if (!digest2.equals("4916d6bdb7f78e6803698cab32d1586ea457dfc8")) { return 0; }

            var view3 = byte_slice_from_buffer(buf);
            var digest3 = hashing_md5_slice(view3);
            byte_slice_release(view3);
            if (!digest3.equals("e2c865db4162bed963bfaa9ef6ac18f0")) { return 0; }

            var view4 = byte_slice_from_buffer(buf);
            var crc = hashing_crc32_slice(view4);
            byte_slice_release(view4);
            if (crc != 688229491) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}
