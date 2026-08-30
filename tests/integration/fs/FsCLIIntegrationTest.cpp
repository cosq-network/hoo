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

// End-to-end integration tests for the hoo.fs and hoo.path modules via the hoo
// CLI. Each test is a complete Hoo program compiled to a .ha archive and
// executed; the CLI prints the int64 result of the entry point as the final
// line, so every program returns `1` on success (or a sentinel identifying the
// failed assertion), and the test asserts on the printed value.
class FsCLIIntegrationTest : public ::testing::Test {
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
        return tempDir + "/hoo_fs_cli_"
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

TEST_F(FsCLIIntegrationTest, WriteReadAppendDelete) {
    const std::string file = hooPath(uniquePath("rw.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "Hello") == 0) { return 0; }
            if (fs_exists(p) == 0) { return 0; }
            if (fs_size(p) != 5) { return 0; }
            if (fs_append_text(p, " World") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (!c.equals("Hello World")) { return 0; }
            if (fs_delete(p) == 0) { return 0; }
            if (fs_exists(p) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsCLIIntegrationTest, ReadTextEmptyVsMissing) {
    const std::string file = hooPath(uniquePath("empty.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "") == 0) { return 0; }
            var c = fs_read_text(p);
            if (!c) { return 0; }
            if (c.length() != 0) { return 0; }
            var missing = fs_read_text("/nonexistent_hoo_dir_xyz/nope.txt");
            if (missing) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Move / rename / copy / remove
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsCLIIntegrationTest, MoveRenameCopyRemove) {
    const std::string a = hooPath(uniquePath("a.txt"));
    const std::string b = hooPath(uniquePath("b.txt"));
    const std::string c = hooPath(uniquePath("c.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var a = ")" + a + R"(";
            var b = ")" + b + R"(";
            var c = ")" + c + R"(";
            if (fs_write_text(a, "data") == 0) { return 0; }
            if (fs_move(a, b) == 0) { return 0; }
            if (fs_exists(a) != 0) { return 0; }
            if (fs_copy(b, c) == 0) { return 0; }
            if (fs_rename(c, a) == 0) { return 0; }
            if (fs_exists(b) == 0) { return 0; }
            if (fs_exists(a) == 0) { return 0; }
            if (fs_remove(b) == 0) { return 0; }
            if (fs_exists(b) != 0) { return 0; }
            if (fs_remove(a) == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsCLIIntegrationTest, MissingSourceOperationsFail) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            if (fs_move("/nonexistent_hoo_dir_xyz/a", "/nonexistent_hoo_dir_xyz/b") != 0) { return 0; }
            if (fs_copy("/nonexistent_hoo_dir_xyz/a", "/nonexistent_hoo_dir_xyz/b") != 0) { return 0; }
            if (fs_remove("/nonexistent_hoo_dir_xyz/a") != 0) { return 0; }
            if (fs_delete("/nonexistent_hoo_dir_xyz/a") != 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Directories
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsCLIIntegrationTest, MkdirMkdirsRmdir) {
    const std::string d1 = hooPath(uniquePath("d1"));
    const std::string d2 = hooPath(uniquePath("d2"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var d1 = ")" + d1 + R"(";
            var d2 = ")" + d2 + R"(";
            if (fs_mkdir(d1) == 0) { return 0; }
            if (fs_is_dir(d1) == 0) { return 0; }
            if (fs_is_file(d1) != 0) { return 0; }
            if (fs_mkdirs(d2 + "/x/y/z") == 0) { return 0; }
            if (fs_is_dir(d2 + "/x/y/z") == 0) { return 0; }
            if (fs_mkdir("/nonexistent_hoo_dir_xyz/child") != 0) { return 0; }
            if (fs_rmdir(d2 + "/x/y/z") == 0) { return 0; }
            if (fs_rmdir(d1) == 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsCLIIntegrationTest, ListDir) {
    const std::string dir = hooPath(uniquePath("listdir"));
    const std::string f1 = hooPath(dir + "/list_a.txt");
    const std::string f2 = hooPath(dir + "/list_b.txt");
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo;
        func :int64 main() {
            var d = ")" + dir + R"(";
            if (fs_mkdirs(d) == 0) { return 0; }
            var a = ")" + f1 + R"(";
            var b = ")" + f2 + R"(";
            if (fs_write_text(a, "a") == 0) { return 0; }
            if (fs_write_text(b, "b") == 0) { return 0; }
            var entries = fs_list_dir(d);
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
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Metadata
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsCLIIntegrationTest, LastModifiedIsInt64Timestamp) {
    const std::string file = hooPath(uniquePath("mtime.txt"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "x") == 0) { return 0; }
            var m = fs_last_modified(p);
            if (m < 1500000000) { return 0; }
            if (m > 5000000000) { return 0; }
            if (fs_last_modified("/nonexistent_hoo_dir_xyz/file.txt") != -1) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsCLIIntegrationTest, TempAndCurrentDirs) {
    expectPass(compileAndRun(R"(
        import hoo.io;
        func :int64 main() {
            var t = fs_temp_dir();
            if (!t) { return 0; }
            if (t.length() == 0) { return 0; }
            var tf = fs_create_temp_file("hoofscli");
            if (!tf) { return 0; }
            if (fs_exists(tf) == 0) { return 0; }
            if (fs_delete(tf) == 0) { return 0; }
            var td = fs_create_temp_dir();
            if (!td) { return 0; }
            if (fs_is_dir(td) == 0) { return 0; }
            if (fs_rmdir(td) == 0) { return 0; }
            var cwd = fs_current_dir();
            if (!cwd) { return 0; }
            if (cwd.length() == 0) { return 0; }
            var exeDir = fs_current_exe_dir();
            if (!exeDir) { return 0; }
            if (exeDir.length() == 0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Binary I/O
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsCLIIntegrationTest, ReadBytesWriteBytes) {
    const std::string file = hooPath(uniquePath("bin.bin"));
    const std::string copy = hooPath(uniquePath("bin2.bin"));
    expectPass(compileAndRun(R"(
        import hoo.io;
        import hoo.buffer;
        func :int64 main() {
            var p = ")" + file + R"(";
            if (fs_write_text(p, "hello") == 0) { return 0; }
            var b = fs_read_bytes(p);
            if (!b) { return 0; }
            if (b.length() != 5) { return 0; }
            var p2 = ")" + copy + R"(";
            if (fs_write_bytes(p2, b) == 0) { return 0; }
            var b2 = fs_read_bytes(p2);
            if (!b2) { return 0; }
            if (b2.length() != 5) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Path module
// ─────────────────────────────────────────────────────────────────────────

TEST_F(FsCLIIntegrationTest, PathFilenameParentSplit) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        import hoo;
        func :int64 main() {
            var p = "/foo/bar/file.txt";
            var f = path_filename(p);
            if (!f) { return 0; }
            if (!f.equals("file.txt")) { return 0; }
            var parent = path_parent(p);
            if (!parent) { return 0; }
            if (!parent.equals("/foo/bar")) { return 0; }
            var parts = path_split("foo/bar/baz");
            if (!parts) { return 0; }
            if (parts.length() != 3) { return 0; }
            var p0: string = parts.getString(0);
            var p2: string = parts.getString(2);
            if (!p0.equals("foo")) { return 0; }
            if (!p2.equals("baz")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(FsCLIIntegrationTest, PathEmptyResultsNotNull) {
    expectPass(compileAndRun(R"(
        import hoo.path;
        func :int64 main() {
            var e = path_extension("file");
            if (!e) { return 0; }
            if (e.length() != 0) { return 0; }
            var s = path_stem("file");
            if (!s) { return 0; }
            if (!s.equals("file")) { return 0; }
            return 1;
        }
    )"));
}
