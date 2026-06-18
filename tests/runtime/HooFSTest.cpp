#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "runtime/lib/hoo_fs.h"

using namespace hoo::fs;

class HooFSTest : public ::testing::Test {
protected:
    void SetUp() override {
        tempDir = Path::getTempDir();
        ASSERT_FALSE(tempDir.empty());

        testDir = tempDir + "/hoo_fs_test_" + std::to_string(time(nullptr));
        bool ok = Directory::createTree(testDir);
        ASSERT_TRUE(ok);
    }

    void TearDown() override {
        Directory::remove(testDir);
    }

    std::string mkpath(const std::string& name) const {
        return testDir + "/" + name;
    }

    std::string tempDir;
    std::string testDir;
};

// ---------------------------------------------------------------------------
// Path
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, Path_GetTempDir) {
    std::string tmp = Path::getTempDir();
    EXPECT_FALSE(tmp.empty());
}

TEST_F(HooFSTest, Path_CreateTempFile) {
    std::string tmpPath = Path::createTempFile("hoofstest");
    EXPECT_FALSE(tmpPath.empty());
    EXPECT_TRUE(File::exists(tmpPath));

    File::remove(tmpPath);
}

TEST_F(HooFSTest, Path_Dirname) {
    EXPECT_EQ(Path::dirname("/foo/bar/file.txt"), "/foo/bar");
    EXPECT_EQ(Path::dirname("file.txt"), ".");
}

TEST_F(HooFSTest, Path_Basename) {
    EXPECT_EQ(Path::basename("/foo/bar/file.txt"), "file.txt");
    EXPECT_EQ(Path::basename("/foo/bar/"), "bar");
}

TEST_F(HooFSTest, Path_Extension) {
    EXPECT_EQ(Path::extension("file.txt"), ".txt");
    EXPECT_EQ(Path::extension("file"), "");
    EXPECT_EQ(Path::extension(".hidden"), "");
}

TEST_F(HooFSTest, Path_Stem) {
    EXPECT_EQ(Path::stem("file.txt"), "file");
    EXPECT_EQ(Path::stem("archive.tar.gz"), "archive.tar");
}

TEST_F(HooFSTest, Path_Root) {
    EXPECT_EQ(Path::root("/foo/bar"), "/");
}

TEST_F(HooFSTest, Path_Join) {
    std::string joined = Path::join("a", "b");
    EXPECT_FALSE(joined.empty());
    EXPECT_GT(joined.size(), 1);
}

TEST_F(HooFSTest, Path_JoinMulti) {
    std::vector<std::string> parts = {"a", "b", "c"};
    std::string joined = Path::joinMulti(parts);
    EXPECT_FALSE(joined.empty());
    EXPECT_GT(joined.size(), 1);
}

TEST_F(HooFSTest, Path_Normalize) {
    std::string norm = Path::normalize("/foo/../bar/./baz");
    EXPECT_FALSE(norm.empty());
    EXPECT_NE(norm.find("bar"), std::string::npos);
    EXPECT_NE(norm.find("baz"), std::string::npos);
}

TEST_F(HooFSTest, Path_Absolute) {
    std::string abs = Path::absolute("relative/path");
    EXPECT_FALSE(abs.empty());
    EXPECT_GT(abs.size(), strlen("relative/path"));
}

TEST_F(HooFSTest, Path_Relative) {
    std::string rel = Path::relative("/foo/bar/baz", "/foo/bar");
    EXPECT_EQ(rel, "baz");
}

TEST_F(HooFSTest, Path_IsAbsolute) {
    EXPECT_FALSE(Path::isAbsolute("foo"));
    EXPECT_FALSE(Path::isAbsolute(""));
#ifdef _WIN32
    EXPECT_TRUE(Path::isAbsolute("C:\\foo"));
#else
    EXPECT_TRUE(Path::isAbsolute("/foo"));
#endif
}

TEST_F(HooFSTest, Path_IsRelative) {
    EXPECT_TRUE(Path::isRelative("foo"));
    EXPECT_TRUE(Path::isRelative(""));
#ifdef _WIN32
    EXPECT_FALSE(Path::isRelative("C:\\foo"));
#else
    EXPECT_FALSE(Path::isRelative("/foo"));
#endif
}

TEST_F(HooFSTest, Path_HasExtension) {
    EXPECT_TRUE(Path::hasExtension("file.txt"));
    EXPECT_FALSE(Path::hasExtension("file"));
}

TEST_F(HooFSTest, Path_HasRoot) {
    EXPECT_FALSE(Path::hasRoot("foo"));
#ifdef _WIN32
    EXPECT_TRUE(Path::hasRoot("C:\\foo"));
    EXPECT_TRUE(Path::hasRoot("\\foo"));
#else
    EXPECT_TRUE(Path::hasRoot("/foo"));
#endif
}

TEST_F(HooFSTest, Path_Split) {
    auto parts = Path::split("foo/bar/baz");
    ASSERT_EQ(parts.size(), size_t{3});
    EXPECT_EQ(parts[0], "foo");
    EXPECT_EQ(parts[1], "bar");
    EXPECT_EQ(parts[2], "baz");
}

TEST_F(HooFSTest, Path_Separator) {
    char sep = Path::separator();
#ifdef _WIN32
    EXPECT_EQ(sep, '\\');
#else
    EXPECT_EQ(sep, '/');
#endif
}

TEST_F(HooFSTest, Path_ListSeparator) {
    char sep = Path::listSeparator();
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
    EXPECT_FALSE(File::exists(path));

    EXPECT_TRUE(File::writeText(path, "hello"));
    EXPECT_TRUE(File::exists(path));

    EXPECT_TRUE(File::remove(path));
    EXPECT_FALSE(File::exists(path));
}

TEST_F(HooFSTest, File_IsFile) {
    std::string filePath = mkpath("is_file_test.txt");
    std::string dirPath = mkpath("is_file_dir");

    EXPECT_TRUE(File::writeText(filePath, "data"));
    EXPECT_TRUE(Directory::create(dirPath));

    EXPECT_TRUE(File::isFile(filePath));
    EXPECT_FALSE(File::isFile(dirPath));

    File::remove(filePath);
    Directory::remove(dirPath);
}

TEST_F(HooFSTest, File_ReadWriteText) {
    std::string path = mkpath("readwrite.txt");
    const std::string content = "Hello, HooFS!";

    EXPECT_TRUE(File::writeText(path, content));

    std::string readback = File::readText(path);
    EXPECT_EQ(readback, content);

    File::remove(path);
}

TEST_F(HooFSTest, File_AppendText) {
    std::string path = mkpath("append.txt");

    EXPECT_TRUE(File::writeText(path, "Hello"));
    EXPECT_TRUE(File::appendText(path, " World"));

    std::string readback = File::readText(path);
    EXPECT_EQ(readback, "Hello World");

    File::remove(path);
}

TEST_F(HooFSTest, File_Size) {
    std::string path = mkpath("size_test.txt");
    const std::string content = "1234567890";

    EXPECT_TRUE(File::writeText(path, content));

    int64_t sz = File::size(path);
    EXPECT_EQ(sz, static_cast<int64_t>(content.size()));

    File::remove(path);
}

TEST_F(HooFSTest, File_LastModified) {
    std::string path = mkpath("modified_test.txt");

    EXPECT_TRUE(File::writeText(path, "data"));

    int64_t mtime = File::lastModified(path);
    EXPECT_GE(mtime, 0);

    File::remove(path);
}

TEST_F(HooFSTest, File_Remove) {
    std::string path = mkpath("delete_test.txt");

    EXPECT_TRUE(File::writeText(path, "to be deleted"));
    EXPECT_TRUE(File::exists(path));

    EXPECT_TRUE(File::remove(path));
    EXPECT_FALSE(File::exists(path));
}

TEST_F(HooFSTest, File_Copy) {
    std::string src = mkpath("copy_src.txt");
    std::string dst = mkpath("copy_dst.txt");
    const std::string content = "copy me";

    EXPECT_TRUE(File::writeText(src, content));
    EXPECT_TRUE(File::copy(src, dst));

    std::string readback = File::readText(dst);
    EXPECT_EQ(readback, content);

    File::remove(src);
    File::remove(dst);
}

TEST_F(HooFSTest, File_Rename) {
    std::string oldPath = mkpath("rename_old.txt");
    std::string newPath = mkpath("rename_new.txt");

    EXPECT_TRUE(File::writeText(oldPath, "renamed content"));
    EXPECT_TRUE(File::rename(oldPath, newPath));

    EXPECT_FALSE(File::exists(oldPath));
    EXPECT_TRUE(File::exists(newPath));

    std::string readback = File::readText(newPath);
    EXPECT_EQ(readback, "renamed content");

    File::remove(newPath);
}

// ---------------------------------------------------------------------------
// Directory
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, Directory_IsDirectory) {
    std::string filePath = mkpath("is_dir_file.txt");
    std::string dirPath = mkpath("is_dir_dir");

    EXPECT_TRUE(File::writeText(filePath, "data"));
    EXPECT_TRUE(Directory::create(dirPath));

    EXPECT_TRUE(Directory::isDirectory(dirPath));
    EXPECT_FALSE(Directory::isDirectory(filePath));

    File::remove(filePath);
    Directory::remove(dirPath);
}

TEST_F(HooFSTest, Directory_CreateTree) {
    std::string nested = testDir + "/a/b/c/d";

    EXPECT_TRUE(Directory::createTree(nested));
    EXPECT_TRUE(Directory::isDirectory(nested));

    Directory::remove(nested);
    Directory::remove(testDir + "/a/b/c");
    Directory::remove(testDir + "/a/b");
    Directory::remove(testDir + "/a");
}

TEST_F(HooFSTest, Directory_List) {
    std::string file1 = mkpath("list_a.txt");
    std::string file2 = mkpath("list_b.txt");

    File::writeText(file1, "aaa");
    File::writeText(file2, "bbb");

    std::vector<std::string> entries = Directory::list(testDir);
    ASSERT_GE(entries.size(), size_t{2});

    bool foundA = false, foundB = false;
    for (const auto& e : entries) {
        if (e == "list_a.txt") foundA = true;
        if (e == "list_b.txt") foundB = true;
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    File::remove(file1);
    File::remove(file2);
}

// ---------------------------------------------------------------------------
// Binary I/O
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, File_BinaryRoundTrip) {
    std::string path = mkpath("binary.bin");
    std::vector<uint8_t> data = {0x00, 0xFF, 0xAB, 0xCD, 0x12, 0x34};

    EXPECT_TRUE(File::writeBytes(path, data));

    std::vector<uint8_t> readback;
    EXPECT_TRUE(File::readBytes(path, readback));

    ASSERT_EQ(readback.size(), data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(readback[i], data[i]) << "byte mismatch at index " << i;
    }

    File::remove(path);
}

// ---------------------------------------------------------------------------
// C-ABI Bridge Compatibility (ensure JIT / FFI still works)
// ---------------------------------------------------------------------------

TEST_F(HooFSTest, CAbi_Bridge_Exists) {
    std::string path = mkpath("cabi_exists.txt");
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);

    EXPECT_TRUE(File::writeText(path, "hi"));
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 1);

    File::remove(path);
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
