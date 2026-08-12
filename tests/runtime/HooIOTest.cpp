#include <gtest/gtest.h>
#include "runtime/lib/io/hoo_io.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"

class HooIOTest : public ::testing::Test {
};

TEST_F(HooIOTest, BasicPrint) {
    HooString str = hoo_string_from_cstr("Hello IO");
    // We can't easily capture stdout/stderr in this environment without complex mocking,
    // but we can call them to ensure no crashes and coverage.
    hoo_print(str);
    hoo_println(str);
    hoo_println(nullptr);
    
    hoo_string_release(str);
}

#include "runtime/lib/character/hoo_character.h"

TEST_F(HooIOTest, ReadChar) {
    // Basic test to ensure it returns a Character (or NULL if no input)
    HooCharacter ch = hoo_readchar();
    // If no input is available, the function returns NULL.
    if (ch == NULL) {
        SUCCEED();
    } else {
        // Ensure the returned object is a managed Character with correct type ID.
        EXPECT_EQ(hoo_get_type_id(ch), HOO_TYPE_CHARACTER);
        // Release the reference we obtained.
        hoo_character_release(ch);
    }
}


