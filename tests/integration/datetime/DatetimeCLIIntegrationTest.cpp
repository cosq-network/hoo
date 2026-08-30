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

// End-to-end integration tests for the hoo.datetime module
// (src/runtime/lib/datetime). Each test is a complete Hoo program compiled to
// a .ha archive and executed via the hoo CLI. The CLI prints the int64 result
// of the entry point as the final line of stdout, so every program returns
// `1` on success (or another sentinel indicating which assertion failed), and
// the test asserts on the printed value and/or program output.
//
// Covered aspects:
//   - Current-time utilities: datetime_now(), datetime_now_seconds(),
//     datetime_now_precise()
//   - Construction: datetime_new(timestamp) including pre-epoch timestamps
//   - ISO 8601 parsing: with/without milliseconds, +/- timezone offsets, no
//     timezone suffix, pre-epoch dates, epoch boundary (1969-12-31T23:59:59Z),
//     rejection of invalid input and trailing garbage
//   - Custom-format parsing (strftime-style) and rejection of trailing garbage
//   - Formatting: iso8601(), format() with %Y/%m/%d/%H/%M/%S/%f/%w/%j,
//     literal text, pre-epoch dates
//   - Arithmetic: addDays/addHours/addMinutes/addSeconds/addMilliseconds
//     (positive, negative, cross-boundary, immutability of the original),
//     and the corresponding datetime_add_* free functions
//   - Difference and comparison: diffDays/diffHours/diffSeconds (including
//     fractional results and reversed operand order) and compare (-1/0/1)
//   - Instance methods vs. module-level free functions agreement
//   - Verbatim program output via print/println (ISO 8601 strings)
class DatetimeCLIIntegrationTest : public ::testing::Test {
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
        return tempDir + "/hoo_datetime_cli_"
            + std::to_string(std::time(nullptr))
            + "_" + std::to_string(++counter) + suffix;
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

    // The CLI prints the entry-point return value as the final line.
    static std::string lastLine(const std::string& output) {
        std::string s = output;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
            s.pop_back();
        }
        const std::string::size_type pos = s.find_last_of('\n');
        return (pos == std::string::npos) ? s : s.substr(pos + 1);
    }

    void expectReturnValue(const ExecResult& result, const std::string& expected) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_EQ(lastLine(result.output), expected) << result.output;
    }

    void expectPass(const ExecResult& result) {
        expectReturnValue(result, "1");
    }

    void expectOutputContains(const ExecResult& result, const std::string& expected) {
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find(expected), std::string::npos) << result.output;
    }
};

// ─────────────────────────────────────────────────────────────────────────
// Current-time utilities
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, NowReturnsCurrentTimestamp) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_now();
            if (!dt) { return 0; }
            if (dt.getTimestamp() < 1700000000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, NowSecondsAfterEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            if (datetime_now_seconds() < 1700000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, NowPreciseReturnsDouble) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            if (datetime_now_precise() < 1700000000.0) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Construction from a raw timestamp
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, NewFromPositiveTimestamp) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_new(1704067200000);
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1704067200000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, NewFromNegativeTimestamp) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_new(-301239000000);
            if (!dt) { return 0; }
            if (dt.getTimestamp() != -301239000000) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// ISO 8601 parsing
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601ExactTimestamp) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705314600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601WithMilliseconds) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00.123Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705314600123) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601PositiveOffset) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T16:00:00+05:30");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705314600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601NegativeOffset) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T04:30:00-06:00");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705314600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601WithoutTimezoneSuffix) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705314600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601ExactEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1970-01-01T00:00:00Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601PreEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1960-06-15T10:30:00Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != -301239000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601EpochMinusOneSecond) {
    // 1969-12-31T23:59:59Z is the instant with epoch seconds == -1; it is a
    // valid timestamp and must parse to -1000 ms (not be treated as failure).
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1969-12-31T23:59:59Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != -1000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601PreEpochWithMilliseconds) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1969-12-31T23:59:59.500Z");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != -500) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601RejectsInvalidStrings) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("junk");
            if (a) { return 0; }
            var b = datetime_from_iso8601("");
            if (b) { return 0; }
            var c = datetime_from_iso8601("2024-01-15");
            if (c) { return 0; }
            var d = datetime_from_iso8601("2024-01-15 10:30:00Z");
            if (d) { return 0; }
            var e = datetime_from_iso8601("not-a-date");
            if (e) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseIso8601RejectsTrailingGarbage) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("2024-01-15T10:30:00Zjunk");
            if (a) { return 0; }
            var b = datetime_from_iso8601("2024-01-15T10:30:00.123junk");
            if (b) { return 0; }
            var c = datetime_from_iso8601("2024-01-15T10:30:00Z ");
            if (c) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Custom-format parsing
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, ParseCustomFormatExact) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_parse("2024-06-15 10:30:00", "%Y-%m-%d %H:%M:%S");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1718447400000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseCustomDateOnly) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_parse("2024-01-15", "%Y-%m-%d");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1705276800000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseCustomLeapDay) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_parse("2024-02-29", "%Y-%m-%d");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != 1709164800000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseCustomPreEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_parse("1960-06-15 10:30:00", "%Y-%m-%d %H:%M:%S");
            if (!dt) { return 0; }
            if (dt.getTimestamp() != -301239000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ParseCustomRejectsTrailingGarbage) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_parse("2024-06-15junk", "%Y-%m-%d");
            if (a) { return 0; }
            var b = datetime_parse("2024-06-15", "%Y-%m-%d %H:%M:%S");
            if (b) { return 0; }
            var c = datetime_parse("", "%Y-%m-%d");
            if (c) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Formatting (instance methods and free functions)
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, Iso8601OutputExact) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var iso = dt.iso8601();
            if (!iso.equals("2024-01-15T10:30:00.000Z")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, Iso8601IncludesMilliseconds) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00.123Z");
            var iso = dt.iso8601();
            if (!iso.equals("2024-01-15T10:30:00.123Z")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, Iso8601PreEpochOutput) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1960-06-15T10:30:00Z");
            var iso = dt.iso8601();
            if (!iso.equals("1960-06-15T10:30:00.000Z")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatYmdHms) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:45Z");
            var s = dt.format("%Y-%m-%d %H:%M:%S");
            if (!s.equals("2024-01-15 10:30:45")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatPreEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1960-06-15T10:30:00Z");
            var s = dt.format("%Y-%m-%d %H:%M:%S");
            if (!s.equals("1960-06-15 10:30:00")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatEpochDate) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_new(0);
            var s = dt.format("%Y-%m-%dT%H:%M:%S");
            if (!s.equals("1970-01-01T00:00:00")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatMillisecondDirective) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00.123Z");
            var s = dt.format("%Y-%m-%dT%H:%M:%S.%fZ");
            if (!s.equals("2024-01-15T10:30:00.123Z")) { return 0; }
            var dt2 = datetime_from_iso8601("1960-06-15T10:30:00Z");
            var s2 = dt2.format("%f");
            if (!s2.equals("000")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatWeekdayAndYearday) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var mon = datetime_from_iso8601("2024-01-15T00:00:00Z");
            if (!mon.format("%w").equals("1")) { return 0; }
            if (!mon.format("%j").equals("014")) { return 0; }
            var wed = datetime_from_iso8601("1960-06-15T00:00:00Z");
            if (!wed.format("%w").equals("3")) { return 0; }
            if (!wed.format("%j").equals("166")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FormatLiteralText) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var s = dt.format("Hello World");
            if (!s.equals("Hello World")) { return 0; }
            var pct = dt.format("100%% done");
            if (!pct.equals("100% done")) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FreeFunctionFormattingAgrees) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-06-15T10:30:00Z");
            var isoInst = dt.iso8601();
            var isoFree = datetime_iso8601(dt);
            if (!isoInst.equals(isoFree)) { return 0; }
            var fmtInst = dt.format("%Y/%m/%d");
            var fmtFree = datetime_format(dt, "%Y/%m/%d");
            if (!fmtInst.equals(fmtFree)) { return 0; }
            if (!fmtFree.equals("2024/06/15")) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Arithmetic: addDays/addHours/addMinutes/addSeconds/addMilliseconds
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, AddDaysPositive) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var next = dt.addDays(1);
            if (next.getTimestamp() != 1705401000000) { return 0; }
            var far = dt.addDays(365);
            if (far.getTimestamp() != 1705314600000 + 365 * 86400000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddDaysNegative) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var prev = dt.addDays(-1);
            if (prev.getTimestamp() != 1705228200000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddDaysCrossesMonthBoundary) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var jan31 = datetime_from_iso8601("2024-01-31T00:00:00Z");
            var feb1 = jan31.addDays(1);
            if (feb1.getTimestamp() != 1706745600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddHours) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var plus = dt.addHours(3);
            if (plus.getTimestamp() != 1705325400000) { return 0; }
            var minus = dt.addHours(-3);
            if (minus.getTimestamp() != 1705303800000) { return 0; }
            var day = dt.addHours(24);
            if (day.getTimestamp() != 1705401000000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddMinutes) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var plus = dt.addMinutes(90);
            if (plus.getTimestamp() != 1705320000000) { return 0; }
            var minus = dt.addMinutes(-90);
            if (minus.getTimestamp() != 1705309200000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddSeconds) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var plus = dt.addSeconds(2);
            if (plus.getTimestamp() != 1705314602000) { return 0; }
            var minus = dt.addSeconds(-2);
            if (minus.getTimestamp() != 1705314598000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddMilliseconds) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var plus = dt.addMilliseconds(1234);
            if (plus.getTimestamp() != 1705314601234) { return 0; }
            var minus = dt.addMilliseconds(-1234);
            if (minus.getTimestamp() != 1705314598766) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddMethodsReturnNewInstance) {
    // The original DateTime must remain unchanged after an add.
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var after = dt.addDays(1);
            if (after.getTimestamp() == dt.getTimestamp()) { return 0; }
            if (dt.getTimestamp() != 1705314600000) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, AddCrossesEpochBoundary) {
    // 1969-12-31T23:59:59Z + 1s == epoch; + 1ms == -999 ms.
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1969-12-31T23:59:59Z");
            var plusMs = dt.addMilliseconds(1);
            if (plusMs.getTimestamp() != -999) { return 0; }
            var plusS = dt.addSeconds(1);
            if (plusS.getTimestamp() != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, FreeFunctionAddFormsAgree) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            var inst = dt.addDays(2);
            var free = datetime_add_days(dt, 2);
            if (inst.getTimestamp() != free.getTimestamp()) { return 0; }
            var instM = dt.addMinutes(5);
            var freeM = datetime_add_minutes(dt, 5);
            if (instM.getTimestamp() != freeM.getTimestamp()) { return 0; }
            var instS = dt.addMilliseconds(250);
            var freeS = datetime_add_milliseconds(dt, 250);
            if (instS.getTimestamp() != freeS.getTimestamp()) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Difference and comparison
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, DiffDaysExact) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var jan1 = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var jun15 = datetime_from_iso8601("2024-06-15T00:00:00Z");
            if (jan1.diffDays(jun15) != 166) { return 0; }
            var pre = datetime_from_iso8601("1960-06-15T10:30:00Z");
            var end = datetime_from_iso8601("1969-12-31T23:59:59Z");
            if (pre.diffDays(end) != 3486) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, DiffDaysNegativeForReversedOperands) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var jan1 = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var jun15 = datetime_from_iso8601("2024-06-15T00:00:00Z");
            if (jun15.diffDays(jan1) != -166) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, DiffHoursExact) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var start = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var end = datetime_from_iso8601("2024-01-02T06:00:00Z");
            if (start.diffHours(end) != 30) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, DiffSecondsFractional) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var b = datetime_from_iso8601("2024-01-01T00:00:02.500Z");
            if (a.diffSeconds(b) != 2.5) { return 0; }
            if (b.diffSeconds(a) != -2.5) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, DiffSecondsFreeFunction) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var b = datetime_from_iso8601("2024-06-15T00:00:00Z");
            var diff = datetime_diff_seconds(a, b);
            if (diff != 14342400.0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, CompareAllSigns) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var b = datetime_from_iso8601("2024-06-15T00:00:00Z");
            if (a.compare(b) != -1) { return 0; }
            if (b.compare(a) != 1) { return 0; }
            if (a.compare(a) != 0) { return 0; }
            if (datetime_compare(b, a) != 1) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, ComparePreEpoch) {
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("1960-06-15T10:30:00Z");
            var b = datetime_from_iso8601("2024-01-15T10:30:00Z");
            if (a.compare(b) != -1) { return 0; }
            if (b.compare(a) != 1) { return 0; }
            var same = datetime_from_iso8601("1960-06-15T10:30:00Z");
            if (a.compare(same) != 0) { return 0; }
            return 1;
        }
    )"));
}

TEST_F(DatetimeCLIIntegrationTest, DiffWithMillisecondPrecision) {
    // Diff in days is truncated, hours in truncated, seconds are fractional.
    expectPass(compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var a = datetime_from_iso8601("2024-01-01T00:00:00Z");
            var b = datetime_from_iso8601("2024-01-01T00:00:00.900Z");
            if (a.diffDays(b) != 0) { return 0; }
            if (a.diffHours(b) != 0) { return 0; }
            if (a.diffSeconds(b) != 0.9) { return 0; }
            return 1;
        }
    )"));
}

// ─────────────────────────────────────────────────────────────────────────
// Program output via print/println
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, PrintIso8601Output) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-01-15T10:30:00Z");
            println(dt.iso8601());
            return 1;
        }
    )");
    expectOutputContains(result, "2024-01-15T10:30:00.000Z");
}

TEST_F(DatetimeCLIIntegrationTest, PrintFormatOutput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("2024-06-15T10:30:00Z");
            println(dt.format("%Y/%m/%d %H:%M"));
            return 1;
        }
    )");
    expectOutputContains(result, "2024/06/15 10:30");
}

TEST_F(DatetimeCLIIntegrationTest, PrintPreEpochIsoOutput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1960-06-15T10:30:00Z");
            println(dt.iso8601());
            return 1;
        }
    )");
    expectOutputContains(result, "1960-06-15T10:30:00.000Z");
}

TEST_F(DatetimeCLIIntegrationTest, PrintFreeFunctionOutput) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var dt = datetime_from_iso8601("1969-12-31T23:59:59Z");
            println(datetime_iso8601(dt));
            return 1;
        }
    )");
    expectOutputContains(result, "1969-12-31T23:59:59.000Z");
}

// ─────────────────────────────────────────────────────────────────────────
// Combined workflow program
// ─────────────────────────────────────────────────────────────────────────

TEST_F(DatetimeCLIIntegrationTest, CombinedDateTimeWorkflow) {
    const auto result = compileAndRun(R"(
        import hoo;
        import hoo.datetime;
        func :int64 main() {
            var due = datetime_parse("2024-06-15 09:00:00", "%Y-%m-%d %H:%M:%S");
            if (!due) { return 0; }
            var today = datetime_from_iso8601("2024-06-10T09:00:00Z");
            var daysLeft = today.diffDays(due);
            if (daysLeft != 5) { return 0; }
            var reminder = today.addDays(5);
            if (reminder.compare(due) != 0) { return 0; }
            var plusWeek = due.addMilliseconds(7 * 86400000);
            var iso = plusWeek.iso8601();
            if (!iso.equals("2024-06-22T09:00:00.000Z")) { return 0; }
            var s = plusWeek.format("%A %d %B %Y");
            if (s.length() == 0) { return 0; }
            println(due.iso8601());
            return 1;
        }
    )");
    expectOutputContains(result, "2024-06-15T09:00:00.000Z");
}
