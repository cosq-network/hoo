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

// End-to-end integration tests for the hoo Dict class
// (src/runtime/lib/mem/hoo_dict.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI. main()
// returns :int64; assertions inside the program return 0 on mismatch
// and 1 on success, so the test asserts that the CLI prints "1".
//
// Covered aspects:
//   - typed dict construction (Dict<int64, int64>, Dict<int64, string>, ...)
//   - subscript assignment and lookup for fixed value types
//   - overwriting an existing key keeps count single
//   - count reflects insertions and removals
//   - remove deletes an entry and subsequent lookup no longer hits
//   - clear empties the dict
//   - Dict<any> accepts values of different runtime types
//   - byte and int8 key variants
//   - release() after use does not crash the runtime
class HooDictCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_dict_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_dict_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
    }

    ExecResult runHoo(const std::string& args) {
        #ifdef _WIN32
        const std::string command = "\"\"" + hooExe + "\" " + args + " 2>&1\"";
        #else
        const std::string command = "\"" + hooExe + "\" " + args + " 2>&1";
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

    void expectReturnOne(const std::string& source) {
        const auto result = compileAndRun(source);
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
    }
};

TEST_F(HooDictCliIntegrationTest, SubscriptSetAndGetInt64) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[10] = 100;
            d[20] = 200;
            if (d[10] != 100) { return 0; }
            if (d[20] != 200) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, OverwriteExistingKeyKeepsSingleEntry) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 10;
            d[1] = 99;
            if (d[1] != 99) { return 0; }
            if (d.count() != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, CountTracksInsertions) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            if (d.count() != 0) { return 0; }
            d[1] = 5;
            d[2] = 6;
            d[3] = 7;
            if (d.count() != 3) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, RemoveDeletesEntry) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 10;
            d[2] = 20;
            d[3] = 30;
            if (d.count() != 3) { return 0; }
            d.remove(2);
            if (d.count() != 2) { return 0; }
            if (d[1] + d[3] != 40) { return 0; }
            d.remove(1);
            d.remove(3);
            if (d.count() != 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, ClearEmptiesCollection) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 1;
            d[2] = 2;
            d[3] = 3;
            d.clear();
            if (d.count() != 0) { return 0; }
            d[9] = 90;
            if (d.count() != 1) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, StringValuesRoundTrip) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, string> = new Dict<int64, string>();
            d[1] = "one";
            d[2] = "two";
            var one: string = d[1];
            var two: string = d[2];
            if (!one.equals("one")) { return 0; }
            if (!two.equals("two")) { return 0; }
            if (one.length() + two.length() != 6) { return 0; }
            if (d.count() != 2) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, AnyValueEntriesOfDifferentTypes) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, any> = new Dict<int64, any>();
            d[1] = 42;
            d[2] = "hello";
            d[3] = true;
            if (d.count() != 3) { return 0; }
            var s: string = d[2];
            if (!s.equals("hello")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, ByteAndInt8KeyVariants) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var b: Dict<byte, int64> = new Dict<byte, int64>();
            b[65] = 100;
            b[66] = 200;
            var r: Dict<int8, int64> = new Dict<int8, int64>();
            r[1] = 200;
            if (b[65] + b[66] + r[1] != 500) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooDictCliIntegrationTest, ReleaseDoesNotCrashRuntime) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var d: Dict<int64, int64> = new Dict<int64, int64>();
            d[1] = 1;
            d[2] = 2;
            d.release();
            return 1;
        }
    )");
}
