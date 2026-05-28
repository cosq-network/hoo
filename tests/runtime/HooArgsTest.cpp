#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/hoo_args.h"

class HooArgsTest : public ::testing::Test {
};

TEST_F(HooArgsTest, NoArgs) {
    const char* argv[] = {"program"};
    HooArgsResult result = hoo_args_parse(1, argv);
    EXPECT_EQ(result.count, 0);
    EXPECT_EQ(hoo_args_count(&result), 0);
    EXPECT_EQ(hoo_args_get(&result, "key"), nullptr);
    EXPECT_EQ(hoo_args_has(&result, "key"), 0);
    hoo_args_free(result);
}

TEST_F(HooArgsTest, PositionalArgs) {
    const char* argv[] = {"program", "file1", "file2"};
    HooArgsResult result = hoo_args_parse(3, argv);
    EXPECT_EQ(result.count, 2);
    EXPECT_EQ(hoo_args_count(&result), 2);
    EXPECT_STREQ(hoo_args_positional(&result, 0), "file1");
    EXPECT_STREQ(hoo_args_positional(&result, 1), "file2");
    EXPECT_EQ(hoo_args_positional(&result, 2), nullptr);
    hoo_args_free(result);
}

TEST_F(HooArgsTest, LongNamedArgs) {
    const char* argv[] = {"program", "--name=value", "--flag"};
    HooArgsResult result = hoo_args_parse(3, argv);
    ASSERT_EQ(result.count, 2);
    EXPECT_STREQ(hoo_args_get(&result, "name"), "value");
    EXPECT_EQ(hoo_args_has(&result, "name"), 1);
    EXPECT_EQ(hoo_args_has(&result, "flag"), 1);
    EXPECT_EQ(hoo_args_has(&result, "nonexistent"), 0);
    hoo_args_free(result);
}

TEST_F(HooArgsTest, LongNamedArgWithSeparateValue) {
    const char* argv[] = {"program", "--output", "out.txt"};
    HooArgsResult result = hoo_args_parse(3, argv);
    ASSERT_EQ(result.count, 1);
    EXPECT_STREQ(hoo_args_get(&result, "output"), "out.txt");
    hoo_args_free(result);
}

TEST_F(HooArgsTest, ShortFlags) {
    const char* argv[] = {"program", "-o", "out.txt", "-v"};
    HooArgsResult result = hoo_args_parse(4, argv);
    ASSERT_EQ(result.count, 2);
    EXPECT_STREQ(hoo_args_get(&result, "o"), "out.txt");
    EXPECT_TRUE(hoo_args_has(&result, "v"));
    hoo_args_free(result);
}

TEST_F(HooArgsTest, DoubleDashStopsOptions) {
    const char* argv[] = {"program", "--", "--not-a-flag", "arg"};
    HooArgsResult result = hoo_args_parse(4, argv);
    EXPECT_EQ(hoo_args_has(&result, "not-a-flag"), 0);
    EXPECT_STREQ(hoo_args_positional(&result, 0), "--not-a-flag");
    EXPECT_STREQ(hoo_args_positional(&result, 1), "arg");
    hoo_args_free(result);
}

TEST_F(HooArgsTest, MixedArgs) {
    const char* argv[] = {"program", "-v", "--name=test", "input.txt", "--verbose"};
    HooArgsResult result = hoo_args_parse(5, argv);
    EXPECT_EQ(hoo_args_has(&result, "v"), 1);
    EXPECT_STREQ(hoo_args_get(&result, "name"), "test");
    EXPECT_EQ(hoo_args_has(&result, "verbose"), 1);
    EXPECT_STREQ(hoo_args_positional(&result, 0), "input.txt");
    hoo_args_free(result);
}
