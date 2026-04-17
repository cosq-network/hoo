#include <gtest/gtest.h>
#include <src/runtime/lib/hoo_string.h>

/**
 * Test suite for HooString (UTF-8 String with ARC)
 *
 * Tests the core string library functions that will be injected into HoocJIT.
 * This serves as both validation of the library and example usage.
 */

class StringBasicsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any per-test state if needed
    }

    void TearDown() override {
        // No cleanup needed - strings handle their own memory
    }
};

// ============================================================================
// Creation and Destruction Tests
// ============================================================================

TEST_F(StringBasicsTest, CreateFromCString) {
    HooString str = hoo_string_from_cstr("Hello, World!");

    EXPECT_NE(str, nullptr);
    EXPECT_EQ(hoo_string_length(str), 13);
    EXPECT_EQ(hoo_string_refcount(str), 1);
    EXPECT_STREQ(hoo_string_data(str), "Hello, World!");

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, CreateFromNullCString) {
    HooString str = hoo_string_from_cstr(nullptr);

    EXPECT_NE(str, nullptr);
    EXPECT_EQ(hoo_string_length(str), 0);
    EXPECT_STREQ(hoo_string_data(str), "");

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, CreateEmptyString) {
    HooString str = hoo_string_new();

    EXPECT_NE(str, nullptr);
    EXPECT_EQ(hoo_string_length(str), 0);
    EXPECT_EQ(hoo_string_refcount(str), 1);
    EXPECT_TRUE(hoo_string_is_empty(str));

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, CreateFromBytes) {
    const char data[] = "Hello\0World";  // Contains null byte
    HooString str = hoo_string_from_bytes(data, 11);

    EXPECT_EQ(hoo_string_length(str), 11);
    EXPECT_EQ(hoo_string_byte_at(str, 5), '\0');
    EXPECT_EQ(hoo_string_byte_at(str, 6), 'W');

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, RepeatCharacter) {
    HooString str = hoo_string_repeat('a', 5);

    EXPECT_EQ(hoo_string_length(str), 5);
    EXPECT_STREQ(hoo_string_data(str), "aaaaa");

    hoo_string_release(str);
}

// ============================================================================
// String Manipulation Tests
// ============================================================================

TEST_F(StringBasicsTest, Concatenation) {
    HooString first = hoo_string_from_cstr("Hello, ");
    HooString second = hoo_string_from_cstr("World!");

    HooString result = hoo_string_concat(first, second);

    EXPECT_EQ(hoo_string_length(result), 13);
    EXPECT_STREQ(hoo_string_data(result), "Hello, World!");

    hoo_string_release(first);
    hoo_string_release(second);
    hoo_string_release(result);
}

TEST_F(StringBasicsTest, ConcatenationWithNull) {
    HooString str = hoo_string_from_cstr("Hello");

    HooString result = hoo_string_concat(nullptr, str);
    EXPECT_STREQ(hoo_string_data(result), "Hello");
    hoo_string_release(result);

    result = hoo_string_concat(str, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "Hello");
    hoo_string_release(result);

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, Substring) {
    HooString str = hoo_string_from_cstr("Hello, World!");

    HooString sub = hoo_string_substring(str, 7, 5);
    EXPECT_STREQ(hoo_string_data(sub), "World");
    hoo_string_release(sub);

    // Out of bounds
    sub = hoo_string_substring(str, 100, 5);
    EXPECT_TRUE(hoo_string_is_empty(sub));
    hoo_string_release(sub);

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, ToUpperCase) {
    HooString str = hoo_string_from_cstr("Hello, World!");

    HooString upper = hoo_string_to_upper(str);
    EXPECT_STREQ(hoo_string_data(upper), "HELLO, WORLD!");

    hoo_string_release(str);
    hoo_string_release(upper);
}

TEST_F(StringBasicsTest, ToLowerCase) {
    HooString str = hoo_string_from_cstr("Hello, World!");

    HooString lower = hoo_string_to_lower(str);
    EXPECT_STREQ(hoo_string_data(lower), "hello, world!");

    hoo_string_release(str);
    hoo_string_release(lower);
}

TEST_F(StringBasicsTest, Trim) {
    HooString str = hoo_string_from_cstr("  \t  Hello, World!  \n  ");

    HooString trimmed = hoo_string_trim(str);
    EXPECT_STREQ(hoo_string_data(trimmed), "Hello, World!");

    hoo_string_release(str);
    hoo_string_release(trimmed);
}

TEST_F(StringBasicsTest, Replace) {
    HooString str = hoo_string_from_cstr("foo bar foo baz foo");
    HooString find = hoo_string_from_cstr("foo");
    HooString repl = hoo_string_from_cstr("qux");

    HooString result = hoo_string_replace(str, find, repl);
    EXPECT_STREQ(hoo_string_data(result), "qux bar qux baz qux");

    hoo_string_release(str);
    hoo_string_release(find);
    hoo_string_release(repl);
    hoo_string_release(result);
}

// ============================================================================
// String Query Tests
// ============================================================================

TEST_F(StringBasicsTest, Length) {
    HooString str = hoo_string_from_cstr("Hello");
    EXPECT_EQ(hoo_string_length(str), 5);
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, ByteAt) {
    HooString str = hoo_string_from_cstr("Hello");

    EXPECT_EQ(hoo_string_byte_at(str, 0), 'H');
    EXPECT_EQ(hoo_string_byte_at(str, 1), 'e');
    EXPECT_EQ(hoo_string_byte_at(str, 4), 'o');
    EXPECT_EQ(hoo_string_byte_at(str, 10), -1);  // Out of bounds
    EXPECT_EQ(hoo_string_byte_at(str, -1), -1);  // Negative index

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, IndexOf) {
    HooString str = hoo_string_from_cstr("Hello, World!");
    HooString needle = hoo_string_from_cstr("World");

    int64_t pos = hoo_string_index_of(str, needle);
    EXPECT_EQ(pos, 7);

    hoo_string_release(str);
    hoo_string_release(needle);
}

TEST_F(StringBasicsTest, IndexOfNotFound) {
    HooString str = hoo_string_from_cstr("Hello, World!");
    HooString needle = hoo_string_from_cstr("xyz");

    int64_t pos = hoo_string_index_of(str, needle);
    EXPECT_EQ(pos, -1);

    hoo_string_release(str);
    hoo_string_release(needle);
}

TEST_F(StringBasicsTest, LastIndexOf) {
    HooString str = hoo_string_from_cstr("foo bar foo baz foo");
    HooString needle = hoo_string_from_cstr("foo");

    int64_t pos = hoo_string_last_index_of(str, needle);
    EXPECT_EQ(pos, 16);  // Last occurrence

    hoo_string_release(str);
    hoo_string_release(needle);
}

TEST_F(StringBasicsTest, Contains) {
    HooString str = hoo_string_from_cstr("Hello, World!");
    HooString needle1 = hoo_string_from_cstr("World");
    HooString needle2 = hoo_string_from_cstr("xyz");

    EXPECT_EQ(hoo_string_contains(str, needle1), 1);
    EXPECT_EQ(hoo_string_contains(str, needle2), 0);

    hoo_string_release(str);
    hoo_string_release(needle1);
    hoo_string_release(needle2);
}

TEST_F(StringBasicsTest, StartsWith) {
    HooString str = hoo_string_from_cstr("Hello, World!");
    HooString prefix1 = hoo_string_from_cstr("Hello");
    HooString prefix2 = hoo_string_from_cstr("World");

    EXPECT_EQ(hoo_string_starts_with(str, prefix1), 1);
    EXPECT_EQ(hoo_string_starts_with(str, prefix2), 0);

    hoo_string_release(str);
    hoo_string_release(prefix1);
    hoo_string_release(prefix2);
}

TEST_F(StringBasicsTest, EndsWith) {
    HooString str = hoo_string_from_cstr("Hello, World!");
    HooString suffix1 = hoo_string_from_cstr("World!");
    HooString suffix2 = hoo_string_from_cstr("Hello");

    EXPECT_EQ(hoo_string_ends_with(str, suffix1), 1);
    EXPECT_EQ(hoo_string_ends_with(str, suffix2), 0);

    hoo_string_release(str);
    hoo_string_release(suffix1);
    hoo_string_release(suffix2);
}

// ============================================================================
// String Comparison Tests
// ============================================================================

TEST_F(StringBasicsTest, Compare) {
    HooString str1 = hoo_string_from_cstr("apple");
    HooString str2 = hoo_string_from_cstr("banana");
    HooString str3 = hoo_string_from_cstr("apple");

    EXPECT_LT(hoo_string_compare(str1, str2), 0);  // apple < banana
    EXPECT_GT(hoo_string_compare(str2, str1), 0);  // banana > apple
    EXPECT_EQ(hoo_string_compare(str1, str3), 0);  // apple == apple

    hoo_string_release(str1);
    hoo_string_release(str2);
    hoo_string_release(str3);
}

TEST_F(StringBasicsTest, Equals) {
    HooString str1 = hoo_string_from_cstr("Hello");
    HooString str2 = hoo_string_from_cstr("Hello");
    HooString str3 = hoo_string_from_cstr("World");

    EXPECT_EQ(hoo_string_equals(str1, str2), 1);
    EXPECT_EQ(hoo_string_equals(str1, str3), 0);

    hoo_string_release(str1);
    hoo_string_release(str2);
    hoo_string_release(str3);
}

TEST_F(StringBasicsTest, EqualsIgnoreCase) {
    HooString str1 = hoo_string_from_cstr("Hello");
    HooString str2 = hoo_string_from_cstr("HELLO");
    HooString str3 = hoo_string_from_cstr("hello");

    EXPECT_EQ(hoo_string_equals_ignore_case(str1, str2), 1);
    EXPECT_EQ(hoo_string_equals_ignore_case(str1, str3), 1);
    EXPECT_EQ(hoo_string_equals_ignore_case(str2, str3), 1);

    hoo_string_release(str1);
    hoo_string_release(str2);
    hoo_string_release(str3);
}

// ============================================================================
// Reference Counting Tests
// ============================================================================

TEST_F(StringBasicsTest, Retain) {
    HooString str = hoo_string_from_cstr("Hello");
    EXPECT_EQ(hoo_string_refcount(str), 1);

    HooString retained = hoo_string_retain(str);
    EXPECT_EQ(retained, str);  // Same pointer
    EXPECT_EQ(hoo_string_refcount(str), 2);

    hoo_string_release(str);
    EXPECT_EQ(hoo_string_refcount(str), 1);  // Still alive

    hoo_string_release(str);  // Now freed
}

TEST_F(StringBasicsTest, RefcountTracking) {
    HooString str = hoo_string_from_cstr("Hello");

    // Build up refcount
    hoo_string_retain(str);
    hoo_string_retain(str);
    hoo_string_retain(str);
    EXPECT_EQ(hoo_string_refcount(str), 4);

    // Release down
    hoo_string_release(str);
    EXPECT_EQ(hoo_string_refcount(str), 3);

    hoo_string_release(str);
    hoo_string_release(str);
    hoo_string_release(str);
    // Last release frees memory
}

// ============================================================================
// Conversion Tests
// ============================================================================

TEST_F(StringBasicsTest, FromInt64) {
    HooString str = hoo_string_from_int64(42);
    EXPECT_STREQ(hoo_string_data(str), "42");
    hoo_string_release(str);

    str = hoo_string_from_int64(-12345);
    EXPECT_STREQ(hoo_string_data(str), "-12345");
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, FromDouble) {
    HooString str = hoo_string_from_double(3.14159);
    // Check that it starts with "3.14" (exact format may vary slightly)
    const char* data = hoo_string_data(str);
    EXPECT_TRUE(data[0] == '3' && data[1] == '.' && data[2] == '1');
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, FromBool) {
    HooString str_true = hoo_string_from_bool(1);
    EXPECT_STREQ(hoo_string_data(str_true), "true");
    hoo_string_release(str_true);

    HooString str_false = hoo_string_from_bool(0);
    EXPECT_STREQ(hoo_string_data(str_false), "false");
    hoo_string_release(str_false);
}

TEST_F(StringBasicsTest, ToInt64) {
    HooString str = hoo_string_from_cstr("12345");
    EXPECT_EQ(hoo_string_to_int64(str), 12345);
    hoo_string_release(str);

    str = hoo_string_from_cstr("invalid");
    EXPECT_EQ(hoo_string_to_int64(str), 0);
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, ToDouble) {
    HooString str = hoo_string_from_cstr("3.14159");
    double val = hoo_string_to_double(str);
    EXPECT_NEAR(val, 3.14159, 0.0001);
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, Format) {
    HooString str = hoo_string_format("Hello %s, you are %d years old", "Alice", 30);
    EXPECT_STREQ(hoo_string_data(str), "Hello Alice, you are 30 years old");
    hoo_string_release(str);
}

// ============================================================================
// Debugging Tests
// ============================================================================

TEST_F(StringBasicsTest, Debug) {
    HooString str = hoo_string_from_cstr("Hello");
    HooString debug_info = hoo_string_debug(str);

    const char* info = hoo_string_data(debug_info);
    EXPECT_TRUE(info != nullptr);
    EXPECT_TRUE(std::string(info).find("HooString") != std::string::npos);

    hoo_string_release(str);
    hoo_string_release(debug_info);
}

TEST_F(StringBasicsTest, PrintAndPrintln) {
    // These should not crash
    HooString str = hoo_string_from_cstr("Test output");

    hoo_string_print(str);      // Should output "Test output"
    hoo_string_println(str);    // Should output "Test output\n"

    hoo_string_release(str);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StringBasicsTest, EmptyStringOperations) {
    HooString empty1 = hoo_string_new();
    HooString empty2 = hoo_string_new();
    HooString str = hoo_string_from_cstr("Hello");

    // Concatenation with empty
    HooString result = hoo_string_concat(empty1, str);
    EXPECT_STREQ(hoo_string_data(result), "Hello");
    hoo_string_release(result);

    result = hoo_string_concat(str, empty2);
    EXPECT_STREQ(hoo_string_data(result), "Hello");
    hoo_string_release(result);

    // Comparison with empty
    EXPECT_EQ(hoo_string_equals(empty1, empty2), 1);
    EXPECT_EQ(hoo_string_equals(empty1, str), 0);

    hoo_string_release(empty1);
    hoo_string_release(empty2);
    hoo_string_release(str);
}

TEST_F(StringBasicsTest, LargeString) {
    // Create a large string (10KB)
    std::string large_text(10000, 'a');
    HooString str = hoo_string_from_cstr(large_text.c_str());

    EXPECT_EQ(hoo_string_length(str), 10000);
    EXPECT_EQ(hoo_string_byte_at(str, 5000), 'a');
    EXPECT_EQ(hoo_string_byte_at(str, 9999), 'a');

    hoo_string_release(str);
}

TEST_F(StringBasicsTest, UTF8Characters) {
    // Test that UTF-8 is preserved (though operations may be byte-based)
    HooString str = hoo_string_from_cstr("Hello 世界");  // "Hello World" in Chinese

    EXPECT_GT(hoo_string_length(str), 5);  // More than just "Hello"
    EXPECT_TRUE(hoo_string_contains(str, hoo_string_from_cstr("Hello")));

    hoo_string_release(str);
}
