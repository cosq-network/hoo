#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <process.h>
#define hoo_jit_getpid _getpid
#else
#include <unistd.h>
#define hoo_jit_getpid getpid
#endif

using namespace hooc;

class HooFsJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
    std::string testDir;
    int64_t result_ = 0;

    void SetUp() override {
        static std::atomic<uint64_t> counter{0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        std::string dir = "hoo_fs_jit_"
            + std::to_string(static_cast<long long>(hoo_jit_getpid())) + "_"
            + std::to_string(static_cast<long long>(nanos)) + "_"
            + std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
        testDir = (std::filesystem::temp_directory_path() / dir).string();
#ifdef _WIN32
        std::replace(testDir.begin(), testDir.end(), '\\', '/');
#endif
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }

    // Forward slashes keep the embedded Hoo literal escaping-free on all platforms.
    std::string p(const std::string& name) const {
        return testDir + "/" + name;
    }

    std::string ioImports() const {
        return "import hoo.io;\n";
    }

    void runCode(const std::string& source) {
        if (!jit.loadSourceCode("test", source)) {
            FAIL() << jit.getLastError();
            return;
        }
        result_ = jit.run("_F_M_test_E_test_i8");
    }
};

TEST_F(HooFsJitTest, WriteTextExistsReadRoundTrip) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var p = ")" + p("roundtrip.txt") + R"(";
            if (fs_write_text(p, "hello world") == 0) { return 0; }
            if (fs_exists(p) == 0) { return 0; }
            if (fs_size(p) != 11) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("hello world")) { return 0; }
            if (fs_delete(p) == 0) { return 0; }
            if (fs_exists(p) != 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ReadTextMissingReturnsNull) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var c = fs_read_text("/nonexistent_hoo_dir_xyz/file.txt");
            if (c) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ReadTextEmptyFileReturnsEmptyString) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var p = ")" + p("empty.txt") + R"(";
            if (fs_write_text(p, "") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (c.length() != 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, AppendText) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var p = ")" + p("append.txt") + R"(";
            if (fs_write_text(p, "Hello") == 0) { return 0; }
            if (fs_append_text(p, " World") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("Hello World")) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, MoveAndRename) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var a = ")" + p("move_a.txt") + R"(";
            var b = ")" + p("move_b.txt") + R"(";
            if (fs_write_text(a, "data") == 0) { return 0; }
            if (fs_move(a, b) == 0) { return 0; }
            if (fs_exists(a) != 0) { return 0; }
            if (fs_exists(b) == 0) { return 0; }
            if (fs_rename(b, a) == 0) { return 0; }
            if (fs_exists(b) != 0) { return 0; }
            if (fs_exists(a) == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, Copy) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var src = ")" + p("copy_src.txt") + R"(";
            var dst = ")" + p("copy_dst.txt") + R"(";
            var dst2 = ")" + p("copy_dst2.txt") + R"(";
            if (fs_write_text(src, "copyme") == 0) { return 0; }
            if (fs_copy(src, dst) == 0) { return 0; }
            if (fs_exists(dst) == 0) { return 0; }
            if (fs_copy("/nonexistent_hoo_dir_xyz/src.txt", dst2) != 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, Remove) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var p = ")" + p("remove.txt") + R"(";
            if (fs_write_text(p, "x") == 0) { return 0; }
            if (fs_remove(p) == 0) { return 0; }
            if (fs_exists(p) != 0) { return 0; }
            if (fs_remove(p) != 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, MkdirMkdirsRmdir) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var d1 = ")" + p("dir_a") + R"(";
            var d2 = ")" + p("dir_b") + R"(";
            if (fs_mkdir(d1) == 0) { return 0; }
            if (fs_is_dir(d1) == 0) { return 0; }
            if (fs_mkdirs(d2 + "/a/b/c") == 0) { return 0; }
            if (fs_is_dir(d2 + "/a/b/c") == 0) { return 0; }
            if (fs_mkdir("/nonexistent_hoo_dir_xyz/child") != 0) { return 0; }
            if (fs_rmdir(d1) == 0) { return 0; }
            if (fs_rmdir(d2 + "/a/b/c") == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, LastModifiedIsInt64Timestamp) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var p = ")" + p("mtime.txt") + R"(";
            if (fs_write_text(p, "x") == 0) { return 0; }
            var m = fs_last_modified(p);
            if (m < 1500000000) { return 0; }
            if (m > 5000000000) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, LastModifiedMissingReturnsNegative) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var m = fs_last_modified("/nonexistent_hoo_dir_xyz/file.txt");
            if (m != -1) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ListDirReturnsEntries) {
    const std::string source = ioImports() + R"(
        import hoo;
        func :int64 test() {
            var a = ")" + p("list_a.txt") + R"(";
            var b = ")" + p("list_b.txt") + R"(";
            if (fs_write_text(a, "a") == 0) { return 0; }
            if (fs_write_text(b, "b") == 0) { return 0; }
            var dir = ")" + testDir + R"(";
            var entries = fs_list_dir(dir);
            if (!entries) { return 0; }
            if (entries.length() < 2) { return 0; }
            var foundA = 0;
            var foundB = 0;
            for i in 0..entries.length() {
                var name: string = entries.getString(i);
                if (name.equals("list_a.txt")) { foundA = 1; }
                if (name.equals("list_b.txt")) { foundB = 1; }
            }
            if (foundA == 0) { return 0; }
            if (foundB == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, TempDirAndCreateTempFile) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var t = fs_temp_dir();
            if (!t) { return 0; }
            if (t.length() == 0) { return 0; }
            var tf = fs_create_temp_file("hoofsjit");
            if (!tf) { return 0; }
            if (fs_exists(tf) == 0) { return 0; }
            if (fs_delete(tf) == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, CreateTempDir) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var td = fs_create_temp_dir();
            if (!td) { return 0; }
            if (fs_is_dir(td) == 0) { return 0; }
            if (fs_rmdir(td) == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, CurrentDirAndCurrentExeDir) {
    const std::string source = ioImports() + R"(
        func :int64 test() {
            var cwd = fs_current_dir();
            if (!cwd) { return 0; }
            if (cwd.length() == 0) { return 0; }
            var exeDir = fs_current_exe_dir();
            if (!exeDir) { return 0; }
            if (exeDir.length() == 0) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ReadBytesWriteBytesBuffer) {
    const std::string source = ioImports() + R"(
        import hoo.buffer;
        func :int64 test() {
            var p = ")" + p("bytes.txt") + R"(";
            var p2 = ")" + p("bytes_copy.txt") + R"(";
            if (fs_write_text(p, "hello") == 0) { return 0; }
            var b = fs_read_bytes(p);
            if (!b) { return 0; }
            if (b.length() != 5) { return 0; }
            if (fs_write_bytes(p2, b) == 0) { return 0; }
            var b2 = fs_read_bytes(p2);
            if (!b2) { return 0; }
            if (b2.length() != 5) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ReadBytesBufferWriteBytesBuffer) {
    const std::string source = ioImports() + R"(
        import hoo.buffer;
        func :int64 test() {
            var p = ")" + p("bytes_buffer.txt") + R"(";
            var b = new Buffer();
            b.append("hello", 5);
            if (fs_write_bytes_buffer(p, b) == 0) { return 0; }
            var r = fs_read_bytes_buffer(p);
            if (!r) { return 0; }
            if (r.length() != 5) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}

TEST_F(HooFsJitTest, ReadBytesBufferMissingReturnsFallback) {
    const std::string source = ioImports() + R"(
        import hoo.buffer;
        func :int64 test() {
            var p = ")" + p("does_not_exist.bin") + R"(";
            var fallback = new Buffer();
            fallback.append("fb", 2);
            var r = fs_read_bytes_buffer(p, fallback);
            if (!r) { return 0; }
            if (r.length() != 2) { return 0; }
            return 1;
        }
    )";
    runCode(source);
    EXPECT_EQ(result_, 1);
}
