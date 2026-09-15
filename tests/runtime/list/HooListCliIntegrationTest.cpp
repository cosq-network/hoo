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

// End-to-end integration tests for the hoo List class
// (src/runtime/lib/mem/hoo_list.cpp / hoo_generic_array.cpp causal backend).
// Each test compiles a complete Hoo program to a .ha archive and executes
// it via the hoo CLI. main() returns :int64; assertions inside the
// program return 0 on mismatch and 1 on success, so the test asserts
// that the CLI prints "1".
//
// Covered aspects:
//   - push grows the list and length reflects the size
//   - subscript reads return the pushed elements
//   - subscript assignment replaces an existing element
//   - pop removes the last element
//   - clear empties the list while the object stays usable
//   - list accepts elements of mixed runtime types
//   - string element access round-trips
//   - arithmetic combinations of subscript reads
//   - release() after use does not crash the runtime
class HooListCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_list_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_list_cli_"
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

TEST_F(HooListCliIntegrationTest, PushGrowsListAndLengthTracksSize) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            if (values.length() != 0) { return 0; }
            values.push(10);
            values.push(20);
            values.push(30);
            if (values.length() != 3) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, SubscriptReadsReturnPushedElements) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            if (values[0] != 10) { return 0; }
            if (values[1] != 20) { return 0; }
            if (values[2] != 30) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, SubscriptAssignmentReplacesElement) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            values[1] = 99;
            if (values[1] != 99) { return 0; }
            if (values[0] + values[2] != 40) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, PopRemovesLastElement) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(10);
            values.push(20);
            values.push(30);
            values.pop();
            if (values.length() != 2) { return 0; }
            values.pop();
            if (values.length() != 1) { return 0; }
            if (values[0] != 10) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, ClearEmptiesButListStaysUsable) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.push(3);
            values.clear();
            if (values.length() != 0) { return 0; }
            values.push(100);
            values.push(200);
            if (values.length() != 2) { return 0; }
            if (values[0] + values[1] != 300) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, MixedTypesShareOneList) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var mixed = new List();
            mixed.push(42);
            mixed.push("hello");
            mixed.push(true);
            if (mixed.length() != 3) { return 0; }
            var word: string = mixed[1];
            if (!word.equals("hello")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, StringElementsRoundTrip) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var words = new List();
            words.push("foo");
            words.push("bar");
            var w0: string = words[0];
            var w1: string = words[1];
            if (!w0.equals("foo")) { return 0; }
            if (!w1.equals("bar")) { return 0; }
            if (w0.length() + w1.length() != 6) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, SubscriptArithmeticCombinations) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(5);
            values.push(7);
            values.push(11);
            values.push(13);
            if (values[0] + values[3] != 18) { return 0; }
            if (values[1] + values[2] != 18) { return 0; }
            if (values[0] * values[3] != 65) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooListCliIntegrationTest, ReleaseDoesNotCrashRuntime) {
    expectReturnOne(R"(
        import hoo.collections;
        func :int64 main() {
            var values = new List();
            values.push(1);
            values.push(2);
            values.release();
            return 1;
        }
    )");
}
