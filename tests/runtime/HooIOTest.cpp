#include <gtest/gtest.h>
#include "runtime/lib/io/hoo_io.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/character/hoo_character.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

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
    // Redirect stdin to a temporary file with no content to simulate EOF
    FILE* temp = tmpfile();
#ifdef _WIN32
    int old_stdin = _dup(_fileno(stdin));
    _dup2(_fileno(temp), _fileno(stdin));
#else
    int old_stdin = dup(fileno(stdin));
    dup2(fileno(temp), fileno(stdin));
#endif

    HooCharacter ch = hoo_readchar();
    if (ch == NULL) {
        SUCCEED();
    } else {
        EXPECT_EQ(hoo_get_type_id(ch), HOO_TYPE_CHARACTER);
        hoo_character_release(ch);
    }

    // Restore stdin
#ifdef _WIN32
    _dup2(old_stdin, _fileno(stdin));
    _close(old_stdin);
#else
    dup2(old_stdin, fileno(stdin));
    close(old_stdin);
#endif
    fclose(temp);
}

TEST_F(HooIOTest, ReadLineReturnsString) {
    // Redirect stdin to a temporary file to prevent blocking
    FILE* temp = tmpfile();
#ifdef _WIN32
    int old_stdin = _dup(_fileno(stdin));
    _dup2(_fileno(temp), _fileno(stdin));
#else
    int old_stdin = dup(fileno(stdin));
    dup2(fileno(temp), fileno(stdin));
#endif

    void* result = hoo_readline();
    if (result != nullptr) {
        EXPECT_EQ(hoo_get_type_id(result), HOO_TYPE_STRING);
        hoo_string_release(result);
    }

    // Restore stdin
#ifdef _WIN32
    _dup2(old_stdin, _fileno(stdin));
    _close(old_stdin);
#else
    dup2(old_stdin, fileno(stdin));
    close(old_stdin);
#endif
    fclose(temp);
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
