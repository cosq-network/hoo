#include <gtest/gtest.h>
#include <cstdio>
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
#endif

#ifndef HOO_EXECUTABLE
#error "HOO_EXECUTABLE must be defined via CMake -D"
#endif

class CsvCLIIntegrationTest : public ::testing::Test {
protected:
    struct ExecResult { std::string output; int exitCode; };

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
        return tempDir + "/hoo_csv_cli_" + std::to_string(std::time(nullptr)) +
            "_" + std::to_string(++counter) + suffix;
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

    void expectPass(const ExecResult& result) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("1"), std::string::npos) << result.output;
    }
};

TEST_F(CsvCLIIntegrationTest, LifecycleAndReferenceCounting) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var rows = csv.parse("a,b\n1,2");
            if (rows.length() != 2) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, ParseRowsAndFields) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var rows = csv.parse("name,age\nAlice,30\nBob,25");
            if (rows.length() != 3) { return 0; }
            var row0 = rows[0];
            var row1 = rows[1];
            var header: string = row0[0];
            var age: string = row1[1];
            if (!header.equals("name")) { return 0; }
            if (!age.equals("30")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, ParseQuotedAndTrailingFields) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var rows = csv.parse("a,\"b,c\",\n1,\"line\nvalue\",");
            if (rows.length() != 2) { return 0; }
            var row0 = rows[0];
            var row1 = rows[1];
            var quoted: string = row0[1];
            var empty: string = row0[2];
            var multiline: string = row1[1];
            if (!quoted.equals("b,c")) { return 0; }
            if (!empty.equals("")) { return 0; }
            if (!multiline.equals("line\nvalue")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, GenerateEscapesFields) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = [["name", "comment"], ["Alice", "hello, world"], ["Bob", "say \"hi\""]];
            var output = csv.generate(data);
            if (!output.contains("hello, world")) { return 0; }
            if (!output.contains("say \"\"hi\"\"")) { return 0; }
            if (output.length() <= 0) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, CustomDelimiterAndQuoteOptions) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = csv_from_opts(59, 39);
            var rows = csv.parse("name;value\nAlice;42");
            if (rows.length() != 2) { return 0; }
            var row: array = rows[1];
            var name: string = row[0];
            if (!name.equals("Alice")) { return 0; }
            if (!csv.escape(59) || !csv.escape(39) || csv.escape(65)) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, ParseAsMapsAndReadFileAsMaps) {
    const std::string path = uniquePath(".csv");
    {
        std::ofstream file(path);
        file << "name,score\nAlice,10\nBob,20\n";
    }
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var parsed = csv.parseAsMaps("name,score\nAlice,10\nBob,20");
            var fromFile = csv.readFileAsMaps(")") + path + R"(");
            if (parsed.length() != 2 || fromFile.length() != 2) { return 0; }
            var parsedRow: Map = parsed[0];
            var fileRow: Map = fromFile[1];
            var parsedName: string = parsedRow.getStringString("name");
            var fileScore: string = fileRow.getStringString("score");
            if (!parsedName.equals("Alice")) { return 0; }
            if (!fileScore.equals("20")) { return 0; }
            csv.release();
            return 1;
        }
    )");
    std::filesystem::remove(path);
    expectPass(result);
}

TEST_F(CsvCLIIntegrationTest, FileReadAndWrite) {
    const std::string path = uniquePath(".csv");
    const auto result = compileAndRun(std::string(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = [["x", "y"], ["10", "20"]];
            if (csv.writeFile(")") + path + R"(", data) != 0) { return 0; }
            var rows = csv.readFile(")" + path + R"(");
            var row: array = rows[1];
            var value: string = row[1];
            if (rows.length() != 2 || !value.equals("20")) { return 0; }
            csv.release();
            return 1;
        }
    )");
    std::filesystem::remove(path);
    expectPass(result);
}

TEST_F(CsvCLIIntegrationTest, Aggregations) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = csv.parseAsMaps("name,score\nAlice,10\nBob,20\nCara,30");
            if (csv.count(data, "score") != 3) { return 0; }
            if (csv.sum(data, "score") != 60) { return 0; }
            var average: string = csv.avg(data, "score");
            var minimum: string = csv.min(data, "name");
            var maximum: string = csv.max(data, "name");
            if (!average.equals("20")) { return 0; }
            if (!minimum.equals("Alice")) { return 0; }
            if (!maximum.equals("Cara")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, SelectColumns) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = csv.parseAsMaps("name,age,city\nAlice,30,Paris\nBob,25,London");
            var columns = ["name", "city"];
            var selected = csv.select(data, columns);
            if (selected.length() != 2) { return 0; }
            var row: Map = selected[0];
            var name: string = row.getStringString("name");
            var city: string = row.getStringString("city");
            if (!name.equals("Alice")) { return 0; }
            if (!city.equals("Paris")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, FilterEqualityAndNumericOrdering) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = csv.parseAsMaps("name,score\nAlice,10\nBob,20\nCara,30");
            var equal = csv.filter(data, "name", "==", "Bob");
            var greater = csv.filter(data, "score", ">", "15");
            if (equal.length() != 1 || greater.length() != 2) { return 0; }
            var row: Map = equal[0];
            var name: string = row.getStringString("name");
            if (!name.equals("Bob")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, SortAscendingAndDescending) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = csv.parseAsMaps("name,score\nCara,30\nAlice,10\nBob,20");
            var ascending = csv.sort(data, "score", 1);
            var descending = csv.sort(data, "score", 0);
            var firstAscending: Map = ascending[0];
            var firstDescending: Map = descending[0];
            var ascendingName: string = firstAscending.getStringString("name");
            var descendingName: string = firstDescending.getStringString("name");
            if (!ascendingName.equals("Alice")) { return 0; }
            if (!descendingName.equals("Cara")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, DescribeStatistics) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var data = csv.parseAsMaps("score\n10\n20\n30");
            var stats: Map = csv.describe(data, "score");
            var count: string = stats.getStringString("count");
            var sum: string = stats.getStringString("sum");
            var average: string = stats.getStringString("avg");
            var minimum: string = stats.getStringString("min");
            var maximum: string = stats.getStringString("max");
            if (!count.equals("3")) { return 0; }
            if (!sum.equals("60")) { return 0; }
            if (!average.equals("20")) { return 0; }
            if (!minimum.equals("10")) { return 0; }
            if (!maximum.equals("30")) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}

TEST_F(CsvCLIIntegrationTest, InvalidInputReturnsEmptyResults) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.csv;
        func :int64 main() {
            var csv = new Csv();
            var malformed = csv.parse("a,\"unterminated");
            var missing = csv.readFile("/definitely/missing/hoo.csv");
            if (malformed != 0 || missing != 0) { return 0; }
            csv.release();
            return 1;
        }
    )"));
}
