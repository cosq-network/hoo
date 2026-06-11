#include <gtest/gtest.h>
#include "runtime/lib/hoo_map.h"

class HooMapTest : public ::testing::Test {
};

TEST_F(HooMapTest, Int64KeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_INT64);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_length(map), 0);
    EXPECT_TRUE(hoo_map_empty(map));
    EXPECT_EQ(hoo_map_key_type(map), HOO_MAP_KEY_INT64);
    EXPECT_EQ(hoo_map_value_type(map), HOO_MAP_VAL_INT64);

    hoo_map_set_int64_int64(map, 10, 100);
    hoo_map_set_int64_int64(map, 20, 200);

    EXPECT_EQ(hoo_map_length(map), 2);
    EXPECT_FALSE(hoo_map_empty(map));
    EXPECT_EQ(hoo_map_contains_int64(map, 10), 1);
    EXPECT_EQ(hoo_map_contains_int64(map, 15), 0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_int64_int64(map, 10, &val), 1);
    EXPECT_EQ(val, 100);

    hoo_map_remove_int64(map, 10);
    EXPECT_EQ(hoo_map_length(map), 1);
    EXPECT_EQ(hoo_map_contains_int64(map, 10), 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);

    hoo_map_set_string_string(map, "key1", "value1");
    hoo_map_set_string_string(map, "key2", "value2");

    const char* sval = nullptr;
    EXPECT_EQ(hoo_map_get_string_string(map, "key1", &sval), 1);
    EXPECT_STREQ(sval, "value1");

    EXPECT_EQ(hoo_map_contains_string(map, "key1"), 1);
    EXPECT_EQ(hoo_map_contains_string(map, "missing"), 0);

    hoo_map_remove_string(map, "key1");
    EXPECT_EQ(hoo_map_contains_string(map, "key1"), 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_INT64);

    hoo_map_set_string_int64(map, "count", 42);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_string_int64(map, "count", &val), 1);
    EXPECT_EQ(val, 42);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_DOUBLE);

    hoo_map_set_string_double(map, "pi", 3.14159);

    double val = 0.0;
    EXPECT_EQ(hoo_map_get_string_double(map, "pi", &val), 1);
    EXPECT_DOUBLE_EQ(val, 3.14159);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_BOOL);

    hoo_map_set_string_bool(map, "flag", 1);
    hoo_map_set_string_bool(map, "noflag", 0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_string_bool(map, "flag", &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_get_string_bool(map, "noflag", &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_OBJECT);

    int dummy = 123;
    hoo_map_set_string_object(map, "obj", &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_get_string_object(map, "obj", &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_DOUBLE);

    hoo_map_set_int64_double(map, 1, 2.718);

    double val = 0.0;
    EXPECT_EQ(hoo_map_get_int64_double(map, 1, &val), 1);
    EXPECT_DOUBLE_EQ(val, 2.718);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_STRING);

    hoo_map_set_int64_string(map, 1, "hello");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_get_int64_string(map, 1, &val), 1);
    EXPECT_STREQ(val, "hello");

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_BOOL);

    hoo_map_set_int64_bool(map, 1, 1);
    hoo_map_set_int64_bool(map, 2, 0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_int64_bool(map, 1, &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_get_int64_bool(map, 2, &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_OBJECT);

    int dummy = 456;
    hoo_map_set_int64_object(map, 1, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_get_int64_object(map, 1, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_INT64);

    hoo_map_set_char_int64(map, 'A', 65);
    hoo_map_set_char_int64(map, 'B', 66);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_char_int64(map, 'A', &val), 1);
    EXPECT_EQ(val, 65);
    EXPECT_EQ(hoo_map_get_char_int64(map, 'B', &val), 1);
    EXPECT_EQ(val, 66);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_DOUBLE);

    hoo_map_set_char_double(map, 'x', 1.23);

    double val = 0.0;
    EXPECT_EQ(hoo_map_get_char_double(map, 'x', &val), 1);
    EXPECT_DOUBLE_EQ(val, 1.23);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_STRING);

    hoo_map_set_char_string(map, 'g', "greeting");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_get_char_string(map, 'g', &val), 1);
    EXPECT_STREQ(val, "greeting");

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_BOOL);

    hoo_map_set_char_bool(map, 'y', 1);
    hoo_map_set_char_bool(map, 'n', 0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_char_bool(map, 'y', &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_get_char_bool(map, 'n', &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_OBJECT);

    int dummy = 789;
    hoo_map_set_char_object(map, 'o', &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_get_char_object(map, 'o', &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_INT64);

    hoo_map_set_int8_int64(map, 7, 77);
    hoo_map_set_int8_int64(map, 8, 88);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_int8_int64(map, 7, &val), 1);
    EXPECT_EQ(val, 77);
    EXPECT_EQ(hoo_map_get_int8_int64(map, 8, &val), 1);
    EXPECT_EQ(val, 88);

    EXPECT_EQ(hoo_map_contains_int64(map, 7), 0); // int8 key, not int64 key
    EXPECT_EQ(hoo_map_contains_int8(map, 7), 1);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_DOUBLE);

    hoo_map_set_int8_double(map, 1, 1.5);

    double val = 0.0;
    EXPECT_EQ(hoo_map_get_int8_double(map, 1, &val), 1);
    EXPECT_DOUBLE_EQ(val, 1.5);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_STRING);

    hoo_map_set_int8_string(map, 1, "test");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_get_int8_string(map, 1, &val), 1);
    EXPECT_STREQ(val, "test");

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_BOOL);

    hoo_map_set_int8_bool(map, 1, 1);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_int8_bool(map, 1, &val), 1);
    EXPECT_EQ(val, 1);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_OBJECT);

    int dummy = 999;
    hoo_map_set_int8_object(map, 1, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_get_int8_object(map, 1, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, GenericValueOps) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_OBJECT);

    int dummy = 111;
    hoo_map_set_int64_value(map, 1, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_get_int64_value(map, 1, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Clear) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_INT64);
    hoo_map_set_int64_int64(map, 1, 10);
    hoo_map_set_int64_int64(map, 2, 20);
    EXPECT_EQ(hoo_map_length(map), 2);

    hoo_map_clear(map);
    EXPECT_EQ(hoo_map_length(map), 0);
    EXPECT_TRUE(hoo_map_empty(map));

    hoo_map_release(map);
}

TEST_F(HooMapTest, ARC) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_INT64);
    EXPECT_EQ(hoo_map_refcount(map), 1);

    hoo_map_retain(map);
    EXPECT_EQ(hoo_map_refcount(map), 2);

    hoo_map_release(map);
    EXPECT_EQ(hoo_map_refcount(map), 1);

    hoo_map_release(map);
}

TEST_F(HooMapTest, NullHandling) {
    EXPECT_EQ(hoo_map_length(nullptr), 0);
    EXPECT_EQ(hoo_map_empty(nullptr), 1);
    EXPECT_EQ(hoo_map_key_type(nullptr), -1);
    EXPECT_EQ(hoo_map_value_type(nullptr), -1);
}

TEST_F(HooMapTest, ValueTypeAny) {
    HooMap map = hoo_map_new_with_keytype(HOO_MAP_KEY_INT64);
    EXPECT_EQ(hoo_map_value_type(map), HOO_MAP_VAL_ANY);
    hoo_map_release(map);
}
