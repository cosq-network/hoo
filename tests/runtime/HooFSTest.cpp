#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "runtime/lib/fs/hoo_fs.h"

#ifdef _WIN32
#include <process.h>
#define hoo_getpid _getpid
#else
#include <unistd.h>
#define hoo_getpid getpid
#endif

using namespace hoo::fs;

class HooFSTest : public ::testing::Test {
protected:
    void SetUp() override {
        tempDir_ = hoo::fs::tempDir();
        ASSERT_FALSE(tempDir_.empty());

        static std::atomic<uint64_t> counter{0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        testDir = tempDir_ + "/hoo_fs_test_"
            + std::to_string(static_cast<long long>(hoo_getpid())) + "_"
            + std::to_string(static_cast<long long>(nanos)) + "_"
            + std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
        bool ok = Directory(testDir).createTree();
        ASSERT_TRUE(ok);
    }

    void TearDown() override {
        Directory(testDir).remove();
    }

    std::string mkpath(const std::string& name) const {
        return testDir + "/" + name;
    }

    std::string tempDir_;
    std::string testDir;
};

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, Path_Str) {
    Path p("foo/bar.txt");
    EXPECT_EQ(p.str(), "foo/bar.txt");

    Path empty("");
    EXPECT_EQ(empty.str(), "");
}

TEST_F(HooFSTest, File_Path) {
    File f("some/file.txt");
    EXPECT_EQ(f.path(), "some/file.txt");
}

TEST_F(HooFSTest, Directory_Path) {
    Directory d("some/dir");
    EXPECT_EQ(d.path(), "some/dir");
}

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, TempDir) {
    std::string tmp = hoo::fs::tempDir();
    EXPECT_FALSE(tmp.empty());
}

TEST_F(HooFSTest, CreateTempFile) {
    std::string tmpPath = hoo::fs::createTempFile("hoofstest");
    EXPECT_FALSE(tmpPath.empty());
    EXPECT_TRUE(File(tmpPath).exists());

    File(tmpPath).remove();
}

TEST_F(HooFSTest, CopyFile) {
    std::string src = mkpath("copy_src.txt");
    std::string dst = mkpath("copy_dst.txt");
    const std::string content = "copy me";

    EXPECT_TRUE(File(src).writeText(content));
    EXPECT_TRUE(hoo::fs::copyFile(src, dst));

    std::string readback = File(dst).readText();
    EXPECT_EQ(readback, content);

    File(src).remove();
    File(dst).remove();
}

// ---------------------------------------------------------------------------
// Path
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, Path_Dirname) {
    EXPECT_EQ(Path("/foo/bar/file.txt").dirname(), "/foo/bar");
    EXPECT_EQ(Path("file.txt").dirname(), ".");
}

TEST_F(HooFSTest, Path_Basename) {
    EXPECT_EQ(Path("/foo/bar/file.txt").basename(), "file.txt");
    EXPECT_EQ(Path("/foo/bar/").basename(), "bar");
}

TEST_F(HooFSTest, Path_Extension) {
    EXPECT_EQ(Path("file.txt").extension(), ".txt");
    EXPECT_EQ(Path("file").extension(), "");
    EXPECT_EQ(Path(".hidden").extension(), "");
}

TEST_F(HooFSTest, Path_Stem) {
    EXPECT_EQ(Path("file.txt").stem(), "file");
    EXPECT_EQ(Path("archive.tar.gz").stem(), "archive.tar");
}

TEST_F(HooFSTest, Path_Root) {
    EXPECT_EQ(Path("/foo/bar").root(), "/");
}

TEST_F(HooFSTest, Path_Join) {
    std::string joined = hoo::fs::join("a", "b");
    EXPECT_FALSE(joined.empty());
    EXPECT_GT(joined.size(), 1);
}

TEST_F(HooFSTest, Path_JoinMulti) {
    std::vector<std::string> parts = {"a", "b", "c"};
    std::string joined = hoo::fs::joinMulti(parts);
    EXPECT_FALSE(joined.empty());
    EXPECT_GT(joined.size(), 1);
}

TEST_F(HooFSTest, Path_Normalized) {
    std::string norm = Path("/foo/../bar/./baz").normalized().str();
    EXPECT_FALSE(norm.empty());
    EXPECT_NE(norm.find("bar"), std::string::npos);
    EXPECT_NE(norm.find("baz"), std::string::npos);
}

TEST_F(HooFSTest, Path_Absolute) {
    std::string abs = Path("relative/path").absolute().str();
    EXPECT_FALSE(abs.empty());
    EXPECT_GT(abs.size(), strlen("relative/path"));
}

TEST_F(HooFSTest, Path_Relative) {
    std::string rel = hoo::fs::relative("/foo/bar/baz", "/foo/bar");
    EXPECT_EQ(rel, "baz");
}

TEST_F(HooFSTest, Path_IsAbsolute) {
    EXPECT_FALSE(Path("foo").isAbsolute());
    EXPECT_FALSE(Path("").isAbsolute());
#ifdef _WIN32
    EXPECT_TRUE(Path("C:\\foo").isAbsolute());
#else
    EXPECT_TRUE(Path("/foo").isAbsolute());
#endif
}

TEST_F(HooFSTest, Path_IsRelative) {
    EXPECT_TRUE(Path("foo").isRelative());
    EXPECT_TRUE(Path("").isRelative());
#ifdef _WIN32
    EXPECT_FALSE(Path("C:\\foo").isRelative());
#else
    EXPECT_FALSE(Path("/foo").isRelative());
#endif
}

TEST_F(HooFSTest, Path_HasExtension) {
    EXPECT_TRUE(Path("file.txt").hasExtension());
    EXPECT_FALSE(Path("file").hasExtension());
}

TEST_F(HooFSTest, Path_HasRoot) {
    EXPECT_FALSE(Path("foo").hasRoot());
#ifdef _WIN32
    EXPECT_TRUE(Path("C:\\foo").hasRoot());
    EXPECT_TRUE(Path("\\foo").hasRoot());
#else
    EXPECT_TRUE(Path("/foo").hasRoot());
#endif
}

TEST_F(HooFSTest, Path_Split) {
    auto parts = Path("foo/bar/baz").split();
    ASSERT_EQ(parts.size(), size_t{3});
    EXPECT_EQ(parts[0], "foo");
    EXPECT_EQ(parts[1], "bar");
    EXPECT_EQ(parts[2], "baz");
}

TEST_F(HooFSTest, Path_Empty) {
    Path empty("");
    EXPECT_EQ(empty.str(), "");
    EXPECT_EQ(empty.dirname(), ".");
    EXPECT_EQ(empty.basename(), "");
    EXPECT_EQ(empty.extension(), "");
    EXPECT_EQ(empty.stem(), "");
    EXPECT_FALSE(empty.hasExtension());
    EXPECT_FALSE(empty.hasRoot());
    EXPECT_TRUE(empty.split().empty());
    EXPECT_FALSE(empty.isAbsolute());
    EXPECT_TRUE(empty.isRelative());
}

TEST_F(HooFSTest, Path_Separator) {
    char sep = hoo::fs::separator();
#ifdef _WIN32
    EXPECT_EQ(sep, '\\');
#else
    EXPECT_EQ(sep, '/');
#endif
}

TEST_F(HooFSTest, Path_ListSeparator) {
    char sep = hoo::fs::listSeparator();
#ifdef _WIN32
    EXPECT_EQ(sep, ';');
#else
    EXPECT_EQ(sep, ':');
#endif
}

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, File_Exists) {
    std::string path = mkpath("exists_test.txt");
    EXPECT_FALSE(File(path).exists());

    EXPECT_TRUE(File(path).writeText("hello"));
    EXPECT_TRUE(File(path).exists());

    EXPECT_TRUE(File(path).remove());
    EXPECT_FALSE(File(path).exists());
}

TEST_F(HooFSTest, File_IsFile) {
    std::string filePath = mkpath("is_file_test.txt");
    std::string dirPath = mkpath("is_file_dir");

    EXPECT_TRUE(File(filePath).writeText("data"));
    EXPECT_TRUE(Directory(dirPath).create());

    EXPECT_TRUE(File(filePath).isFile());
    EXPECT_FALSE(File(dirPath).isFile());

    File(filePath).remove();
    Directory(dirPath).remove();
}

TEST_F(HooFSTest, File_ReadWriteText) {
    std::string path = mkpath("readwrite.txt");
    const std::string content = "Hello, HooFS!";

    EXPECT_TRUE(File(path).writeText(content));

    std::string readback = File(path).readText();
    EXPECT_EQ(readback, content);

    File(path).remove();
}

TEST_F(HooFSTest, File_AppendText) {
    std::string path = mkpath("append.txt");

    EXPECT_TRUE(File(path).writeText("Hello"));
    EXPECT_TRUE(File(path).appendText(" World"));

    std::string readback = File(path).readText();
    EXPECT_EQ(readback, "Hello World");

    File(path).remove();
}

TEST_F(HooFSTest, File_Size) {
    std::string path = mkpath("size_test.txt");
    const std::string content = "1234567890";

    EXPECT_TRUE(File(path).writeText(content));

    int64_t sz = File(path).size();
    EXPECT_EQ(sz, static_cast<int64_t>(content.size()));

    File(path).remove();
}

TEST_F(HooFSTest, File_LastModified) {
    std::string path = mkpath("modified_test.txt");

    EXPECT_TRUE(File(path).writeText("data"));

    int64_t mtime = File(path).lastModified();
    EXPECT_GE(mtime, 0);

    File(path).remove();
}

TEST_F(HooFSTest, File_Remove) {
    std::string path = mkpath("delete_test.txt");

    EXPECT_TRUE(File(path).writeText("to be deleted"));
    EXPECT_TRUE(File(path).exists());

    EXPECT_TRUE(File(path).remove());
    EXPECT_FALSE(File(path).exists());
}

TEST_F(HooFSTest, File_Rename) {
    std::string oldPath = mkpath("rename_old.txt");
    std::string newPath = mkpath("rename_new.txt");

    EXPECT_TRUE(File(oldPath).writeText("renamed content"));
    EXPECT_TRUE(File(oldPath).rename(newPath));

    EXPECT_FALSE(File(oldPath).exists());
    EXPECT_TRUE(File(newPath).exists());

    std::string readback = File(newPath).readText();
    EXPECT_EQ(readback, "renamed content");

    File(newPath).remove();
}

TEST_F(HooFSTest, File_RenameUpdatesPath) {
    std::string oldPath = mkpath("rename_path_old.txt");
    std::string newPath = mkpath("rename_path_new.txt");

    File f(oldPath);
    EXPECT_TRUE(f.writeText("content"));
    EXPECT_TRUE(f.rename(newPath));

    // File object's internal path should be updated
    EXPECT_EQ(f.path(), newPath);

    EXPECT_TRUE(f.exists());
    EXPECT_FALSE(File(oldPath).exists());

    f.remove();
}

// ---------------------------------------------------------------------------
// Directory
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, Directory_IsDirectory) {
    std::string filePath = mkpath("is_dir_file.txt");
    std::string dirPath = mkpath("is_dir_dir");

    EXPECT_TRUE(File(filePath).writeText("data"));
    EXPECT_TRUE(Directory(dirPath).create());

    EXPECT_TRUE(Directory(dirPath).isDirectory());
    EXPECT_FALSE(Directory(filePath).isDirectory());

    File(filePath).remove();
    Directory(dirPath).remove();
}

TEST_F(HooFSTest, Directory_CreateTree) {
    std::string nested = testDir + "/a/b/c/d";

    EXPECT_TRUE(Directory(nested).createTree());
    EXPECT_TRUE(Directory(nested).isDirectory());

    Directory(nested).remove();
    Directory(testDir + "/a/b/c").remove();
    Directory(testDir + "/a/b").remove();
    Directory(testDir + "/a").remove();
}

TEST_F(HooFSTest, Directory_Exists) {
    std::string dirPath = mkpath("exists_dir");
    std::string otherDir = mkpath("exists_dir_other");

    EXPECT_FALSE(Directory(dirPath).exists());

    EXPECT_TRUE(Directory(dirPath).create());
    EXPECT_TRUE(Directory(dirPath).exists());

    EXPECT_TRUE(Directory(otherDir).createTree());
    EXPECT_TRUE(Directory(otherDir).exists());

    EXPECT_TRUE(Directory(dirPath).remove());
    EXPECT_FALSE(Directory(dirPath).exists());

    EXPECT_TRUE(Directory(otherDir).remove());
    EXPECT_FALSE(Directory(otherDir).exists());
}

TEST_F(HooFSTest, Directory_List) {
    std::string file1 = mkpath("list_a.txt");
    std::string file2 = mkpath("list_b.txt");

    File(file1).writeText("aaa");
    File(file2).writeText("bbb");

    std::vector<std::string> entries = Directory(testDir).list();
    ASSERT_GE(entries.size(), size_t{2});

    bool foundA = false, foundB = false;
    for (const auto& e : entries) {
        if (e == "list_a.txt") foundA = true;
        if (e == "list_b.txt") foundB = true;
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    File(file1).remove();
    File(file2).remove();
}

// ---------------------------------------------------------------------------
// Binary I/O
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, File_BinaryRoundTrip) {
    std::string path = mkpath("binary.bin");
    std::vector<uint8_t> data = {0x00, 0xFF, 0xAB, 0xCD, 0x12, 0x34};

    EXPECT_TRUE(File(path).writeBytes(data));

    std::vector<uint8_t> readback;
    EXPECT_TRUE(File(path).readBytes(readback));

    ASSERT_EQ(readback.size(), data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(readback[i], data[i]) << "byte mismatch at index " << i;
    }

    File(path).remove();
}

// ---------------------------------------------------------------------------
// C-ABI Bridge Compatibility (ensure JIT / FFI still works)
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, CAbi_Bridge_Exists) {
    std::string path = mkpath("cabi_exists.txt");
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);

    EXPECT_TRUE(File(path).writeText("hi"));
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 1);

    File(path).remove();
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);
}

TEST_F(HooFSTest, CAbi_Bridge_TempDir) {
    char* tmp = hoo_fs_temp_dir();
    ASSERT_NE(tmp, nullptr);
    EXPECT_GT(strlen(tmp), 0);
    hoo_fs_free_string(tmp);
}

TEST_F(HooFSTest, CAbi_Bridge_ReadWriteText) {
    std::string path = mkpath("cabi_rw.txt");
    const char* content = "C-ABI bridge test";

    int64_t ok = hoo_fs_write_text(path.c_str(), content);
    ASSERT_EQ(ok, 1);

    char* readback = hoo_fs_read_text(path.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, content);
    hoo_fs_free_string(readback);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_ListDir) {
    std::string file1 = mkpath("cabi_a.txt");
    std::string file2 = mkpath("cabi_b.txt");

    hoo_fs_write_text(file1.c_str(), "a");
    hoo_fs_write_text(file2.c_str(), "b");

    int64_t count = 0;
    char** entries = hoo_fs_list_dir(testDir.c_str(), &count);
    ASSERT_NE(entries, nullptr);
    ASSERT_GE(count, 2);

    bool foundA = false, foundB = false;
    for (int64_t i = 0; i < count; i++) {
        if (strcmp(entries[i], "cabi_a.txt") == 0) foundA = true;
        if (strcmp(entries[i], "cabi_b.txt") == 0) foundB = true;
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    hoo_fs_free_list(entries, count);
    hoo_fs_delete(file1.c_str());
    hoo_fs_delete(file2.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_CreateTempFile) {
    char* tmpPath = hoo_fs_create_temp_file("cabitest");
    ASSERT_NE(tmpPath, nullptr);
    EXPECT_GT(strlen(tmpPath), 0);
    EXPECT_EQ(hoo_fs_exists(tmpPath), 1);

    hoo_fs_delete(tmpPath);
    hoo_fs_free_string(tmpPath);
}

TEST_F(HooFSTest, CAbi_Bridge_Move) {
    std::string src = mkpath("cabi_move_src.txt");
    std::string dst = mkpath("cabi_move_dst.txt");

    ASSERT_EQ(hoo_fs_write_text(src.c_str(), "moveme"), 1);
    EXPECT_EQ(hoo_fs_move(src.c_str(), dst.c_str()), 1);
    EXPECT_EQ(hoo_fs_exists(src.c_str()), 0);
    EXPECT_EQ(hoo_fs_exists(dst.c_str()), 1);

    char* readback = hoo_fs_read_text(dst.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, "moveme");
    hoo_fs_free_string(readback);

    // Moving a nonexistent source must fail.
    EXPECT_EQ(hoo_fs_move(src.c_str(), dst.c_str()), 0);

    hoo_fs_delete(dst.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_Remove) {
    std::string path = mkpath("cabi_remove.txt");

    ASSERT_EQ(hoo_fs_write_text(path.c_str(), "gone"), 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 1);
    EXPECT_EQ(hoo_fs_remove(path.c_str()), 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);

    // Removing a nonexistent file must fail.
    EXPECT_EQ(hoo_fs_remove(path.c_str()), 0);
}

TEST_F(HooFSTest, CAbi_Bridge_ReadTextEmptyVsMissing) {
    std::string path = mkpath("cabi_empty.txt");

    // Existing but empty file returns an empty string, not null.
    ASSERT_EQ(hoo_fs_write_text(path.c_str(), ""), 1);
    char* empty = hoo_fs_read_text(path.c_str());
    ASSERT_NE(empty, nullptr);
    EXPECT_STREQ(empty, "");
    hoo_fs_free_string(empty);

    // Missing file returns null.
    std::string missing = mkpath("cabi_missing.txt");
    EXPECT_EQ(hoo_fs_read_text(missing.c_str()), nullptr);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_ReadBytesBufferEmptyVsMissing) {
    std::string path = mkpath("cabi_empty.bin");

    // Existing but empty file returns a zero-length buffer, not null.
    ASSERT_EQ(hoo_fs_write_text(path.c_str(), ""), 1);
    HooBuffer empty = hoo_fs_read_bytes_buffer(path.c_str());
    ASSERT_NE(empty, nullptr);
    EXPECT_EQ(hoo_buffer_length(empty), 0);
    hoo_buffer_release(empty);

    // Missing file returns null.
    std::string missing = mkpath("cabi_missing.bin");
    EXPECT_EQ(hoo_fs_read_bytes_buffer(missing.c_str()), nullptr);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_WriteBytesAndReadBack) {
    std::string path = mkpath("cabi_bytes.bin");
    const uint8_t data[] = {0x00, 0xFF, 0xAB, 0xCD, 0x01, 0x02};

    EXPECT_EQ(hoo_fs_write_bytes(path.c_str(), data, 6), 1);

    uint8_t* readback = nullptr;
    int64_t len = 0;
    EXPECT_EQ(hoo_fs_read_bytes(path.c_str(), &readback, &len), 1);
    ASSERT_NE(readback, nullptr);
    ASSERT_EQ(len, 6);
    for (int64_t i = 0; i < len; ++i) {
        EXPECT_EQ(readback[i], data[i]) << "byte mismatch at index " << i;
    }
    std::free(readback);

    // Writing to an unwritable path must fail (returns 0).
    EXPECT_EQ(hoo_fs_write_bytes("/nonexistent-dir/hoo/x.bin", data, 6), 0);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_WriteBytesBuffer) {
    std::string path = mkpath("cabi_buf.bin");
    const char* content = "buffer content";

    HooBuffer buf = hoo_buffer_from_bytes(reinterpret_cast<const uint8_t*>(content), strlen(content));
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_fs_write_bytes_buffer(path.c_str(), buf), 1);
    hoo_buffer_release(buf);

    char* readback = hoo_fs_read_text(path.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, content);
    hoo_fs_free_string(readback);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, CAbi_Bridge_CreateTempDir) {
    char* dir = hoo_fs_create_temp_dir();
    ASSERT_NE(dir, nullptr);
    EXPECT_GT(strlen(dir), 0);
    EXPECT_EQ(hoo_fs_exists(dir), 1);
    EXPECT_EQ(hoo_fs_is_dir(dir), 1);

    EXPECT_EQ(hoo_fs_rmdir(dir), 1);
    EXPECT_EQ(hoo_fs_exists(dir), 0);
    hoo_fs_free_string(dir);
}

TEST_F(HooFSTest, CAbi_Bridge_CurrentDir) {
    char* cwd = hoo_fs_current_dir();
    ASSERT_NE(cwd, nullptr);
    EXPECT_GT(strlen(cwd), 0);
    hoo_fs_free_string(cwd);
}

TEST_F(HooFSTest, CAbi_Bridge_CurrentExeDir) {
    char* exeDir = hoo_fs_current_exe_dir();
    ASSERT_NE(exeDir, nullptr);
    EXPECT_GT(strlen(exeDir), 0);
    EXPECT_EQ(exeDir[0], '/');
    hoo_fs_free_string(exeDir);
}
