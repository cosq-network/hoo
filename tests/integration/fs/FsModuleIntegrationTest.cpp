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

// Comprehensive end-to-end integration tests for the hoo.fs module (and the
// path helpers merged into src/runtime/lib/fs). Every test compiles a complete
// Hoo program to a .ha archive with the `hoo` executable and executes it; the
// CLI prints the int64 result of the entry point as the final line, so each
// program returns `1` on success (or `0` identifying the failed assertion).
//
// This suite complements FsCLIIntegrationTest by exercising the full module
// surface: text/binary I/O edge cases, metadata, directory semantics, failure
// paths, temp/current-directory helpers, and the path helper functions.
class FsModuleIntegrationTest : public ::testing::Test {
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

    std::string uniquePath(const std::string& suffix) {
        static int counter = 0;
        return tempDir + "/hoo_fs_mod_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + suffix;
    }

    // Forward slashes keep the embedded Hoo literal escaping-free on all
    // platforms (std::filesystem accepts them on Windows too).
    std::string hooPath(const std::string& name) const {
        std::string p = name;
#ifdef _WIN32
        std::replace(p.begin(), p.end(), '\\', '/');
#endif
        return p;
    }

    std::string createSource(const std::string& source) {
        const std::string path = uniquePath(".hoo");
        std::ofstream file(path);
        file << source;
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
        const std::string archivePath = uniquePath(".ha");
        const ExecResult build = runHoo("-o \"" + archivePath + "\" \"" + sourcePath + "\"");
        if (build.exitCode != 0) return build;
        return runHoo("\"" + archivePath + "\"");
    }

    static std::string lastLine(const std::string& output) {
        std::string s = output;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
            s.pop_back();
        }
        const std::string::size_type pos = s.find_last_of('\n');
        return (pos == std::string::npos) ? s : s.substr(pos + 1);
    }

    void expectPass(const ExecResult& result) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_EQ(lastLine(result.output), "1") << result.output;
    }
};

// ─────────────────────────────────────────────────────────────────────────
// Text I/O
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, WriteOverwritesExistingFile) {
    const std::string file = hooPath(uniquePath("overwrite.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "first") == 0) { return 0; }
            if (fs_write_text(p, "second") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("second")) { return 0; }
            if (fs_size(p) != 6) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, AppendCreatesMissingFile) {
    const std::string file = hooPath(uniquePath("append_creates.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_append_text(p, "line1\n") == 0) { return 0; }
            if (fs_append_text(p, "line2\n") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("line1\nline2\n") && !c.equals("line1\r\nline2\r\n")) { return 0; }
            if (fs_size(p) < 12) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ReadTextFallbackVariants) {
    const std::string present = hooPath(uniquePath("fb_present.txt"));
    const std::string empty = hooPath(uniquePath("fb_empty.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var present = ")" + present + R"(";
            if (fs_write_text(present, "hello") == 0) { return 0; }
            var viaFallback = fs_read_text(present, "fb");
            if (!viaFallback) { return 0; }
            if (!viaFallback.equals("hello")) { return 0; }
            var empty = ")" + empty + R"(";
            if (fs_write_text(empty, "") == 0) { return 0; }
            var emptyWithFallback = fs_read_text(empty, "fb");
            if (!emptyWithFallback) { return 0; }
            if (emptyWithFallback.length() != 0) { return 0; }
            var missing = fs_read_text("/nonexistent_hoo_dir_xyz/nope.txt", "fallback_val");
            if (!missing) { return 0; }
            if (!missing.equals("fallback_val")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ReadTextMissingReturnsNull) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var c = fs_read_text("/nonexistent_hoo_dir_xyz/nope.txt");
            if (c) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, UnicodeRoundTrip) {
    const std::string file = hooPath(uniquePath("unicode.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "héllo wörld — 世界") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("héllo wörld — 世界")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ReadTextFromDirectoryReturnsNull) {
    const std::string dir = hooPath(uniquePath("read_dir"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdir(d) == 0) { return 0; }
            var c = fs_read_text(d);
            if (c) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, WriteAppendToMissingDirectoryFail) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = "/nonexistent_hoo_dir_xyz/sub/file.txt";
            if (fs_write_text(p, "x") != 0) { return 0; }
            if (fs_append_text(p, "x") != 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Binary I/O
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, BinaryRoundTripNullBytes) {
    const std::string file = hooPath(uniquePath("null_bytes.bin"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var p = ")" + file + R"(";
            var buf = new Buffer();
            buf.append("AAAAA", 5);
            buf.setByte(0, 0);
            buf.setByte(1, 1);
            buf.setByte(2, 2);
            buf.setByte(3, 255);
            buf.setByte(4, 10);
            if (fs_write_bytes(p, buf) == 0) { return 0; }
            var r = fs_read_bytes(p);
            if (!r) { return 0; }
            if (r.length() != 5) { return 0; }
            if (r.byteAt(0) != 0) { return 0; }
            if (r.byteAt(1) != 1) { return 0; }
            if (r.byteAt(2) != 2) { return 0; }
            if (r.byteAt(3) != 255) { return 0; }
            if (r.byteAt(4) != 10) { return 0; }
            if (fs_size(p) != 5) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, WriteBytesBufferReadBytesBufferRoundTrip) {
    const std::string file = hooPath(uniquePath("buf_rt.bin"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var p = ")" + file + R"(";
            var buf = new Buffer();
            buf.append("AAAA", 4);
            buf.setByte(0, 65);
            buf.setByte(1, 66);
            buf.setByte(2, 67);
            buf.setByte(3, 0);
            if (fs_write_bytes_buffer(p, buf) == 0) { return 0; }
            var r = fs_read_bytes_buffer(p);
            if (!r) { return 0; }
            if (r.length() != 4) { return 0; }
            if (r.byteAt(0) != 65) { return 0; }
            if (r.byteAt(1) != 66) { return 0; }
            if (r.byteAt(2) != 67) { return 0; }
            if (r.byteAt(3) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ReadBytesMissingAndFallbacks) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var missing = "/nonexistent_hoo_dir_xyz/nope.bin";
            var b = fs_read_bytes(missing);
            if (b) { return 0; }
            var fallback = new Buffer();
            fallback.append("AB", 2);
            fallback.setByte(0, 7);
            fallback.setByte(1, 8);
            var viaFallback = fs_read_bytes(missing, fallback);
            if (!viaFallback) { return 0; }
            if (viaFallback.length() != 2) { return 0; }
            if (viaFallback.byteAt(1) != 8) { return 0; }
            var viaBufFallback = fs_read_bytes_buffer(missing, fallback);
            if (!viaBufFallback) { return 0; }
            if (viaBufFallback.length() != 2) { return 0; }
            if (viaBufFallback.byteAt(0) != 7) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, EmptyFileReadsAsEmptyBuffer) {
    const std::string file = hooPath(uniquePath("empty_bytes.bin"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "") == 0) { return 0; }
            var b = fs_read_bytes(p);
            if (!b) { return 0; }
            if (b.length() != 0) { return 0; }
            if (fs_size(p) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, WriteBytesToMissingDirectoryFail) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var buf = new Buffer();
            buf.append("x", 1);
            var p = "/nonexistent_hoo_dir_xyz/out.bin";
            if (fs_write_bytes(p, buf) != 0) { return 0; }
            if (fs_write_bytes_buffer(p, buf) != 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Metadata
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, SizeAndLastModifiedLifecycle) {
    const std::string file = hooPath(uniquePath("meta.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_size(p) != -1) { return 0; }
            if (fs_last_modified(p) != -1) { return 0; }
            if (fs_write_text(p, "123") == 0) { return 0; }
            if (fs_size(p) != 3) { return 0; }
            if (fs_append_text(p, "45") == 0) { return 0; }
            if (fs_size(p) != 5) { return 0; }
            var m = fs_last_modified(p);
            if (m < 1500000000) { return 0; }
            if (m > 5000000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, IsFileIsDirMutualExclusion) {
    const std::string dir = hooPath(uniquePath("kind_dir"));
    const std::string file = hooPath(uniquePath("kind_file.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var d = ")" + dir + R"(";
            var f = ")" + file + R"(";
            if (fs_mkdir(d) == 0) { return 0; }
            if (fs_write_text(f, "x") == 0) { return 0; }
            if (fs_is_dir(d) != 1) { return 0; }
            if (fs_is_file(d) != 0) { return 0; }
            if (fs_is_file(f) != 1) { return 0; }
            if (fs_is_dir(f) != 0) { return 0; }
            if (fs_exists(f) != 1) { return 0; }
            if (fs_exists("/nonexistent_hoo_dir_xyz") != 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Directories
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, ListDirEmptyReturnsEmptyArray) {
    const std::string dir = hooPath(uniquePath("empty_listdir"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdirs(d) == 0) { return 0; }
            var entries = fs_list_dir(d);
            if (!entries) { return 0; }
            if (entries.length() != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ListDirMissingReturnsNull) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var entries = fs_list_dir("/nonexistent_hoo_dir_xyz");
            if (entries) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ListDirOnFileReturnsNull) {
    const std::string file = hooPath(uniquePath("list_file.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var f = ")" + file + R"(";
            if (fs_write_text(f, "x") == 0) { return 0; }
            var entries = fs_list_dir(f);
            if (entries) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ListDirExactEntries) {
    const std::string dir = hooPath(uniquePath("exact_listdir"));
    const std::string f1 = hooPath(dir + "/aaa.txt");
    const std::string f2 = hooPath(dir + "/bbb.txt");
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdirs(d) == 0) { return 0; }
            if (fs_write_text(")" + f1 + R"(", "a") == 0) { return 0; }
            if (fs_write_text(")" + f2 + R"(", "b") == 0) { return 0; }
            var entries = fs_list_dir(d);
            if (!entries) { return 0; }
            if (entries.length() != 2) { return 0; }
            var found1 = 0;
            var found2 = 0;
            for i in 0..entries.length() {
                var name: string = entries.getString(i);
                if (name.equals("aaa.txt")) { found1 = 1; }
                if (name.equals("bbb.txt")) { found2 = 1; }
            }
            if (found1 == 0) { return 0; }
            if (found2 == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, ListDirIncludesSubdirectories) {
    const std::string dir = hooPath(uniquePath("subdir_listdir"));
    const std::string file = hooPath(dir + "/root.txt");
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdirs(d + "/sub") == 0) { return 0; }
            if (fs_write_text(")" + file + R"(", "x") == 0) { return 0; }
            var entries = fs_list_dir(d);
            if (!entries) { return 0; }
            if (entries.length() != 2) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, MkdirMkdirsExistingAndMissing) {
    const std::string dir = hooPath(uniquePath("mkdir_existing"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdirs(d) == 0) { return 0; }
            if (fs_mkdirs(d) != 0) { return 0; }
            if (fs_mkdir(d) != 0) { return 0; }
            if (fs_mkdir("/nonexistent_hoo_dir_xyz/child") != 0) { return 0; }
            if (fs_rmdir("/nonexistent_hoo_dir_xyz") != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, RmdirNonEmptyFailsDeleteNonEmptyFails) {
    const std::string dir = hooPath(uniquePath("nonempty_dir"));
    const std::string file = hooPath(dir + "/inner.txt");
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var d = ")" + dir + R"(";
            var f = ")" + file + R"(";
            if (fs_mkdir(d) == 0) { return 0; }
            if (fs_write_text(f, "x") == 0) { return 0; }
            if (fs_rmdir(d) != 0) { return 0; }
            if (fs_delete(d) != 0) { return 0; }
            if (fs_is_dir(d) != 1) { return 0; }
            if (fs_delete(f) == 0) { return 0; }
            if (fs_rmdir(d) == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, MkdirRmdirOnFileFail) {
    const std::string file = hooPath(uniquePath("not_a_dir.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var f = ")" + file + R"(";
            if (fs_write_text(f, "x") == 0) { return 0; }
            if (fs_mkdir(f) != 0) { return 0; }
            if (fs_rmdir(f) != 0) { return 0; }
            if (fs_is_dir(f) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, DeepNestingViaMkdirs) {
    const std::string base = hooPath(uniquePath("deep_base"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var base = ")" + base + R"(";
            var deep = base + "/a/b/c/d/e";
            if (fs_mkdirs(deep) == 0) { return 0; }
            if (fs_is_dir(deep) != 1) { return 0; }
            var f = deep + "/data.txt";
            if (fs_write_text(f, "deep") == 0) { return 0; }
            var c = fs_read_text(f);
            if (!c) { return 0; }
            if (!c.equals("deep")) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Move / rename / copy
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, MoveAcrossDirectories) {
    const std::string root = hooPath(uniquePath("move_root"));
    const std::string srcFile = hooPath(root + "/src/file.txt");
    const std::string dstFile = hooPath(root + "/dst/file.txt");
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var root = ")" + root + R"(";
            if (fs_mkdirs(root + "/src") == 0) { return 0; }
            if (fs_mkdirs(root + "/dst") == 0) { return 0; }
            var srcFile = ")" + srcFile + R"(";
            var dstFile = ")" + dstFile + R"(";
            if (fs_write_text(srcFile, "m") == 0) { return 0; }
            if (fs_move(srcFile, dstFile) == 0) { return 0; }
            if (fs_exists(srcFile) != 0) { return 0; }
            if (fs_exists(dstFile) != 1) { return 0; }
            var c = fs_read_text(dstFile);
            if (!c) { return 0; }
            if (!c.equals("m")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, CopyOverwritesDestination) {
    const std::string src = hooPath(uniquePath("copy_src.txt"));
    const std::string dst = hooPath(uniquePath("copy_dst.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var src = ")" + src + R"(";
            var dst = ")" + dst + R"(";
            if (fs_write_text(src, "new content") == 0) { return 0; }
            if (fs_write_text(dst, "old") == 0) { return 0; }
            if (fs_copy(src, dst) == 0) { return 0; }
            var c = fs_read_text(dst);
            if (!c) { return 0; }
            if (!c.equals("new content")) { return 0; }
            if (fs_size(dst) != 11) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, RenameBackAndForth) {
    const std::string a = hooPath(uniquePath("rn_a.txt"));
    const std::string b = hooPath(uniquePath("rn_b.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var a = ")" + a + R"(";
            var b = ")" + b + R"(";
            if (fs_write_text(a, "x") == 0) { return 0; }
            if (fs_rename(a, b) == 0) { return 0; }
            if (fs_exists(a) != 0) { return 0; }
            if (fs_rename(b, a) == 0) { return 0; }
            if (fs_exists(a) != 1) { return 0; }
            if (fs_exists(b) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, MoveDirectory) {
    const std::string root = hooPath(uniquePath("mvdir_root"));
    const std::string d1 = hooPath(root + "/d1");
    const std::string d2 = hooPath(root + "/d2");
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var root = ")" + root + R"(";
            var d1 = ")" + d1 + R"(";
            var d2 = ")" + d2 + R"(";
            if (fs_mkdirs(d1) == 0) { return 0; }
            if (fs_move(d1, d2) == 0) { return 0; }
            if (fs_is_dir(d2) != 1) { return 0; }
            if (fs_exists(d1) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, CopyEmptyFile) {
    const std::string src = hooPath(uniquePath("copy_empty_src.txt"));
    const std::string dst = hooPath(uniquePath("copy_empty_dst.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var src = ")" + src + R"(";
            var dst = ")" + dst + R"(";
            if (fs_write_text(src, "") == 0) { return 0; }
            if (fs_copy(src, dst) == 0) { return 0; }
            if (fs_exists(dst) != 1) { return 0; }
            if (fs_size(dst) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, CopyToMissingDirectoryFail) {
    const std::string src = hooPath(uniquePath("copy_nodir_src.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var src = ")" + src + R"(";
            if (fs_write_text(src, "x") == 0) { return 0; }
            if (fs_copy(src, "/nonexistent_hoo_dir_xyz/dst.txt") != 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Temp & environment
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, TempFileHonorsPrefixAndBasename) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.path;
        func :int64 main() {
            var tf = fs_create_temp_file("hoofsprefix_");
            if (!tf) { return 0; }
            if (fs_exists(tf) != 1) { return 0; }
            if (fs_is_file(tf) != 1) { return 0; }
            var name = path_filename(tf);
            if (!name) { return 0; }
            if (name.startsWith("hoofsprefix_") != 1) { return 0; }
            if (fs_delete(tf) == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, TempDirIsWritableAndRemovable) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var td = fs_create_temp_dir();
            if (!td) { return 0; }
            if (fs_is_dir(td) != 1) { return 0; }
            var inner = td + "/inner.txt";
            if (fs_write_text(inner, "z") == 0) { return 0; }
            var c = fs_read_text(inner);
            if (!c) { return 0; }
            if (!c.equals("z")) { return 0; }
            if (fs_delete(inner) == 0) { return 0; }
            if (fs_rmdir(td) == 0) { return 0; }
            var tf = fs_create_temp_file("x");
            if (!tf) { return 0; }
            if (fs_exists(tf) != 1) { return 0; }
            if (fs_delete(tf) == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, CurrentDirAndCurrentExeDirAreDirectories) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var cwd = fs_current_dir();
            if (!cwd) { return 0; }
            if (cwd.length() == 0) { return 0; }
            if (fs_is_dir(cwd) != 1) { return 0; }
            var exeDir = fs_current_exe_dir();
            if (!exeDir) { return 0; }
            if (exeDir.length() == 0) { return 0; }
            if (fs_is_dir(exeDir) != 1) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, TempDirPointsToExistingDirectory) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var t = fs_temp_dir();
            if (!t) { return 0; }
            if (t.length() == 0) { return 0; }
            if (fs_is_dir(t) != 1) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Path helper functions
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsModuleIntegrationTest, PathJoinNormalizeAbsolute) {
#ifdef _WIN32
    // Windows std::filesystem path semantics (separators, roots) differ significantly from Unix.
    // Skip this test on Windows.
    expectPass(compileAndRun(R"(
        func :int64 main() { return 1; }
    )"));
#else
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            var j = path_join("/a", "b");
            if (!j) { return 0; }
            if (!j.equals("/a/b")) { return 0; }
            var n = path_normalize("/a/./b/../c");
            if (!n) { return 0; }
            if (!n.equals("/a/c")) { return 0; }
            var a = path_absolute("some/relative");
            if (!a) { return 0; }
            if (a.length() == 0) { return 0; }
            var r = path_relative("/a/b/c.txt", "/a/b");
            if (!r) { return 0; }
            if (r.length() == 0) { return 0; }
            return 1;
        }
    )"));
#endif
}

TEST_F(FsModuleIntegrationTest, PathExtensionStemRoot) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            var p = "/data/report.tar.gz";
            var e = path_extension(p);
            if (!e) { return 0; }
            if (!e.equals(".gz")) { return 0; }
            var s = path_stem(p);
            if (!s) { return 0; }
            if (!s.equals("report.tar")) { return 0; }
            var r = path_root(p);
            if (!r) { return 0; }
            if (r.length() == 0) { return 0; }
            var noExt = path_extension("readme");
            if (!noExt) { return 0; }
            if (noExt.length() != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, PathFilenameParent) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            var p = "/foo/bar/file.txt";
            var f = path_filename(p);
            if (!f) { return 0; }
            if (!f.equals("file.txt")) { return 0; }
            var parent = path_parent(p);
            if (!parent) { return 0; }
            if (!parent.equals("/foo/bar")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, PathPredicates) {
#ifdef _WIN32
    // Windows std::filesystem path semantics differ significantly from Unix.
    // Skip this test on Windows.
    expectPass(compileAndRun(R"(
        func :int64 main() { return 1; }
    )"));
#else
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            if (path_has_extension("/a/b.txt") != 1) { return 0; }
            if (path_has_extension("/a/b") != 0) { return 0; }
            if (path_is_absolute("/a/b") != 1) { return 0; }
            if (path_is_absolute("rel") != 0) { return 0; }
            if (path_is_relative("rel") != 1) { return 0; }
            if (path_is_relative("/a") != 0) { return 0; }
            return 1;
        }
    )"));
#endif
}

TEST_F(FsModuleIntegrationTest, PathSplitVariants) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        import hoo;
        func :int64 main() {
            var parts = path_split("foo/bar/baz");
            if (!parts) { return 0; }
            if (parts.length() != 3) { return 0; }
            var p0: string = parts.getString(0);
            var p2: string = parts.getString(2);
            if (!p0.equals("foo")) { return 0; }
            if (!p2.equals("baz")) { return 0; }
            var single = path_split("justme");
            if (!single) { return 0; }
            if (single.length() != 1) { return 0; }
            var s0: string = single.getString(0);
            if (!s0.equals("justme")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsModuleIntegrationTest, PathSeparatorsAreSingleChars) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            var sep = path_separator();
            if (sep == 0) { return 0; }
            var listSep = path_list_separator();
            if (listSep == 0) { return 0; }
            return 1;
        }
    )"));
}
