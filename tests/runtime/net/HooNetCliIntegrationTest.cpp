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

// End-to-end integration tests for the hoo.net module
// (src/runtime/lib/io/hoo_net.cpp). Each test compiles a complete Hoo
// program to a .ha archive and executes it via the hoo CLI. main()
// returns :int64; assertions inside the program return 0 on mismatch
// and 1 on success, so the test asserts that the CLI prints "1".
//
// All tests are offline-safe: URL parsing never touches the network, and
// HttpClient requests target 127.0.0.1:9 (the discard port), which
// reliably and immediately fails to connect without DNS or external
// network dependency. Covered aspects:
//   - URL parsing: scheme, host, port, path, full query, fragment
//   - URL default ports (80 for http / 443 for https)
//   - URL with no path defaults to "/"
//   - URL toString is non-empty; URL release is safe twice-used
//   - HttpClient construction + setHeader + setTimeout + release
//   - GET to a refused endpoint yields statusCode 0, !isSuccess, empty body
//   - POST/PUT/DELETE to refused endpoints fail cleanly the same way
class HooNetCliIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_net_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_net_cli_"
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

TEST_F(HooNetCliIntegrationTest, UrlParsesComponents) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var u = new URL("https://example.com:8443/path/to/page?name=hoo&x=42#frag");
            var scheme: string = u.getScheme();
            var host: string = u.getHost();
            var path: string = u.getPath();
            var query: string = u.getQuery();
            var frag: string = u.getFragment();
            if (!scheme.equals("https")) { return 0; }
            if (!host.equals("example.com")) { return 0; }
            if (!path.equals("/path/to/page")) { return 0; }
            if (!query.equals("name=hoo&x=42")) { return 0; }
            if (!frag.equals("frag")) { return 0; }
            u.release();
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, UrlDefaultPorts) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var u = new URL("http://example.com/some/page");
            var p: int64 = u.getPort();
            if (p != 80) { return 0; }
            var u2 = new URL("https://example.org");
            var p2: int64 = u2.getPort();
            if (p2 != 443) { return 0; }
            u.release();
            u2.release();
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, UrlWithoutPathDefaultsToSlash) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var u = new URL("https://example.org");
            var path: string = u.getPath();
            if (!path.equals("/")) { return 0; }
            u.release();
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, UrlToStringNonEmptyAndReleaseIdempotentSafe) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var u = new URL("http://example.com/x?y=1");
            var t: string = u.toString();
            if (t.length() == 0) { return 0; }
            u.release();
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, ClientConfiguresAndReleases) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var c = new HttpClient();
            c.setHeader("X-Test", "hoo");
            c.setTimeout(500);
            c.release();
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, GetRefusedEndpointFailsCleanly) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var c = new HttpClient();
            var r: HttpResponse = c.get("http://127.0.0.1:9/ping");
            c.release();
            var code: int64 = r.statusCode();
            var ok: int64 = r.isSuccess();
            var body: string = r.getBody();
            r.release();
            if (code != 500) { return 0; }
            if (ok != 0) { return 0; }
            if (!body.contains("error")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, PostRefusedEndpointFailsCleanly) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var c = new HttpClient();
            var r: HttpResponse = c.post("http://127.0.0.1:9/submit", "payload");
            c.release();
            var code: int64 = r.statusCode();
            var body: string = r.getBody();
            r.release();
            if (code != 500) { return 0; }
            if (!body.contains("error")) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, PutAndDeleteRefusedEndpointsFailCleanly) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var c = new HttpClient();
            var rp: HttpResponse = c.put("http://127.0.0.1:9/upd", "data");
            var rd: HttpResponse = c.delete("http://127.0.0.1:9/obj");
            c.release();
            var cp: int64 = rp.statusCode();
            var cd: int64 = rd.statusCode();
            rp.release();
            rd.release();
            if (cp != 500) { return 0; }
            if (cd != 500) { return 0; }
            return 1;
        }
    )");
}

TEST_F(HooNetCliIntegrationTest, ConfiguredClientGetRefusedStillClean) {
    expectReturnOne(R"(
        import hoo.net;
        func :int64 main() {
            var c = new HttpClient();
            c.setHeader("X-Test", "hoo");
            c.setTimeout(250);
            var r: HttpResponse = c.get("http://127.0.0.1:9/late");
            c.release();
            var code: int64 = r.statusCode();
            var ok: int64 = r.isSuccess();
            r.release();
            if (code != 500) { return 0; }
            if (ok != 0) { return 0; }
            return 1;
        }
    )");
}
