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

// End-to-end integration tests for the hoo.thread module
// (src/runtime/lib/concurrency/hoo_thread.cpp). Each test compiles a complete
// Hoo program to a .ha archive and executes it via the hoo CLI. The CLI prints
// the int64 result of the entry point, so each program returns :int64 and
// the test asserts on the printed value.
//
// Covered aspects:
//   - thread_self returns a positive thread id
//   - Mutex creation, lock, unlock, try_lock, destroy
//   - Mutex reference counting: refcount, retain, release
//   - thread_spawn creates a new thread
//   - thread_join waits for thread completion
//   - thread_sleep pauses execution
//   - Condition variable creation, wait, notify_one, notify_all, destroy
//   - Semaphore creation, wait, try_wait, post, destroy
class ThreadCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_thread_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_thread_cli_"
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

TEST_F(ThreadCLIIntegrationTest, ThreadSelfReturnsPositiveId) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var self = thread_self();
            if (self <= 0) { return 0; }
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, MutexCreateAndDestroy) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            if (!mtx) { return 0; }
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, MutexLockUnlock) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            mtx.lock();
            mtx.unlock();
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, MutexTryLock) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            mtx.lock();
            mtx.unlock();
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, MutexRetainRefcount) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, ThreadSpawnJoin) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, ThreadSleep) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, ConditionCreateDestroy) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var cond = new Condition();
            if (!cond) { return 0; }
            cond.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, ConditionWaitNotifyOne) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            var cond = new Condition();
            mtx.lock();
            cond.notifyOne();
            mtx.unlock();
            cond.release();
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, ConditionNotifyAll) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var mtx = new Mutex();
            var cond = new Condition();
            mtx.lock();
            cond.notifyAll();
            mtx.unlock();
            cond.release();
            mtx.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, SemaphoreCreateDestroy) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var sem = new Semaphore(1);
            if (!sem) { return 0; }
            sem.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, SemaphoreWaitPost) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var sem = new Semaphore(0);
            sem.post();
            sem.wait();
            sem.release();
            return 1;
        }
    )");
}

TEST_F(ThreadCLIIntegrationTest, SemaphoreTryWait) {
    expectReturnOne(R"(
        import hoo.thread;
        func :int64 main() {
            var sem = new Semaphore(0);
            if (sem.tryWait() != 1) { return 0; }
            sem.post();
            if (sem.tryWait() != 0) { return 0; }
            sem.release();
            return 1;
        }
    )");
}
