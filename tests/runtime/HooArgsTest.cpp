#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/args/hoo_args.h"

class HooArgsTest : public ::testing::Test {
protected:
    void* handle = nullptr;

    void SetUp() override {
        // No global init needed for these tests - each test does its own
    }

    void TearDown() override {
        hoo_args_shutdown();
    }
};

TEST_F(HooArgsTest, NoArgs) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_count(h), 0);
    EXPECT_EQ(hoo_args_get(h, 0), nullptr);
    EXPECT_EQ(hoo_args_has(h, "key"), 0);
    EXPECT_EQ(hoo_args_value(h, "key"), nullptr);
}

TEST_F(HooArgsTest, PositionalArgs) {
    const char* argv[] = {"program", "file1", "file2"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_count(h), 2);
    EXPECT_STREQ(hoo_args_get(h, 0), "file1");
    EXPECT_STREQ(hoo_args_get(h, 1), "file2");
    EXPECT_EQ(hoo_args_get(h, 2), nullptr);
}

TEST_F(HooArgsTest, LongNamedArgs) {
    const char* argv[] = {"program", "--name=value", "--flag"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(hoo_args_value(h, "name"), "value");
    EXPECT_EQ(hoo_args_has(h, "name"), 1);
    EXPECT_EQ(hoo_args_has(h, "flag"), 1);
    EXPECT_EQ(hoo_args_has(h, "nonexistent"), 0);
}

TEST_F(HooArgsTest, LongNamedArgWithSeparateValue) {
    const char* argv[] = {"program", "--output", "out.txt"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_count(h), 0);
    EXPECT_STREQ(hoo_args_value(h, "output"), "out.txt");
    EXPECT_EQ(hoo_args_has(h, "output"), 1);
}

TEST_F(HooArgsTest, ShortFlags) {
    const char* argv[] = {"program", "-o", "out.txt", "-v"};
    hoo_args_init(4, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_count(h), 0);
    EXPECT_STREQ(hoo_args_value(h, "o"), "out.txt");
    EXPECT_EQ(hoo_args_has(h, "v"), 1);
}

TEST_F(HooArgsTest, DoubleDashStopsOptions) {
    const char* argv[] = {"program", "--", "--not-a-flag", "arg"};
    hoo_args_init(4, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_has(h, "not-a-flag"), 0);
    EXPECT_STREQ(hoo_args_get(h, 0), "--not-a-flag");
    EXPECT_STREQ(hoo_args_get(h, 1), "arg");
}

TEST_F(HooArgsTest, MixedArgs) {
    const char* argv[] = {"program", "-v", "--name=test", "input.txt", "--verbose"};
    hoo_args_init(5, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_has(h, "v"), 1);
    EXPECT_STREQ(hoo_args_value(h, "name"), "test");
    EXPECT_EQ(hoo_args_has(h, "verbose"), 1);
    EXPECT_STREQ(hoo_args_get(h, 0), "input.txt");
}

// ── Argparse-style API tests ─────────────────────────────────────────────────

TEST_F(HooArgsTest, AddStringWithDefault) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "Output file", "default.txt");
    hoo_args_add_flag(h, "verbose", "-v", "--verbose", "Verbose mode");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "default.txt");
    EXPECT_EQ(hoo_args_get_bool(h, "verbose"), 0);
}

TEST_F(HooArgsTest, AddStringFromArg) {
    const char* argv[] = {"program", "--output=result.txt"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "Output", "default.txt");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "result.txt");
}

TEST_F(HooArgsTest, AddFlagSet) {
    const char* argv[] = {"program", "-v"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_flag(h, "verbose", "-v", "--verbose", "Verbose");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_bool(h, "verbose"), 1);
}

TEST_F(HooArgsTest, AddFlagNotSet) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_flag(h, "verbose", "-v", "--verbose", "Verbose");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_bool(h, "verbose"), 0);
}

TEST_F(HooArgsTest, AddIntWithDefault) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_int(h, "count", "-c", "--count", "Count", 42);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_int(h, "count"), 42);
}

TEST_F(HooArgsTest, AddIntFromArg) {
    const char* argv[] = {"program", "--count=100"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_int(h, "count", "-c", "--count", "Count", 0);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_int(h, "count"), 100);
}

TEST_F(HooArgsTest, AddIntFromShortOpt) {
    const char* argv[] = {"program", "-c", "99"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_int(h, "count", "-c", "--count", "Count", 0);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_int(h, "count"), 99);
}

TEST_F(HooArgsTest, AddFloatWithDefault) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_float(h, "threshold", "-t", "--threshold", "Threshold", 0.5);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_DOUBLE_EQ(hoo_args_get_float(h, "threshold"), 0.5);
}

TEST_F(HooArgsTest, AddFloatFromArg) {
    const char* argv[] = {"program", "--threshold=3.14"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_float(h, "threshold", "-t", "--threshold", "Threshold", 0.5);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_DOUBLE_EQ(hoo_args_get_float(h, "threshold"), 3.14);
}

TEST_F(HooArgsTest, AddFloatFromShortOpt) {
    const char* argv[] = {"program", "-t", "2.718"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_float(h, "threshold", "-t", "--threshold", "Threshold", 1.0);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_DOUBLE_EQ(hoo_args_get_float(h, "threshold"), 2.718);
}

TEST_F(HooArgsTest, AddPositionalGetsFirstArg) {
    const char* argv[] = {"program", "myfile.txt"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_positional(h, "input", "Input file");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "input"), "myfile.txt");
}

TEST_F(HooArgsTest, AddPositionalDefaultEmpty) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_positional(h, "input", "Input file");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "input"), "");
}

TEST_F(HooArgsTest, ParseHelpReturnsZero) {
    const char* argv[] = {"program", "--help"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "Output", "out.txt");
    EXPECT_EQ(hoo_args_parse(h), 0);
}

TEST_F(HooArgsTest, HelpTextContainsKeyElements) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "Output file", "out.txt");
    hoo_args_add_flag(h, "verbose", "-v", "--verbose", "Verbose mode");
    hoo_args_add_positional(h, "input", "Input file path");
    char* help = hoo_args_help_text(h);
    ASSERT_NE(help, nullptr);
    EXPECT_GT(strlen(help), 0);
    EXPECT_NE(strstr(help, "usage:"), nullptr);
    EXPECT_NE(strstr(help, "--output"), nullptr);
    EXPECT_NE(strstr(help, "-v, --verbose"), nullptr);
    EXPECT_NE(strstr(help, "input"), nullptr);
    EXPECT_NE(strstr(help, "--help"), nullptr);
    EXPECT_NE(strstr(help, "Output file"), nullptr);
    EXPECT_NE(strstr(help, "default: out.txt"), nullptr);
    free(help);
}

TEST_F(HooArgsTest, ClearAndReuseHandle) {
    const char* argv[] = {"program", "--output=result.txt"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "Output", "default.txt");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "result.txt");
    hoo_args_clear(h);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "");
    hoo_args_add_string(h, "name", "-n", "--name", "Name", "unknown");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "name"), "unknown");
}

TEST_F(HooArgsTest, ShortOptOnlyMatch) {
    const char* argv[] = {"program", "-o", "value.txt"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "", "Output", "default.txt");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "value.txt");
}

TEST_F(HooArgsTest, LongOptOnlyMatch) {
    const char* argv[] = {"program", "--output", "value.txt"};
    hoo_args_init(3, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "", "--output", "Output", "default.txt");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "value.txt");
}

TEST_F(HooArgsTest, GetIntParsesStringValue) {
    const char* argv[] = {"program", "--count=99"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_int(h, "count", "-c", "--count", "Count", 0);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_EQ(hoo_args_get_int(h, "count"), 99);
}

TEST_F(HooArgsTest, GetFloatParsesStringValue) {
    const char* argv[] = {"program", "--threshold=2.718"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_float(h, "threshold", "-t", "--threshold", "Threshold", 1.0);
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_DOUBLE_EQ(hoo_args_get_float(h, "threshold"), 2.718);
}

TEST_F(HooArgsTest, MultiplePositionalArgs) {
    const char* argv[] = {"program", "first", "second", "third"};
    hoo_args_init(4, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_positional(h, "arg1", "First");
    hoo_args_add_positional(h, "arg2", "Second");
    hoo_args_add_positional(h, "arg3", "Third");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "arg1"), "first");
    EXPECT_STREQ(hoo_args_get_string(h, "arg2"), "second");
    EXPECT_STREQ(hoo_args_get_string(h, "arg3"), "third");
}

TEST_F(HooArgsTest, MixedPositionalAndFlag) {
    const char* argv[] = {"program", "input.txt", "--output=result.txt", "-v"};
    hoo_args_init(4, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_positional(h, "input", "Input file");
    hoo_args_add_string(h, "output", "-o", "--output", "Output", "default.txt");
    hoo_args_add_flag(h, "verbose", "-v", "--verbose", "Verbose");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "input"), "input.txt");
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "result.txt");
    EXPECT_EQ(hoo_args_get_bool(h, "verbose"), 1);
}

TEST_F(HooArgsTest, UnknownArgReturnsDefault) {
    const char* argv[] = {"program"};
    hoo_args_init(1, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    hoo_args_add_string(h, "output", "-o", "--output", "", "default.txt");
    EXPECT_EQ(hoo_args_parse(h), 1);
    EXPECT_STREQ(hoo_args_get_string(h, "output"), "default.txt");
}

TEST_F(HooArgsTest, GetStringForNonexistentName) {
    const char* argv[] = {"program", "--output=val"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(hoo_args_get_string(h, "nonexistent"), "");
}

TEST_F(HooArgsTest, ParseWithNoDefinitions) {
    const char* argv[] = {"program", "arg"};
    hoo_args_init(2, argv);
    void* h = hoo_args_new();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(hoo_args_parse(h), 1);
}
