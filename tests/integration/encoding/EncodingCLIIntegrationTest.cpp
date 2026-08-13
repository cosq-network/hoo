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

class EncodingCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_encoding_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_encoding_cli_"
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

TEST_F(EncodingCLIIntegrationTest, Base64EncodeDecodeRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var original = "Hello, World!";
            var encoded = encoding_base64_encode(original.data(), original.length());
            var decoded = encoding_base64_decode(encoded);
            if (decoded.equals(original)) { return 1; }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64EncodeBufferDecodeBufferRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var encoded = encoding_base64_encode_buffer(buf);
            var decoded = encoding_base64_decode_buffer(encoded);
            if (decoded.length() != 5) { return 0; }
            if (decoded.byteAt(0) != 72) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexEncodeDecodeRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var original = "Hello";
            var encoded = encoding_hex_encode(original.data(), original.length());
            var decoded = encoding_hex_decode(encoded);
            if (decoded.equals(original)) { return 1; }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexEncodeBufferDecodeBufferRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var encoded = encoding_hex_encode_buffer(buf);
            var decoded = encoding_hex_decode_buffer(encoded);
            if (decoded.length() != 5) { return 0; }
            if (decoded.byteAt(0) != 72) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, UrlEncodeDecodeRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var original = "a b=c/d?e=f";
            var encoded = encoding_url_encode(original);
            var decoded = encoding_url_decode(encoded);
            if (decoded.equals(original)) { return 1; }
            return 0;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64EncodeEmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var empty = "";
            var encoded = encoding_base64_encode(empty.data(), empty.length());
            if (encoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexEncodeEmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var empty = "";
            var encoded = encoding_hex_encode(empty.data(), empty.length());
            if (encoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64DecodeInvalidInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var decoded = encoding_base64_decode("not-valid-base64!!!");
            if (decoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexDecodeInvalidInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var decoded = encoding_hex_decode("not-valid-hex!!!");
            if (decoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64EncodeBufferWithSpecialBytes) {
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
            var encoded = encoding_base64_encode_buffer(buf);
            if (encoded.length() != 8) { return 0; }
            var decoded = encoding_base64_decode_buffer(encoded);
            if (decoded.length() != 4) { return 0; }
            if (decoded.byteAt(0) != 0) { return 0; }
            if (decoded.byteAt(3) != 255) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexEncodeBufferWithSpecialBytes) {
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
            var encoded = encoding_hex_encode_buffer(buf);
            if (encoded.length() != 8) { return 0; }
            var decoded = encoding_hex_decode_buffer(encoded);
            if (decoded.length() != 4) { return 0; }
            if (decoded.byteAt(3) != 255) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, UrlEncodeDecodeSpecialCharacters) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var original = "hello world!";
            var encoded = encoding_url_encode(original);
            var decoded = encoding_url_decode(encoded);
            if (!decoded.equals(original)) { return 0; }
            if (encoded.length() <= original.length()) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, ByteSliceFromBufferBase64) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_base64_encode_slice(view);
            byte_slice_release(view);
            if (encoded.length() != 8) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, ByteSliceFromBufferHex) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = Buffer.fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_hex_encode_slice(view);
            byte_slice_release(view);
            if (encoded.length() != 10) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64RoundTripWithBinaryData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var data = Buffer.fromBytes("ABCD", 4);
            data.setByte(0, 0);
            data.setByte(1, 1);
            data.setByte(2, 2);
            data.setByte(3, 255);
            var encoded = encoding_base64_encode_buffer(data);
            var decoded = encoding_base64_decode_buffer(encoded);
            if (decoded.length() != 4) { return 0; }
            if (decoded.byteAt(0) != 0) { return 0; }
            if (decoded.byteAt(3) != 255) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexRoundTripWithBinaryData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var data = Buffer.fromBytes("ABCD", 4);
            data.setByte(0, 0);
            data.setByte(1, 1);
            data.setByte(2, 2);
            data.setByte(3, 255);
            var encoded = encoding_hex_encode_buffer(data);
            var decoded = encoding_hex_decode_buffer(encoded);
            if (decoded.length() != 4) { return 0; }
            if (decoded.byteAt(0) != 0) { return 0; }
            if (decoded.byteAt(3) != 255) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, UrlEncodeDecodeSpaceAndSpecial) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.encoding;
        func :int64 main() {
            var original = "hello world!";
            var encoded = encoding_url_encode(original);
            var decoded = encoding_url_decode(encoded);
            if (!decoded.equals(original)) { return 0; }
            if (encoded.length() <= original.length()) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, Base64EncodeBufferEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = new Buffer();
            var encoded = encoding_base64_encode_buffer(buf);
            if (encoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(EncodingCLIIntegrationTest, HexEncodeBufferEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.encoding;
        func :int64 main() {
            var buf = new Buffer();
            var encoded = encoding_hex_encode_buffer(buf);
            if (encoded.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}
