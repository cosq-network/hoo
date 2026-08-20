#include <gtest/gtest.h>
#include "runtime/lib/io/hoo_io.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/character/hoo_character.h"

class HooIOTest : public ::testing::Test {
};

TEST_F(HooIOTest, PrintValidString) {
    HooString str = hoo_string_from_cstr("Hello IO");
    hoo_print(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintNullptr) {
    hoo_print(nullptr);
}

TEST_F(HooIOTest, PrintEmptyString) {
    HooString str = hoo_string_new();
    EXPECT_EQ(hoo_string_is_empty(str), 1);
    hoo_print(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintlnValidString) {
    HooString str = hoo_string_from_cstr("Hello println");
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintlnNullptr) {
    hoo_println(nullptr);
}

TEST_F(HooIOTest, PrintlnEmptyString) {
    HooString str = hoo_string_new();
    EXPECT_EQ(hoo_string_is_empty(str), 1);
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintLongString) {
    std::string longStr(1000, 'A');
    HooString str = hoo_string_from_cstr(longStr.c_str());
    EXPECT_EQ(hoo_string_length(str), 1000);
    hoo_print(str);
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintUnicodeString) {
    HooString str = hoo_string_from_cstr("Hello \xe4\xb8\x96\xe7\x95\x8c");
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, ReadCharNonBlocking) {
    // When no input is available, readchar should return NULL without blocking
    HooCharacter ch = hoo_readchar();
    // If no input is available, the function returns NULL.
    if (ch == NULL) {
        SUCCEED();
    } else {
        // Ensure the returned object is a managed Character with correct type ID.
        EXPECT_EQ(hoo_get_type_id(ch), HOO_TYPE_CHARACTER);
        hoo_character_release(ch);
    }
}

TEST_F(HooIOTest, ReadLineReturnsString) {
    // hoo_readline reads from stdin, which is not available in unit tests.
    // When stdin is not a tty, it may return empty or block.
    // We just verify the function doesn't crash.
    // Note: This test may produce different results depending on the test runner.
    // In a CI environment with no stdin, it should return an empty string.
    void* result = hoo_readline();
    if (result != nullptr) {
        // If we got a result, it should be a valid HooString
        EXPECT_EQ(hoo_get_type_id(result), HOO_TYPE_STRING);
        hoo_string_release(result);
    }
}

TEST_F(HooIOTest, PrintMultipleTimes) {
    HooString str = hoo_string_from_cstr("multi");
    for (int i = 0; i < 10; i++) {
        hoo_print(str);
    }
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintSpecialCharacters) {
    HooString str = hoo_string_from_cstr("line1\nline2\ttab");
    hoo_println(str);
    hoo_string_release(str);
}

TEST_F(HooIOTest, PrintSingleChar) {
    HooString str = hoo_string_from_cstr("X");
    hoo_print(str);
    hoo_string_release(str);
}
