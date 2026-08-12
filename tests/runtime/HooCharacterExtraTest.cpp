#include <gtest/gtest.h>
#include "runtime/lib/character/hoo_character.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"

class HooCharacterExtraTest : public ::testing::Test {
protected:
    void SetUp() override { hoo_reset_memory_stats(); }
};

TEST_F(HooCharacterExtraTest, StringFromAnyCharacter) {
    HooCharacter ch = hoo_character_from_codepoint(0x41); // 'A'
    // Use hoo_string_from_any with type HOO_TYPE_CHARACTER
    HooString s = hoo_string_from_any((int64_t)ch, HOO_TYPE_CHARACTER);
    ASSERT_NE(nullptr, s);
    EXPECT_STREQ("A", hoo_string_data(s));
    // Clean up
    hoo_string_release(s);
    hoo_character_release(ch);
}

TEST_F(HooCharacterExtraTest, InvalidCodepointHandled) {
    // Pass invalid codepoint, will get replacement character U+FFFD ("�")
    HooCharacter ch = hoo_character_from_codepoint(0x110000); // beyond Unicode range
    ASSERT_NE(nullptr, ch);
    // The replacement character should be returned as a string
    HooString s = hoo_string_from_any((int64_t)ch, HOO_TYPE_CHARACTER);
    ASSERT_NE(nullptr, s);
    // UTF-8 for replacement char is 0xEF 0xBF 0xBD, which prints as �
    EXPECT_EQ(3, hoo_string_length(s));
    // We don't check exact bytes, just that length is 3 and not empty
    hoo_string_release(s);
    hoo_character_release(ch);
}
