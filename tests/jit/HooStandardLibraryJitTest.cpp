#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooStandardLibraryJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooStandardLibraryJitTest, SystemHostname) {
    const std::string source = R"(
        import hoo.system;
        func:int64 test() {
            var name = system_hostname();
            return name.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Assuming hostname length is > 0
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooStandardLibraryJitTest, FsExists) {
    const std::string source = R"(
        import hoo.io;
        func:int64 test() {
            // Check if current directory exists, should be true (1)
            return Fs.exists(".");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStandardLibraryJitTest, RegexCompile) {
    const std::string source = R"(
        import hoo.regex;
        func:int64 test() {
            var re = Regex.compile("[a-z]+");
            var result = Regex.match(re, "hello");
            return result;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStandardLibraryJitTest, UuidV4) {
    const std::string source = R"(
        import hoo.uuid;
        func:int64 test() {
            var id = Uuid.v4();
            var str = Uuid.to_string(id);
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 36); // UUID is 36 chars long
}

TEST_F(HooStandardLibraryJitTest, EncodingBase64) {
    const std::string source = R"(
        import hoo.encoding;
        func:int64 test() {
            var str = "Hello";
            var bytes = str.data();
            var len = str.length();
            var b64 = encoding_base64_encode(bytes, len);
            return b64.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // "Hello" is 5 bytes -> base64 is 8 bytes
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8);
}

TEST_F(HooStandardLibraryJitTest, MissingImportMath) {
    const std::string source = R"(
        func:int64 test() {
            return math_abs(-42);
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.math;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportDateTime) {
    const std::string source = R"(
        func:ptr test() {
            return new DateTime();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.datetime;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportDateTimeFunc) {
    const std::string source = R"(
        func:ptr test() {
            return datetime_now();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.datetime;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportFs) {
    const std::string source = R"(
        func:int64 test() {
            return Fs.exists(".");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.io;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportFsFunc) {
    const std::string source = R"(
        func:int64 test() {
            return fs_exists(".");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.io;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportSystem) {
    const std::string source = R"(
        func:ptr test() {
            return system_hostname();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.system;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportRegex) {
    const std::string source = R"(
        func:ptr test() {
            return Regex.compile("[a-z]+");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.regex;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportNet) {
    const std::string source = R"(
        func:ptr test() {
            return new HttpClient();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.net;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportPath) {
    const std::string source = R"(
        func:ptr test() {
            return Path.absolute("a");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.path;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportHashing) {
    const std::string source = R"(
        func:ptr test() {
            var data = "hello";
            return hashing_md5(data.data(), 5);
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.hashing;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportEncoding) {
    const std::string source = R"(
        func:ptr test() {
            var data = "hello";
            return encoding_base64_encode(data.data(), 5);
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.encoding;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportUuid) {
    const std::string source = R"(
        func:ptr test() {
            return Uuid.v4();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.uuid;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportCompression) {
    const std::string source = R"(
        func:ptr test() {
            return new Compression();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.compression;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportProcess) {
    const std::string source = R"(
        func:ptr test() {
            return new Process();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.process;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportArgs) {
    const std::string source = R"(
        func:ptr test() {
            return new Args();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.args;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportCsv) {
    const std::string source = R"(
        func:ptr test() {
            return new Csv();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.csv;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportConsole) {
    const std::string source = R"(
        func:int64 test() {
            Console.println("test");
            return 0;
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportThread) {
    const std::string source = R"(
        func:ptr test() {
            return new Thread();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.thread;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportThreadFunc) {
    const std::string source = R"(
        func:int64 test() {
            return thread_self();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.thread;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportCharacter) {
    const std::string source = R"(
        func:ptr test() {
            return new Character();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.character;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportBuffer) {
    const std::string source = R"(
        func:ptr test() {
            return new Buffer();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.buffer;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportBufferFunc) {
    const std::string source = R"(
        func:ptr test() {
            var data = "hello";
            return buffer_fromBytes(data.data(), 5);
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.buffer;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportHashMap) {
    const std::string source = R"(
        func:ptr test() {
            return new HashMap<int64, int64>();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.collections;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportAnyArray) {
    const std::string source = R"(
        func:ptr test() {
            return new AnyArray();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.collections;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, MissingImportJsonFunc) {
    const std::string source = R"(
        func:ptr test() {
            return json_minify("{\"a\": 1}");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.json;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, IncorrectImportMathForDateTime) {
    const std::string source = R"(
        import hoo.math;
        func:ptr test() {
            return new DateTime();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.datetime;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, IncorrectSymbolImport) {
    const std::string source = R"(
        from hoo.math import datetime_now;
        func:ptr test() {
            return datetime_now();
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.datetime;'"), std::string::npos);
}

TEST_F(HooStandardLibraryJitTest, IncorrectImportForFs) {
    const std::string source = R"(
        import hoo.datetime;
        func:int64 test() {
            return Fs.exists(".");
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("requires 'import hoo.io;'"), std::string::npos);
}



