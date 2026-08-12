#include <gtest/gtest.h>
#include "runtime/lib/character/hoo_character.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/generic_array/hoo_generic_array.h"
#include "runtime/lib/runtime/hoo_runtime.h"
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
