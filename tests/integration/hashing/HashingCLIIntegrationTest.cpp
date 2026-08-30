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

class HashingCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_hashing_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_hashing_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    std::string createTempFile(const std::string& content) {
        static int counter = 0;
        const std::string path = tempDir + "/hoo_hashing_data_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".tmp";
        std::ofstream file(path, std::ios::binary);
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        return path;
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

// ─── Raw data: SHA-256 ──────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha256KnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_sha256(data.data(), data.length());
            if (!hash.equals("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256EmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "";
            var hash = hashing_sha256(data.data(), data.length());
            if (!hash.equals("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256DigestLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_sha256(data.data(), data.length());
            if (hash.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256Deterministic) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var h1 = hashing_sha256(data.data(), data.length());
            var h2 = hashing_sha256(data.data(), data.length());
            if (!h1.equals(h2)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256DifferentInputsDifferentHashes) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var h1 = hashing_sha256("hello".data(), 5);
            var h2 = hashing_sha256("world".data(), 5);
            if (h1.equals(h2)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Raw data: SHA-1 ────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha1KnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_sha1(data.data(), data.length());
            if (!hash.equals("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha1EmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "";
            var hash = hashing_sha1(data.data(), data.length());
            if (!hash.equals("da39a3ee5e6b4b0d3255bfef95601890afd80709")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha1DigestLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_sha1(data.data(), data.length());
            if (hash.length() != 40) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Raw data: MD5 ──────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Md5KnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_md5(data.data(), data.length());
            if (!hash.equals("5d41402abc4b2a76b9719d911017c592")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Md5EmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "";
            var hash = hashing_md5(data.data(), data.length());
            if (!hash.equals("d41d8cd98f00b204e9800998ecf8427e")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Md5DigestLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var hash = hashing_md5(data.data(), data.length());
            if (hash.length() != 32) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Raw data: CRC-32 ───────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Crc32KnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var crc = hashing_crc32(data.data(), data.length());
            if (crc != 907060870) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Crc32EmptyInput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "";
            var crc = hashing_crc32(data.data(), data.length());
            if (crc != 0) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Crc32KnownString) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "123456789";
            var crc = hashing_crc32(data.data(), data.length());
            if (crc != 3421780262) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Raw data: HMAC-SHA256 ──────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, HmacSha256KnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var key = "key";
            var data = "data";
            var mac = hashing_hmac_sha256(key.data(), key.length(), data.data(), data.length());
            if (!mac.equals("5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, HmacSha256DigestLength) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var key = "key";
            var data = "data";
            var mac = hashing_hmac_sha256(key.data(), key.length(), data.data(), data.length());
            if (mac.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, HmacSha256DifferentKeysDifferentResults) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "data";
            var mac1 = hashing_hmac_sha256("key1".data(), 4, data.data(), data.length());
            var mac2 = hashing_hmac_sha256("key2".data(), 4, data.data(), data.length());
            if (mac1.equals(mac2)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, HmacSha256EmptyKeyAndData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var mac = hashing_hmac_sha256("".data(), 0, "".data(), 0);
            if (mac.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── File: SHA-256 ──────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha256FileKnownValue) {
    const std::string tmpPath = createTempFile("hello");
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var hash = hashing_sha256_file(")") + tmpPath + R"(");
            if (!hash.equals("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
    std::remove(tmpPath.c_str());
}

TEST_F(HashingCLIIntegrationTest, Sha256FileEmpty) {
    const std::string tmpPath = createTempFile("");
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var hash = hashing_sha256_file(")") + tmpPath + R"(");
            if (!hash.equals("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
    std::remove(tmpPath.c_str());
}

TEST_F(HashingCLIIntegrationTest, Sha256FileMatchesDirectHash) {
    const std::string tmpPath = createTempFile("hello");
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var from_data = hashing_sha256(data.data(), data.length());
            var from_file = hashing_sha256_file(")") + tmpPath + R"(");
            if (!from_data.equals(from_file)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
    std::remove(tmpPath.c_str());
}

// ─── Buffer variants ─────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha256BufferKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var hash = hashing_sha256_buffer(buf);
            if (!hash.equals("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha1BufferKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var hash = hashing_sha1_buffer(buf);
            if (!hash.equals("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Md5BufferKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var hash = hashing_md5_buffer(buf);
            if (!hash.equals("5d41402abc4b2a76b9719d911017c592")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Crc32BufferKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var crc = hashing_crc32_buffer(buf);
            if (crc != 907060870) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256BufferEmpty) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = new Buffer();
            var hash = hashing_sha256_buffer(buf);
            if (!hash.equals("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256BufferMatchesRawData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var h1 = hashing_sha256(data.data(), data.length());
            var h2 = hashing_sha256_buffer(buf);
            if (!h1.equals(h2)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, HmacSha256BufferKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var key = Buffer.fromBytes("key", 3);
            var data = Buffer.fromBytes("data", 4);
            var mac = hashing_hmac_sha256_buffer(key, data);
            if (!mac.equals("5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Slice variants ──────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha256SliceKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var view = byte_slice_from_buffer(buf);
            var hash = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!hash.equals("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha1SliceKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var view = byte_slice_from_buffer(buf);
            var hash = hashing_sha1_slice(view);
            byte_slice_release(view);
            if (!hash.equals("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Md5SliceKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var view = byte_slice_from_buffer(buf);
            var hash = hashing_md5_slice(view);
            byte_slice_release(view);
            if (!hash.equals("5d41402abc4b2a76b9719d911017c592")) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Crc32SliceKnownValue) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = Buffer.fromBytes("hello", 5);
            var view = byte_slice_from_buffer(buf);
            var crc = hashing_crc32_slice(view);
            byte_slice_release(view);
            if (crc != 907060870) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256SliceMatchesRawData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var h1 = hashing_sha256(data.data(), data.length());
            var view = byte_slice_from_buffer(buf);
            var h2 = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!h1.equals(h2)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Cross-validation: raw ↔ buffer ↔ slice ─────────────────────────────────

TEST_F(HashingCLIIntegrationTest, CrossValidateSha256AllVariants) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var h_raw = hashing_sha256(data.data(), data.length());
            var h_buf = hashing_sha256_buffer(buf);
            var view = byte_slice_from_buffer(buf);
            var h_slc = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!h_raw.equals(h_buf)) { return 0; }
            if (!h_raw.equals(h_slc)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, CrossValidateMd5AllVariants) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var h_raw = hashing_md5(data.data(), data.length());
            var h_buf = hashing_md5_buffer(buf);
            var view = byte_slice_from_buffer(buf);
            var h_slc = hashing_md5_slice(view);
            byte_slice_release(view);
            if (!h_raw.equals(h_buf)) { return 0; }
            if (!h_raw.equals(h_slc)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, CrossValidateSha1AllVariants) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var h_raw = hashing_sha1(data.data(), data.length());
            var h_buf = hashing_sha1_buffer(buf);
            var view = byte_slice_from_buffer(buf);
            var h_slc = hashing_sha1_slice(view);
            byte_slice_release(view);
            if (!h_raw.equals(h_buf)) { return 0; }
            if (!h_raw.equals(h_slc)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, CrossValidateCrc32AllVariants) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var buf = Buffer.fromBytes("hello", 5);
            var c_raw = hashing_crc32(data.data(), data.length());
            var c_buf = hashing_crc32_buffer(buf);
            var view = byte_slice_from_buffer(buf);
            var c_slc = hashing_crc32_slice(view);
            byte_slice_release(view);
            if (c_raw != c_buf) { return 0; }
            if (c_raw != c_slc) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

// ─── Edge cases ──────────────────────────────────────────────────────────────

TEST_F(HashingCLIIntegrationTest, Sha256SingleCharacter) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var hash = hashing_sha256("a".data(), 1);
            if (hash.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256BinaryData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 main() {
            var buf = new Buffer();
            buf.append("ABCD", 4);
            buf.setByte(0, 0);
            buf.setByte(1, 1);
            buf.setByte(2, 2);
            buf.setByte(3, 255);
            var h1 = hashing_sha256_buffer(buf);
            var view = byte_slice_from_buffer(buf);
            var h2 = hashing_sha256_slice(view);
            byte_slice_release(view);
            if (!h1.equals(h2)) { return 0; }
            if (h1.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, MultipleHashesSameData) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "hello";
            var sha = hashing_sha256(data.data(), data.length());
            var md = hashing_md5(data.data(), data.length());
            var s1 = hashing_sha1(data.data(), data.length());
            if (sha.length() != 64) { return 0; }
            if (md.length() != 32) { return 0; }
            if (s1.length() != 40) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, LongerInputHashConsistency) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "The quick brown fox jumps over the lazy dog";
            var h1 = hashing_sha256(data.data(), data.length());
            var h2 = hashing_sha256(data.data(), data.length());
            if (!h1.equals(h2)) { return 0; }
            if (h1.length() != 64) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
}

TEST_F(HashingCLIIntegrationTest, Sha256FileMatchesHashOfContent) {
    const std::string content = "The quick brown fox jumps over the lazy dog";
    const std::string tmpPath = createTempFile(content);
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.hashing;
        func :int64 main() {
            var data = "The quick brown fox jumps over the lazy dog";
            var from_data = hashing_sha256(data.data(), data.length());
            var from_file = hashing_sha256_file(")") + tmpPath + R"(");
            if (!from_data.equals(from_file)) { return 0; }
            return 1;
        }
    )");
    ASSERT_EQ(result.exitCode, 0) << result.output;
    EXPECT_NE(result.output.find("1"), std::string::npos);
    std::remove(tmpPath.c_str());
}
