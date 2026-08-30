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

// End-to-end CLI integration tests for the hoo.json module. Each test
// compiles a complete Hoo program to a .ha archive and executes it; the CLI
// prints the int64 result of the entry point as the final line, so every
// program returns `1` on success (or a sentinel identifying the failed
// assertion), and the test asserts on the printed value.
class JsonCLIIntegrationTest : public ::testing::Test {
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
        const std::string path = tempDir + "/hoo_json_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".hoo";
        std::ofstream file(path);
        file << source;
        return path;
    }

    std::string createArchive() {
        static int counter = 0;
        return tempDir + "/hoo_json_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + ".ha";
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
        const std::string archivePath = createArchive();
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

    void expectFailure(const ExecResult& result) {
        EXPECT_NE(result.exitCode, 0) << result.output;
    }
};

// ─────────────────────────────────────────────────────────────────────────
// Serialization
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, SerializeEmptyDict) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            var json = json_serialize_hashmap(m);
            return json.equals("{}");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeDictInt64Values) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[1] = 42;
            m[2] = 7;
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":42")) { return 0; }
            if (!json.contains("\"2\":7")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeDictStringValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, string> = new Dict<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":\"hello\"")) { return 0; }
            if (!json.contains("\"2\":\"world\"")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeDictMixedTypes) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = 42;
            m[2] = "hello";
            m[3] = 3.14;
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":42")) { return 0; }
            if (!json.contains("\"2\":\"hello\"")) { return 0; }
            if (!json.contains("\"3\":3.14")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeDictNestedDict) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var inner: Dict<int64, int64> = new Dict<int64, int64>();
            inner[10] = 99;
            var outer: Dict<int64, any> = new Dict<int64, any>();
            outer[1] = inner;
            var json = json_serialize_hashmap(outer);
            if (!json.contains("\"1\":{")) { return 0; }
            if (!json.contains("\"10\":99")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeDictNestedList) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            var values = [1, 2, 3]any;
            m[1] = values;
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":[1,2,3]")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeEmptyList) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = []any;
            var json = json_serialize_anyarray(values);
            return json.equals("[]");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeListIntValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = [10, 20, 30]any;
            var json = json_serialize_anyarray(values);
            return json.equals("[10,20,30]");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeListMixedValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = [1, "two", true]any;
            var json = json_serialize_anyarray(values);
            return json.equals("[1,\"two\",true]");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeListNestedDict) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[1] = 42;
            var values = [m]any;
            var json = json_serialize_anyarray(values);
            if (!json.contains("[{\"1\":42}]")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, SerializeListNestedList) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var inner = [4, 5]any;
            var values = [inner, 6]any;
            var json = json_serialize_anyarray(values);
            if (!json.contains("[[4,5],6]")) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Deserialization
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, DeserializeEmptyObject) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{}");
            return m.count() == 0;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeEmptyArray) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("[]");
            return a.length() == 0;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeObjectIntValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"1\":42,\"2\":7}");
            if (m.count() != 2) { return 0; }
            if (m[1] != 42) { return 0; }
            if (m[2] != 7) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeObjectStringValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"1\":\"hello\",\"2\":\"world\"}");
            if (m.count() != 2) { return 0; }
            var s1: string = m[1];
            if (!s1.equals("hello")) { return 0; }
            var s2: string = m[2];
            if (!s2.equals("world")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeObjectNested) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"1\":{\"2\":42}}");
            if (m.count() != 1) { return 0; }
            var json = json_serialize_hashmap(m);
            if (!json.contains("{\"2\":42}")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeArrayIntValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("[10,20,30]");
            if (a.length() != 3) { return 0; }
            if (a[0] != 10) { return 0; }
            if (a[1] != 20) { return 0; }
            if (a[2] != 30) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeArrayMixedValues) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("[1,\"two\",true]");
            if (a.length() != 3) { return 0; }
            if (a[0] != 1) { return 0; }
            var s: string = a[1];
            if (!s.equals("two")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeArrayNestedArray) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("[[1,2],[3,4]]");
            if (a.length() != 2) { return 0; }
            var json = json_serialize_anyarray(a);
            if (!json.contains("[1,2]")) { return 0; }
            if (!json.contains("[3,4]")) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Round-trip: serialize → deserialize → verify
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, RoundTripDictInt64) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[1] = 42;
            m[2] = 7;
            var json = json_serialize_hashmap(m);
            var decoded = json_deserialize_hashmap(json);
            if (decoded.count() != 2) { return 0; }
            if (decoded[1] != 42) { return 0; }
            if (decoded[2] != 7) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, RoundTripDictString) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, string> = new Dict<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            var json = json_serialize_hashmap(m);
            var decoded = json_deserialize_hashmap(json);
            if (decoded.count() != 2) { return 0; }
            var s1: string = decoded[1];
            if (!s1.equals("hello")) { return 0; }
            var s2: string = decoded[2];
            if (!s2.equals("world")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, RoundTripListInt64) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = [10, 20, 30]any;
            var json = json_serialize_anyarray(values);
            var decoded = json_deserialize_anyarray(json);
            if (decoded.length() != 3) { return 0; }
            if (decoded[0] != 10) { return 0; }
            if (decoded[1] != 20) { return 0; }
            if (decoded[2] != 30) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, RoundTripListMixed) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = [1, "two", true]any;
            var json = json_serialize_anyarray(values);
            var decoded = json_deserialize_anyarray(json);
            if (decoded.length() != 3) { return 0; }
            if (decoded[0] != 1) { return 0; }
            var s: string = decoded[1];
            if (!s.equals("two")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, RoundTripEmptyDict) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            var json = json_serialize_hashmap(m);
            var decoded = json_deserialize_hashmap(json);
            return decoded.count() == 0;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, RoundTripEmptyList) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = []any;
            var json = json_serialize_anyarray(values);
            var decoded = json_deserialize_anyarray(json);
            return decoded.length() == 0;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Float64 round-trip
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, Float64IntegralValuePreservesDecimalPoint) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = 2.0;
            var json = json_serialize_hashmap(m);
            if (!json.contains("2.0")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, Float64NonIntegralPreservesValue) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = 3.14;
            var json = json_serialize_hashmap(m);
            if (!json.contains("3.14")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, Float64InListRoundTrip) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var values = [1.0, 2.5]any;
            var json = json_serialize_anyarray(values);
            if (!json.contains("1.0")) { return 0; }
            if (!json.contains("2.5")) { return 0; }
            var decoded = json_deserialize_anyarray(json);
            if (decoded.length() != 2) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Bool values
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, BoolTrueSerializeDeserialize) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = true;
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":true")) { return 0; }
            var decoded = json_deserialize_hashmap(json);
            if (decoded[1] != true) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, BoolFalseSerializeDeserialize) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = false;
            var json = json_serialize_hashmap(m);
            if (!json.contains("\"1\":false")) { return 0; }
            var decoded = json_deserialize_hashmap(json);
            if (decoded[1] != false) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Null handling
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, NullInArray) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("[1,null,3]");
            if (a.length() != 3) { return 0; }
            if (a[0] != 1) { return 0; }
            if (a[2] != 3) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Minify / Beautify
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, MinifyValidJson) {
    expectPass(compileAndRun(R"(
        import hoo.json;
        func :int64 main() {
            var result = json_minify("{ \"a\" : [ 1, true ] }");
            return result.equals("{\"a\":[1,true]}");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, BeautifyValidJson) {
    expectPass(compileAndRun(R"(
        import hoo.json;
        func :int64 main() {
            var result = json_beautify("{\"a\":1}");
            return result.contains("\n  \"a\": 1");
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, MinifyAndBeautifyRoundTrip) {
    expectPass(compileAndRun(R"(
        import hoo.json;
        func :int64 main() {
            var minified = json_minify("{ \"x\" : 1, \"y\" : [ 2, 3 ] }");
            var beautified = json_beautify(minified);
            var reMinified = json_minify(beautified);
            return minified.equals(reMinified);
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Boundary integers
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, DeserializeInt64MaxValue) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"1\":9223372036854775807}");
            return m[1] == 9223372036854775807;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeInt64MinValue) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[1] = -9223372036854775807;
            m[2] = -1;
            var json = json_serialize_hashmap(m);
            var decoded = json_deserialize_hashmap(json);
            if (decoded[1] != -9223372036854775807) { return 0; }
            if (decoded[2] != -1) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(JsonCLIIntegrationTest, DeserializeInt64Zero) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"1\":0}");
            return m[1] == 0;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Complex round-trip
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, RoundTripNestedStructures) {
    expectPass(compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var inner: Dict<int64, int64> = new Dict<int64, int64>();
            inner[10] = 99;
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = inner;
            var values = [42, "text"]any;
            m[2] = values;
            var json = json_serialize_hashmap(m);
            var decoded = json_deserialize_hashmap(json);
            if (decoded.count() != 2) { return 0; }
            var decodedJson = json_serialize_hashmap(decoded);
            if (!decodedJson.contains("\"10\":99")) { return 0; }
            if (!decodedJson.contains("\"text\"")) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Error cases
// ─────────────────────────────────────────────────────────────────────────

TEST_F(JsonCLIIntegrationTest, InvalidJsonDeserializeFails) {
    const auto result = compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{invalid}");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, NonObjectRootForHashmapFails) {
    const auto result = compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("[1,2]");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, NonArrayRootForListFails) {
    const auto result = compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var a = json_deserialize_anyarray("{\"1\":2}");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, NonIntegerKeyForHashmapFails) {
    const auto result = compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{\"name\":\"Alice\"}");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, TrailingGarbageFails) {
    const auto result = compileAndRun(R"(
        import hoo.collections;
        import hoo.json;
        func :int64 main() {
            var m = json_deserialize_hashmap("{} extra");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, InvalidMinifyInputFails) {
    const auto result = compileAndRun(R"(
        import hoo.json;
        func :int64 main() {
            var r = json_minify("{bad");
            return 1;
        }
    )");
    expectFailure(result);
}

TEST_F(JsonCLIIntegrationTest, InvalidBeautifyInputFails) {
    const auto result = compileAndRun(R"(
        import hoo.json;
        func :int64 main() {
            var r = json_beautify("[1,]");
            return 1;
        }
    )");
    expectFailure(result);
}
