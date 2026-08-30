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

class CompressionCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_compression_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_compression_cli_"
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
};

TEST_F(CompressionCLIIntegrationTest, GzipRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo! Compression round trip.";
            var compressed = c.gzipCompress(original.data(), original.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo! Deflate round trip.";
            var compressed = c.deflateCompress(original.data(), original.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipCompressReducesSize) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "";
            var i = 0;
            while (i < 200) { original = original + "compress me repeatedly "; i = i + 1; }
            var compressed = c.gzipCompress(original.data(), original.length());
            if (compressed.length() <= 0) { return 0; }
            if (compressed.length() >= original.length()) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipCompressHasGzipHeader) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo!";
            var compressed = c.gzipCompress(original.data(), original.length());
            var bytes = Buffer.fromBytes(compressed, compressed.length());
            if (bytes.length() < 2) { return 0; }
            if (bytes.byteAt(0) != 31) { return 0; }
            if (bytes.byteAt(1) != 139) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateCompressHasNoGzipHeader) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo!";
            var compressed = c.deflateCompress(original.data(), original.length());
            var bytes = Buffer.fromBytes(compressed, compressed.length());
            if (bytes.length() < 2) { return 0; }
            if (bytes.byteAt(0) == 31 && bytes.byteAt(1) == 139) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipEmptyInputRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var empty = "";
            var compressed = c.gzipCompress(empty.data(), empty.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (decompressed.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateEmptyInputRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var empty = "";
            var compressed = c.deflateCompress(empty.data(), empty.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            if (decompressed.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipBinaryDataRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var data = Buffer.fromBytes("ABCD", 4);
            data.setByte(0, 0);
            data.setByte(1, 1);
            data.setByte(2, 128);
            data.setByte(3, 255);
            var compressed = c.gzipCompress(data.data(), data.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            var back = Buffer.fromBytes(decompressed, decompressed.length());
            if (back.length() != 4) { return 0; }
            if (back.byteAt(0) != 0) { return 0; }
            if (back.byteAt(1) != 1) { return 0; }
            if (back.byteAt(2) != 128) { return 0; }
            if (back.byteAt(3) != 255) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateBinaryDataRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var data = Buffer.fromBytes("ABCD", 4);
            data.setByte(0, 0);
            data.setByte(1, 254);
            data.setByte(2, 127);
            data.setByte(3, 255);
            var compressed = c.deflateCompress(data.data(), data.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            var back = Buffer.fromBytes(decompressed, decompressed.length());
            if (back.length() != 4) { return 0; }
            if (back.byteAt(0) != 0) { return 0; }
            if (back.byteAt(1) != 254) { return 0; }
            if (back.byteAt(2) != 127) { return 0; }
            if (back.byteAt(3) != 255) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipLargeDataRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "";
            var i = 0;
            while (i < 5000) { original = original + "Hello, Hoo! "; i = i + 1; }
            var compressed = c.gzipCompress(original.data(), original.length());
            if (compressed.length() <= 0) { return 0; }
            if (compressed.length() >= original.length()) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (decompressed.length() != original.length()) { return 0; }
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateLargeDataRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "";
            var i = 0;
            while (i < 5000) { original = original + "Hello, Hoo! "; i = i + 1; }
            var compressed = c.deflateCompress(original.data(), original.length());
            if (compressed.length() <= 0) { return 0; }
            if (compressed.length() >= original.length()) { return 0; }
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            if (decompressed.length() != original.length()) { return 0; }
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, IncompressibleDataRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var seed = "";
            var k = 0;
            while (k < 256) { seed = seed + "A"; k = k + 1; }
            var data = Buffer.fromBytes(seed, seed.length());
            var i = 0;
            while (i < 256) {
                var r = (i * 37 + 11) % 256;
                data.setByte(i, r);
                i = i + 1;
            }
            if (data.length() != 256) { return 0; }
            var compressed = c.gzipCompress(data.data(), data.length());
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            var back = Buffer.fromBytes(decompressed, decompressed.length());
            if (back.length() != 256) { return 0; }
            var j = 0;
            while (j < 256) {
                if (back.byteAt(j) != data.byteAt(j)) { return 0; }
                j = j + 1;
            }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceGzipCompressRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo! Slice gzip round trip.";
            var buf = Buffer.fromBytes(original, original.length());
            var view = byte_slice_from_buffer(buf);
            var compressed = compression_gzip_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceDeflateCompressRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo! Slice deflate round trip.";
            var buf = Buffer.fromBytes(original, original.length());
            var view = byte_slice_from_buffer(buf);
            var compressed = compression_deflate_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceGzipCompressReducesSize) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "";
            var i = 0;
            while (i < 200) { original = original + "compress me repeatedly "; i = i + 1; }
            var buf = Buffer.fromBytes(original, original.length());
            var view = byte_slice_from_buffer(buf);
            var compressed = compression_gzip_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() <= 0) { return 0; }
            if (compressed.length() >= original.length()) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (!decompressed.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceCompressMatchesInstanceCompress) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "";
            var i = 0;
            while (i < 100) { original = original + "same input both paths "; i = i + 1; }
            var buf = Buffer.fromBytes(original, original.length());
            var view = byte_slice_from_buffer(buf);
            var viaSlice = compression_gzip_compress_slice(view);
            byte_slice_release(view);
            var viaInstance = c.gzipCompress(original.data(), original.length());
            if (viaSlice.length() != viaInstance.length()) { return 0; }
            var backSlice = c.gzipDecompress(viaSlice.data(), viaSlice.length());
            var backInstance = c.gzipDecompress(viaInstance.data(), viaInstance.length());
            if (!backSlice.equals(original)) { return 0; }
            if (!backInstance.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceOnEmptyBufferCompresses) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var buf = new Buffer();
            var view = byte_slice_from_buffer(buf);
            var compressed = compression_gzip_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() <= 0) { return 0; }
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            if (decompressed.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipDecompressInvalidDataReturnsEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var invalid = "this is not gzip data at all";
            var decompressed = c.gzipDecompress(invalid.data(), invalid.length());
            if (decompressed.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateDecompressInvalidDataReturnsEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var invalid = "this is not deflate data at all";
            var decompressed = c.deflateDecompress(invalid.data(), invalid.length());
            if (decompressed.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipDataNotDecodableAsDeflate) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo!";
            var gz = c.gzipCompress(original.data(), original.length());
            if (gz.length() <= 0) { return 0; }
            var wrong = c.deflateDecompress(gz.data(), gz.length());
            if (wrong.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateDataNotDecodableAsGzip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "Hello, Hoo!";
            var df = c.deflateCompress(original.data(), original.length());
            if (df.length() <= 0) { return 0; }
            var wrong = c.gzipDecompress(df.data(), df.length());
            if (wrong.length() != 0) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, GzipTruncatedDataReturnsEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "truncation must be rejected";
            var compressed = c.gzipCompress(original.data(), original.length());
            if (compressed.length() < 2) { return 0; }
            var compressedBuffer = Buffer.fromBytes(compressed, compressed.length());
            var truncated = compressedBuffer.sub(0, compressed.length() - 1);
            var decompressed = c.gzipDecompress(truncated.data(), truncated.length());
            c.release();
            if (decompressed.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, DeflateTruncatedDataReturnsEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var original = "truncation must be rejected";
            var compressed = c.deflateCompress(original.data(), original.length());
            if (compressed.length() < 2) { return 0; }
            var compressedBuffer = Buffer.fromBytes(compressed, compressed.length());
            var truncated = compressedBuffer.sub(0, compressed.length() - 1);
            var decompressed = c.deflateDecompress(truncated.data(), truncated.length());
            c.release();
            if (decompressed.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, BufferDataPreservesEmbeddedZeroes) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var input = Buffer.fromBytes("0123456789", 10);
            input.setByte(1, 0);
            input.setByte(4, 0);
            input.setByte(9, 255);
            var compressed = c.gzipCompress(input.data(), input.length());
            var restored = c.gzipDecompress(compressed.data(), compressed.length());
            var output = Buffer.fromBytes(restored, restored.length());
            if (output.length() != 10) { return 0; }
            if (output.byteAt(0) != 48 || output.byteAt(1) != 0) { return 0; }
            if (output.byteAt(4) != 0 || output.byteAt(9) != 255) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, EmptyDeflateSliceRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var c = new Compression();
            var input = new Buffer();
            var view = byte_slice_from_buffer(input);
            var compressed = compression_deflate_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() <= 0) { return 0; }
            var restored = c.deflateDecompress(compressed.data(), compressed.length());
            c.release();
            if (restored.length() != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, SliceDeflateOutputHasNoGzipHeader) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        import hoo.buffer;
        func :int64 main() {
            var input = Buffer.fromBytes("slice framing", 13);
            var view = byte_slice_from_buffer(input);
            var compressed = compression_deflate_compress_slice(view);
            byte_slice_release(view);
            if (compressed.length() < 2) { return 0; }
            var output = Buffer.fromBytes(compressed, compressed.length());
            if (output.byteAt(0) == 31 && output.byteAt(1) == 139) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, NestedCompressionRoundTrip) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "nested compression test string for hoo";
            var inner = c.gzipCompress(original.data(), original.length());
            if (inner.length() <= 0) { return 0; }
            var outer = c.deflateCompress(inner.data(), inner.length());
            if (outer.length() <= 0) { return 0; }
            var mid = c.deflateDecompress(outer.data(), outer.length());
            if (mid.length() != inner.length()) { return 0; }
            var back = c.gzipDecompress(mid.data(), mid.length());
            if (!back.equals(original)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, MultipleOperationsSameInstance) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var a = "first string to compress";
            var b = "second string to compress";
            var c1 = c.gzipCompress(a.data(), a.length());
            var d1 = c.deflateCompress(b.data(), b.length());
            var backA = c.gzipDecompress(c1.data(), c1.length());
            var backB = c.deflateDecompress(d1.data(), d1.length());
            var c2 = c.gzipCompress(b.data(), b.length());
            var backC = c.gzipDecompress(c2.data(), c2.length());
            if (!backA.equals(a)) { return 0; }
            if (!backB.equals(b)) { return 0; }
            if (!backC.equals(b)) { return 0; }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, MultipleInstancesIndependent) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c1 = new Compression();
            var c2 = new Compression();
            var first = "data for the first instance";
            var second = "data for the second instance";
            var gz1 = c1.gzipCompress(first.data(), first.length());
            var df2 = c2.deflateCompress(second.data(), second.length());
            var back1 = c2.gzipDecompress(gz1.data(), gz1.length());
            var back2 = c1.deflateDecompress(df2.data(), df2.length());
            if (!back1.equals(first)) { return 0; }
            if (!back2.equals(second)) { return 0; }
            c1.release();
            c2.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, ManyCompressDecompressCycles) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var original = "cycle test data";
            var i = 0;
            while (i < 100) {
                var compressed = c.gzipCompress(original.data(), original.length());
                var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
                if (!decompressed.equals(original)) { return 0; }
                var deflated = c.deflateCompress(original.data(), original.length());
                var inflated = c.deflateDecompress(deflated.data(), deflated.length());
                if (!inflated.equals(original)) { return 0; }
                i = i + 1;
            }
            c.release();
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(CompressionCLIIntegrationTest, ReleaseReturnsZero) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.compression;
        func :int64 main() {
            var c = new Compression();
            var r = c.release();
            if (r != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}
