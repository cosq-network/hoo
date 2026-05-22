#include <gtest/gtest.h>
#include "runtime/lib/hoo_map.h"

class HooMapTest : public ::testing::Test {
};

TEST_F(HooMapTest, Int64KeyMap) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT64);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_length(map), 0);
    EXPECT_TRUE(hoo_map_empty(map));
    
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

TEST_F(HooMapTest, StringKeyMap) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING);
    
    hoo_map_set_string_int64(map, "key1", 1000);
    hoo_map_set_string_string(map, "key2", "value2");
    
    int64_t val = 0;
    EXPECT_EQ(hoo_map_get_string_int64(map, "key1", &val), 1);
    EXPECT_EQ(val, 1000);
    
    const char* sval = nullptr;
    EXPECT_EQ(hoo_map_get_string_string(map, "key2", &sval), 1);
    EXPECT_STREQ(sval, "value2");
    
    hoo_map_release(map);
}

TEST_F(HooMapTest, ARC) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_INT8);
    EXPECT_EQ(hoo_map_refcount(map), 1);
    
    hoo_map_retain(map);
    EXPECT_EQ(hoo_map_refcount(map), 2);
    
    hoo_map_release(map);
    EXPECT_EQ(hoo_map_refcount(map), 1);
    
    hoo_map_release(map);
}
