#include <gtest/gtest.h>
#include "runtime/lib/map/hoo_map.h"

class HooMapTest : public ::testing::Test {
};

TEST_F(HooMapTest, Int64KeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_INT64);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 0);
    EXPECT_TRUE(hoo_map_is_empty(map));
    EXPECT_EQ(hoo_map_key_type(map), HOO_MAP_KEY_INT64);
    EXPECT_EQ(hoo_map_value_type(map), HOO_MAP_VAL_INT64);

    int64_t k10 = 10, k20 = 20;
    int64_t v100 = 100, v200 = 200;
    hoo_map_set(map, &k10, &v100);
    hoo_map_set(map, &k20, &v200);

    EXPECT_EQ(hoo_map_count(map), 2);
    EXPECT_FALSE(hoo_map_is_empty(map));
    EXPECT_EQ(hoo_map_contains_key(map, &k10), 1);
    int64_t k15 = 15;
    EXPECT_EQ(hoo_map_contains_key(map, &k15), 0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &k10, &val), 1);
    EXPECT_EQ(val, 100);

    hoo_map_remove(map, &k10);
    EXPECT_EQ(hoo_map_count(map), 1);
    EXPECT_EQ(hoo_map_contains_key(map, &k10), 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);

    hoo_map_set(map, "key1", "value1");
    hoo_map_set(map, "key2", "value2");

    const char* sval = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, "key1", &sval), 1);
    EXPECT_STREQ(sval, "value1");

    EXPECT_EQ(hoo_map_contains_key(map, "key1"), 1);
    EXPECT_EQ(hoo_map_contains_key(map, "missing"), 0);

    hoo_map_remove(map, "key1");
    EXPECT_EQ(hoo_map_contains_key(map, "key1"), 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_INT64);

    int64_t v42 = 42;
    hoo_map_set(map, "count", &v42);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, "count", &val), 1);
    EXPECT_EQ(val, 42);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_DOUBLE);

    double pi = 3.14159;
    hoo_map_set(map, "pi", &pi);

    double val = 0.0;
    EXPECT_EQ(hoo_map_try_get(map, "pi", &val), 1);
    EXPECT_DOUBLE_EQ(val, 3.14159);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_BOOL);

    int64_t v1 = 1, v0 = 0;
    hoo_map_set(map, "flag", &v1);
    hoo_map_set(map, "noflag", &v0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, "flag", &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_try_get(map, "noflag", &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, StringKeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_OBJECT);

    int dummy = 123;
    hoo_map_set(map, "obj", &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, "obj", &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_DOUBLE);

    int64_t k1 = 1;
    double v2718 = 2.718;
    hoo_map_set(map, &k1, &v2718);

    double val = 0.0;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_DOUBLE_EQ(val, 2.718);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_STRING);

    int64_t k1 = 1;
    hoo_map_set(map, &k1, "hello");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_STREQ(val, "hello");

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_BOOL);

    int64_t k1 = 1, k2 = 2;
    int64_t v1 = 1, v0 = 0;
    hoo_map_set(map, &k1, &v1);
    hoo_map_set(map, &k2, &v0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_try_get(map, &k2, &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int64KeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_OBJECT);

    int dummy = 456;
    int64_t k1 = 1;
    hoo_map_set(map, &k1, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_INT64);

    // Char keys are passed as int64* with the char value
    int64_t kA = 'A', kB = 'B';
    int64_t v65 = 65, v66 = 66;
    hoo_map_set(map, &kA, &v65);
    hoo_map_set(map, &kB, &v66);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &kA, &val), 1);
    EXPECT_EQ(val, 65);
    EXPECT_EQ(hoo_map_try_get(map, &kB, &val), 1);
    EXPECT_EQ(val, 66);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_DOUBLE);

    int64_t kx = 'x';
    double v123 = 1.23;
    hoo_map_set(map, &kx, &v123);

    double val = 0.0;
    EXPECT_EQ(hoo_map_try_get(map, &kx, &val), 1);
    EXPECT_DOUBLE_EQ(val, 1.23);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_STRING);

    int64_t kg = 'g';
    hoo_map_set(map, &kg, "greeting");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &kg, &val), 1);
    EXPECT_STREQ(val, "greeting");

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_BOOL);

    int64_t ky = 'y', kn = 'n';
    int64_t v1 = 1, v0 = 0;
    hoo_map_set(map, &ky, &v1);
    hoo_map_set(map, &kn, &v0);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &ky, &val), 1);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(hoo_map_try_get(map, &kn, &val), 1);
    EXPECT_EQ(val, 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, CharKeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_CHAR, HOO_MAP_VAL_OBJECT);

    int dummy = 789;
    int64_t ko = 'o';
    hoo_map_set(map, &ko, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &ko, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyInt64Val) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_INT64);

    int8_t k7 = 7, k8 = 8;
    int64_t v77 = 77, v88 = 88;
    hoo_map_set(map, &k7, &v77);
    hoo_map_set(map, &k8, &v88);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &k7, &val), 1);
    EXPECT_EQ(val, 77);
    EXPECT_EQ(hoo_map_try_get(map, &k8, &val), 1);
    EXPECT_EQ(val, 88);

    int8_t k7check = 7, k15check = 15;
    EXPECT_EQ(hoo_map_contains_key(map, &k7check), 1);
    EXPECT_EQ(hoo_map_contains_key(map, &k15check), 0);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyDoubleVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_DOUBLE);

    int8_t k1 = 1;
    double v15 = 1.5;
    hoo_map_set(map, &k1, &v15);

    double val = 0.0;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_DOUBLE_EQ(val, 1.5);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyStringVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_STRING);

    int8_t k1 = 1;
    hoo_map_set(map, &k1, "test");

    const char* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_STREQ(val, "test");

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyBoolVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_BOOL);

    int8_t k1 = 1;
    int64_t v1 = 1;
    hoo_map_set(map, &k1, &v1);

    int64_t val = 0;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_EQ(val, 1);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Int8KeyObjectVal) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8, HOO_MAP_VAL_OBJECT);

    int dummy = 999;
    int8_t k1 = 1;
    hoo_map_set(map, &k1, &dummy);

    void* val = nullptr;
    EXPECT_EQ(hoo_map_try_get(map, &k1, &val), 1);
    EXPECT_EQ(val, &dummy);

    hoo_map_release(map);
}

TEST_F(HooMapTest, Clear) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64, HOO_MAP_VAL_INT64);
    int64_t k1 = 1, k2 = 2;
    int64_t v10 = 10, v20 = 20;
    hoo_map_set(map, &k1, &v10);
    hoo_map_set(map, &k2, &v20);
    EXPECT_EQ(hoo_map_count(map), 2);

    hoo_map_clear(map);
    EXPECT_EQ(hoo_map_count(map), 0);
    EXPECT_TRUE(hoo_map_is_empty(map));

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
    EXPECT_EQ(hoo_map_count(nullptr), 0);
    EXPECT_EQ(hoo_map_is_empty(nullptr), 1);
    EXPECT_EQ(hoo_map_key_type(nullptr), -1);
    EXPECT_EQ(hoo_map_value_type(nullptr), -1);
}
