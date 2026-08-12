#include <gtest/gtest.h>
#include <cstring>
#include "runtime/lib/uuid/hoo_uuid.h"

class HooUuidTest : public ::testing::Test {
protected:
    void TearDown() override {
        if (uuid_) {
            hoo_uuid_release(uuid_);
            uuid_ = nullptr;
        }
    }
    HooUUID uuid_ = nullptr;
};

TEST_F(HooUuidTest, V4) {
    uuid_ = hoo_uuid_v4();
    ASSERT_NE(nullptr, uuid_);
}

TEST_F(HooUuidTest, V4Format) {
    HooUUID u = hoo_uuid_v4();
    ASSERT_NE(nullptr, u);
    char* s = hoo_uuid_to_string(u);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(36, strlen(s));
    EXPECT_EQ('-', s[8]);
    EXPECT_EQ('-', s[13]);
    EXPECT_EQ('-', s[18]);
    EXPECT_EQ('-', s[23]);
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) continue;
        EXPECT_TRUE((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))
            << "Non-hex character at position " << i << ": " << s[i];
    }
    hoo_uuid_free_string(s);
    hoo_uuid_release(u);
}

TEST_F(HooUuidTest, V4Variant) {
    HooUUID u = hoo_uuid_v4();
    ASSERT_NE(nullptr, u);
    uint8_t bytes[16];
    int64_t ret = hoo_uuid_to_bytes(u, bytes);
    ASSERT_EQ(1, ret);
    EXPECT_EQ(4, (bytes[6] >> 4) & 0x0f);
    hoo_uuid_release(u);
}

TEST_F(HooUuidTest, Nil) {
    HooUUID u = hoo_uuid_nil();
    ASSERT_NE(nullptr, u);
    EXPECT_EQ(1, hoo_uuid_is_nil(u));
    char* s = hoo_uuid_to_string(u);
    ASSERT_NE(nullptr, s);
    EXPECT_STREQ("00000000-0000-0000-0000-000000000000", s);
    hoo_uuid_free_string(s);
    hoo_uuid_release(u);
}

TEST_F(HooUuidTest, FromString) {
    const char* expected = "550e8400-e29b-41d4-a716-446655440000";
    HooUUID u = hoo_uuid_from_string(expected);
    ASSERT_NE(nullptr, u);
    char* s = hoo_uuid_to_string(u);
    ASSERT_NE(nullptr, s);
    EXPECT_STREQ(expected, s);
    hoo_uuid_free_string(s);
    hoo_uuid_release(u);
}

TEST_F(HooUuidTest, FromStringInvalid) {
    HooUUID u = hoo_uuid_from_string("not-a-uuid");
    EXPECT_EQ(nullptr, u);
    u = hoo_uuid_from_string("550e8400-e29b-41d4-a716-44665544000Z");
    EXPECT_EQ(nullptr, u);
    u = hoo_uuid_from_string("");
    EXPECT_EQ(nullptr, u);
}

TEST_F(HooUuidTest, Equals) {
    const char* s = "550e8400-e29b-41d4-a716-446655440000";
    HooUUID a = hoo_uuid_from_string(s);
    HooUUID b = hoo_uuid_from_string(s);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(1, hoo_uuid_equals(a, b));
    hoo_uuid_release(a);
    hoo_uuid_release(b);
}

TEST_F(HooUuidTest, NotEquals) {
    HooUUID a = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440000");
    HooUUID b = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440001");
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(0, hoo_uuid_equals(a, b));
    hoo_uuid_release(a);
    hoo_uuid_release(b);
}

TEST_F(HooUuidTest, Compare) {
    HooUUID a = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440000");
    HooUUID b = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440000");
    HooUUID c = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440001");
    HooUUID d = hoo_uuid_from_string("550e8400-e29b-41d4-a716-446655440000");
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(0, hoo_uuid_compare(a, b));
    EXPECT_EQ(-1, hoo_uuid_compare(a, c))
        << "Expected " << hoo_uuid_to_string(a) << " < " << hoo_uuid_to_string(c)
        << " but got " << hoo_uuid_compare(a, c);
    EXPECT_EQ(1, hoo_uuid_compare(c, a));
    hoo_uuid_release(a);
    hoo_uuid_release(b);
    hoo_uuid_release(c);
    hoo_uuid_release(d);
}

TEST_F(HooUuidTest, ToBytes) {
    HooUUID u = hoo_uuid_v4();
    ASSERT_NE(nullptr, u);
    uint8_t bytes[16];
    int64_t ret = hoo_uuid_to_bytes(u, bytes);
    EXPECT_EQ(1, ret);
    HooUUID u2 = hoo_uuid_from_bytes(bytes);
    ASSERT_NE(nullptr, u2);
    char* s1 = hoo_uuid_to_string(u);
    char* s2 = hoo_uuid_to_string(u2);
    ASSERT_NE(nullptr, s1);
    ASSERT_NE(nullptr, s2);
    EXPECT_STREQ(s1, s2);
    hoo_uuid_free_string(s1);
    hoo_uuid_free_string(s2);
    hoo_uuid_release(u);
    hoo_uuid_release(u2);
}

TEST_F(HooUuidTest, RetainRelease) {
    HooUUID u = hoo_uuid_v4();
    ASSERT_NE(nullptr, u);
    hoo_uuid_retain(u);
    hoo_uuid_release(u);
    hoo_uuid_release(u);
}

TEST_F(HooUuidTest, FreeString) {
    hoo_uuid_free_string(nullptr);
    HooUUID u = hoo_uuid_v4();
    ASSERT_NE(nullptr, u);
    char* s = hoo_uuid_to_string(u);
    ASSERT_NE(nullptr, s);
    hoo_uuid_free_string(s);
    hoo_uuid_release(u);
}
