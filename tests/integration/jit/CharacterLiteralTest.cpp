#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/character/hoo_character.h"
#include "runtime/lib/runtime/hoo_runtime.h"

using namespace hooc;

class CharacterLiteralTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};

    void SetUp() override {
    }
};

TEST_F(CharacterLiteralTest, LowerCharacterLiteral) {
    const std::string source = R"(
        import hoo.character;
        func :Character getChar() {
            return 'A';
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    // Mangled name: _F_M_test_E_getChar_p
    // _F_ : Function
    // M_test : Module test
    // E_getChar : Entry getChar
    // _p : returns pointer (Character object)
    int64_t result = jit.run("_F_M_test_E_getChar_p");
    
    ASSERT_NE(-1, result) << jit.getLastError();
    ASSERT_NE(0, result) << jit.getLastError();
    HooCharacter ch = (HooCharacter)result;
    
    EXPECT_EQ(65, hoo_character_codepoint(ch));
    EXPECT_EQ(1, hoo_character_length(ch));
    EXPECT_STREQ("A", hoo_character_data(ch));
    EXPECT_EQ(HOO_TYPE_CHARACTER, hoo_get_type_id(ch));
    
    hoo_character_release(ch);
}

TEST_F(CharacterLiteralTest, LowerMultiByteCharacterLiteral) {
    const std::string source = R"(
        import hoo.character;
        func :Character getEuro() {
            return '€';
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_getEuro_p");
    
    ASSERT_NE(-1, result) << jit.getLastError();
    ASSERT_NE(0, result) << jit.getLastError();
    HooCharacter ch = (HooCharacter)result;
    
    EXPECT_EQ(0x20AC, hoo_character_codepoint(ch));
    EXPECT_EQ(3, hoo_character_length(ch));
    EXPECT_STREQ("\xE2\x82\xAC", hoo_character_data(ch));
    
    hoo_character_release(ch);
}
