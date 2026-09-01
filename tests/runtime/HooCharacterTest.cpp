#include <gtest/gtest.h>
#include "runtime/lib/text/hoo_character.h"
#include "runtime/lib/text/hoo_string.h"
#include "runtime/lib/mem/hoo_generic_array.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <cstring>

class HooCharacterTest : public ::testing::Test {
protected:
    void SetUp() override {
        hoo_reset_memory_stats();
    }
};

TEST_F(HooCharacterTest, CreateFromUtf8) {
    // ASCII character 'A' (1 byte)
    HooCharacter c1 = hoo_character_from_utf8("A", 1);
    ASSERT_NE(nullptr, c1);
    EXPECT_EQ(1, hoo_character_length(c1));
    EXPECT_STREQ("A", hoo_character_data(c1));
    EXPECT_EQ(65, hoo_character_codepoint(c1));
    EXPECT_EQ(HOO_TYPE_CHARACTER, hoo_get_type_id(c1));
    hoo_character_release(c1);

    // Multi-byte character '€' (3 bytes: E2 82 AC)
    const char* euro = "\xE2\x82\xAC";
    HooCharacter c2 = hoo_character_from_utf8(euro, 3);
    ASSERT_NE(nullptr, c2);
    EXPECT_EQ(3, hoo_character_length(c2));
    EXPECT_STREQ(euro, hoo_character_data(c2));
    EXPECT_EQ(0x20AC, hoo_character_codepoint(c2));
    hoo_character_release(c2);
}

TEST_F(HooCharacterTest, CreateFromUtf8FirstSequence) {
    // A buffer longer than a single sequence takes the first scalar value.
    HooCharacter c1 = hoo_character_from_utf8("abcde", 5);
    ASSERT_NE(nullptr, c1);
    EXPECT_EQ(1, hoo_character_length(c1));
    EXPECT_EQ('a', hoo_character_codepoint(c1));
    hoo_character_release(c1);

    const char* emojiStr = "\xF0\x9F\x98\x80x"; // 😀 then 'x'
    HooCharacter c2 = hoo_character_from_utf8(emojiStr, 5);
    ASSERT_NE(nullptr, c2);
    EXPECT_EQ(4, hoo_character_length(c2));
    EXPECT_EQ(0x1F600, hoo_character_codepoint(c2));
    hoo_character_release(c2);
}

TEST_F(HooCharacterTest, CreateFromUtf8RejectsInvalid) {
    // Null input.
    EXPECT_EQ(nullptr, hoo_character_from_utf8(nullptr, 1));
    // Empty input.
    EXPECT_EQ(nullptr, hoo_character_from_utf8("", 0));
    // Invalid lead byte (0xFF is never a UTF-8 lead byte).
    EXPECT_EQ(nullptr, hoo_character_from_utf8("\xFF", 1));
    // Lone continuation byte (0x80).
    EXPECT_EQ(nullptr, hoo_character_from_utf8("\x80", 1));
    // Truncated 3-byte sequence (only the lead byte is present).
    EXPECT_EQ(nullptr, hoo_character_from_utf8("\xE2", 1));
    // Truncated 4-byte sequence (lead byte plus one continuation byte).
    EXPECT_EQ(nullptr, hoo_character_from_utf8("\xF0\x9F", 2));
    // Missing continuation byte (0x41 is not a continuation byte).
    EXPECT_EQ(nullptr, hoo_character_from_utf8("\xE2\x41\xAC", 3));
}

TEST_F(HooCharacterTest, CreateFromCodepointRejectsInvalid) {
    // Negative values map to U+FFFD.
    HooCharacter c1 = hoo_character_from_codepoint(-1);
    ASSERT_NE(nullptr, c1);
    EXPECT_EQ(0xFFFD, hoo_character_codepoint(c1));
    hoo_character_release(c1);

    // UTF-16 surrogates are not scalar values and map to U+FFFD.
    HooCharacter c2 = hoo_character_from_codepoint(0xD800);
    ASSERT_NE(nullptr, c2);
    EXPECT_EQ(0xFFFD, hoo_character_codepoint(c2));
    hoo_character_release(c2);

    HooCharacter c3 = hoo_character_from_codepoint(0xDFFF);
    ASSERT_NE(nullptr, c3);
    EXPECT_EQ(0xFFFD, hoo_character_codepoint(c3));
    hoo_character_release(c3);

    // Values above U+10FFFF map to U+FFFD (pre-existing behavior).
    HooCharacter c4 = hoo_character_from_codepoint(0x110000);
    ASSERT_NE(nullptr, c4);
    EXPECT_EQ(0xFFFD, hoo_character_codepoint(c4));
    hoo_character_release(c4);
}

TEST_F(HooCharacterTest, CreateFromCodepoint) {
    // ASCII 'Z'
    HooCharacter c1 = hoo_character_from_codepoint(90);
    ASSERT_NE(nullptr, c1);
    EXPECT_EQ(1, hoo_character_length(c1));
    EXPECT_STREQ("Z", hoo_character_data(c1));
    EXPECT_EQ(90, hoo_character_codepoint(c1));
    hoo_character_release(c1);

    // Emoji '😀' (U+1F600, 4 bytes)
    HooCharacter c2 = hoo_character_from_codepoint(0x1F600);
    ASSERT_NE(nullptr, c2);
    EXPECT_EQ(4, hoo_character_length(c2));
    EXPECT_EQ(0x1F600, hoo_character_codepoint(c2));
    hoo_character_release(c2);
}

TEST_F(HooCharacterTest, ReferenceCounting) {
    HooCharacter c = hoo_character_from_codepoint('X');
    EXPECT_EQ(1, hoo_character_refcount(c));
    
    hoo_character_retain(c);
    EXPECT_EQ(2, hoo_character_refcount(c));
    
    hoo_character_release(c);
    EXPECT_EQ(1, hoo_character_refcount(c));
    
    hoo_character_release(c);
}

TEST_F(HooCharacterTest, StringToCharacters) {
    HooString s = hoo_string_from_cstr("A€😀");
    HooArray chars = hoo_string_to_characters(s);
    
    ASSERT_NE(nullptr, chars);
    EXPECT_EQ(3, hoo_array_length(chars));
    
    HooCharacter c1 = nullptr, c2 = nullptr, c3 = nullptr;
    hoo_array_get_object(chars, 0, (void**)&c1);
    hoo_array_get_object(chars, 1, (void**)&c2);
    hoo_array_get_object(chars, 2, (void**)&c3);
    
    ASSERT_NE(nullptr, c1);
    ASSERT_NE(nullptr, c2);
    ASSERT_NE(nullptr, c3);
    
    EXPECT_EQ(65, hoo_character_codepoint(c1));
    EXPECT_EQ(0x20AC, hoo_character_codepoint(c2));
    EXPECT_EQ(0x1F600, hoo_character_codepoint(c3));
    
    hoo_array_release(chars);
    hoo_string_release(s);
}
