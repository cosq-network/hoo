#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/hoo_fs.h"

class HooFSTest : public ::testing::Test {
protected:
    void SetUp() override {
        char* tmpdir = hoo_fs_temp_dir();
        ASSERT_NE(tmpdir, nullptr);
        tempDir = tmpdir;
        hoo_fs_free_string(tmpdir);

        testDir = tempDir + "/hoo_fs_test_" + std::to_string(time(nullptr));
        int64_t ok = hoo_fs_mkdirs(testDir.c_str());
        ASSERT_EQ(ok, 1);
    }

    void TearDown() override {
        hoo_fs_rmdir(testDir.c_str());
    }

    std::string mkpath(const std::string& name) const {
        return testDir + "/" + name;
    }

    std::string tempDir;
    std::string testDir;
};

TEST_F(HooFSTest, TempDir) {
    char* tmp = hoo_fs_temp_dir();
    ASSERT_NE(tmp, nullptr);
    EXPECT_GT(strlen(tmp), 0);
    hoo_fs_free_string(tmp);
}

TEST_F(HooFSTest, Exists) {
    std::string path = mkpath("exists_test.txt");
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);

    int64_t ok = hoo_fs_write_text(path.c_str(), "hello");
    ASSERT_EQ(ok, 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 1);

    ok = hoo_fs_delete(path.c_str());
    ASSERT_EQ(ok, 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);
}

TEST_F(HooFSTest, IsFileIsDir) {
    std::string filePath = mkpath("is_file_test.txt");
    std::string dirPath = mkpath("is_dir_test");

    int64_t ok = hoo_fs_write_text(filePath.c_str(), "data");
    ASSERT_EQ(ok, 1);
    ok = hoo_fs_mkdir(dirPath.c_str());
    ASSERT_EQ(ok, 1);

    EXPECT_EQ(hoo_fs_is_file(filePath.c_str()), 1);
    EXPECT_EQ(hoo_fs_is_file(dirPath.c_str()), 0);
    EXPECT_EQ(hoo_fs_is_dir(dirPath.c_str()), 1);
    EXPECT_EQ(hoo_fs_is_dir(filePath.c_str()), 0);

    hoo_fs_delete(filePath.c_str());
    hoo_fs_rmdir(dirPath.c_str());
}

TEST_F(HooFSTest, ReadWriteText) {
    std::string path = mkpath("readwrite.txt");
    const char* content = "Hello, HooFS!";

    int64_t ok = hoo_fs_write_text(path.c_str(), content);
    ASSERT_EQ(ok, 1);

    char* readback = hoo_fs_read_text(path.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, content);
    hoo_fs_free_string(readback);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, AppendText) {
    std::string path = mkpath("append.txt");

    int64_t ok = hoo_fs_write_text(path.c_str(), "Hello");
    ASSERT_EQ(ok, 1);

    ok = hoo_fs_append_text(path.c_str(), " World");
    ASSERT_EQ(ok, 1);

    char* readback = hoo_fs_read_text(path.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, "Hello World");
    hoo_fs_free_string(readback);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, FileSize) {
    std::string path = mkpath("size_test.txt");
    const char* content = "1234567890";

    int64_t ok = hoo_fs_write_text(path.c_str(), content);
    ASSERT_EQ(ok, 1);

    int64_t size = hoo_fs_size(path.c_str());
    EXPECT_EQ(size, static_cast<int64_t>(strlen(content)));

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, LastModified) {
    std::string path = mkpath("modified_test.txt");

    int64_t ok = hoo_fs_write_text(path.c_str(), "data");
    ASSERT_EQ(ok, 1);

    int64_t mtime = hoo_fs_last_modified(path.c_str());
    EXPECT_GE(mtime, 0);

    hoo_fs_delete(path.c_str());
}

TEST_F(HooFSTest, Delete) {
    std::string path = mkpath("delete_test.txt");

    int64_t ok = hoo_fs_write_text(path.c_str(), "to be deleted");
    ASSERT_EQ(ok, 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 1);

    ok = hoo_fs_delete(path.c_str());
    ASSERT_EQ(ok, 1);
    EXPECT_EQ(hoo_fs_exists(path.c_str()), 0);
}

TEST_F(HooFSTest, Copy) {
    std::string src = mkpath("copy_src.txt");
    std::string dst = mkpath("copy_dst.txt");
    const char* content = "copy me";

    int64_t ok = hoo_fs_write_text(src.c_str(), content);
    ASSERT_EQ(ok, 1);

    ok = hoo_fs_copy(src.c_str(), dst.c_str());
    ASSERT_EQ(ok, 1);

    char* readback = hoo_fs_read_text(dst.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, content);
    hoo_fs_free_string(readback);

    hoo_fs_delete(src.c_str());
    hoo_fs_delete(dst.c_str());
}

TEST_F(HooFSTest, Rename) {
    std::string oldPath = mkpath("rename_old.txt");
    std::string newPath = mkpath("rename_new.txt");

    int64_t ok = hoo_fs_write_text(oldPath.c_str(), "renamed content");
    ASSERT_EQ(ok, 1);

    ok = hoo_fs_rename(oldPath.c_str(), newPath.c_str());
    ASSERT_EQ(ok, 1);

    EXPECT_EQ(hoo_fs_exists(oldPath.c_str()), 0);
    EXPECT_EQ(hoo_fs_exists(newPath.c_str()), 1);

    char* readback = hoo_fs_read_text(newPath.c_str());
    ASSERT_NE(readback, nullptr);
    EXPECT_STREQ(readback, "renamed content");
    hoo_fs_free_string(readback);

    hoo_fs_delete(newPath.c_str());
}

TEST_F(HooFSTest, Mkdirs) {
    std::string nested = testDir + "/a/b/c/d";

    int64_t ok = hoo_fs_mkdirs(nested.c_str());
    ASSERT_EQ(ok, 1);

    EXPECT_EQ(hoo_fs_exists(nested.c_str()), 1);
    EXPECT_EQ(hoo_fs_is_dir(nested.c_str()), 1);

    hoo_fs_rmdir(nested.c_str());
    hoo_fs_rmdir((testDir + "/a/b/c").c_str());
    hoo_fs_rmdir((testDir + "/a/b").c_str());
    hoo_fs_rmdir((testDir + "/a").c_str());
}

TEST_F(HooFSTest, ListDir) {
    std::string file1 = mkpath("list_a.txt");
    std::string file2 = mkpath("list_b.txt");

    hoo_fs_write_text(file1.c_str(), "aaa");
    hoo_fs_write_text(file2.c_str(), "bbb");

    int64_t count = 0;
    char** entries = hoo_fs_list_dir(testDir.c_str(), &count);
    ASSERT_NE(entries, nullptr);
    ASSERT_GE(count, 2);

    bool foundA = false, foundB = false;
    for (int64_t i = 0; i < count; i++) {
        if (strcmp(entries[i], "list_a.txt") == 0) foundA = true;
        if (strcmp(entries[i], "list_b.txt") == 0) foundB = true;
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    hoo_fs_free_list(entries, count);
    hoo_fs_delete(file1.c_str());
    hoo_fs_delete(file2.c_str());
}

TEST_F(HooFSTest, CreateTempFile) {
    char* tmpPath = hoo_fs_create_temp_file("hoofstest");
    ASSERT_NE(tmpPath, nullptr);
    EXPECT_GT(strlen(tmpPath), 0);
    EXPECT_EQ(hoo_fs_exists(tmpPath), 1);

    hoo_fs_delete(tmpPath);
    hoo_fs_free_string(tmpPath);
}
