#include <gtest/gtest.h>

#include "runtime/lib/anyarray/hoo_anyarray.h"
#include "runtime/lib/hashmap/hoo_hashmap.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/string/hoo_string.h"

#include <cstring>

class HooAnyCollectionsTest : public ::testing::Test {};

TEST_F(HooAnyCollectionsTest, AnyArrayStoresMixedValues) {
    HooAnyArray arr = hoo_anyarray_new();
    ASSERT_NE(arr, nullptr);

    EXPECT_EQ(hoo_anyarray_push(arr, HOO_TYPE_INT64, 42), 1);

    double pi = 3.25;
    uint64_t piBits = 0;
    std::memcpy(&piBits, &pi, sizeof(double));
    EXPECT_EQ(hoo_anyarray_push(arr, HOO_TYPE_FLOAT64, piBits), 1);

    HooAnyValue out{0, 0};
    EXPECT_EQ(hoo_anyarray_get(arr, 0, &out), 1);
    EXPECT_EQ(out.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(out.data, 42);
    EXPECT_EQ(hoo_anyarray_get(arr, 1, &out), 1);
    EXPECT_EQ(out.type_id, HOO_TYPE_FLOAT64);
    EXPECT_EQ(out.data, piBits);
    EXPECT_EQ(hoo_anyarray_get(arr, 2, &out), 0);

    hoo_anyarray_release(arr);
}

TEST_F(HooAnyCollectionsTest, AnyArrayRetainsAndReleasesManagedValues) {
    HooAnyArray arr = hoo_anyarray_new();
    void* str = hoo_string_from_cstr("hello");
    ASSERT_NE(arr, nullptr);
    ASSERT_NE(str, nullptr);

    EXPECT_EQ(hoo_get_refcount(str), 1);
    EXPECT_EQ(hoo_anyarray_push(arr, HOO_TYPE_STRING, reinterpret_cast<uintptr_t>(str)), 1);
    EXPECT_EQ(hoo_get_refcount(str), 2);

    EXPECT_EQ(hoo_anyarray_set(arr, 0, HOO_TYPE_INT64, 7), 1);
    EXPECT_EQ(hoo_get_refcount(str), 1);

    hoo_release(str);
    hoo_anyarray_release(arr);
}

TEST_F(HooAnyCollectionsTest, HashMapFixedAndAnyValues) {
    HooHashMap fixed = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(fixed, nullptr);
    EXPECT_EQ(hoo_hashmap_set_fixed_i8(fixed, 10, 100), 1);
    uint64_t fixedOut = 0;
    EXPECT_EQ(hoo_hashmap_get_fixed_i8(fixed, 10, &fixedOut), 1);
    EXPECT_EQ(fixedOut, 100);
    EXPECT_EQ(hoo_hashmap_count(fixed), 1);
    hoo_hashmap_release(fixed);

    HooHashMap any = hoo_hashmap_new(HOO_TYPE_BYTE, HOO_TYPE_ANY);
    ASSERT_NE(any, nullptr);
    EXPECT_EQ(hoo_hashmap_set_any_i8(any, 1, HOO_TYPE_INT64, 200), 1);
    HooAnyValue out{0, 0};
    EXPECT_EQ(hoo_hashmap_get_any_i8(any, 1, &out), 1);
    EXPECT_EQ(out.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(out.data, 200);
    EXPECT_EQ(hoo_hashmap_remove_i8(any, 1), 1);
    EXPECT_EQ(hoo_hashmap_count(any), 0);
    hoo_hashmap_release(any);
}

TEST_F(HooAnyCollectionsTest, HashMapAnyRetainsManagedValuesOnOverwriteAndClear) {
    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    void* str = hoo_string_from_cstr("value");
    ASSERT_NE(map, nullptr);
    ASSERT_NE(str, nullptr);

    EXPECT_EQ(hoo_hashmap_set_any_i8(map, 5, HOO_TYPE_STRING, reinterpret_cast<uintptr_t>(str)), 1);
    EXPECT_EQ(hoo_get_refcount(str), 2);
    EXPECT_EQ(hoo_hashmap_set_any_i8(map, 5, HOO_TYPE_INT64, 99), 1);
    EXPECT_EQ(hoo_get_refcount(str), 1);

    EXPECT_EQ(hoo_hashmap_set_any_i8(map, 6, HOO_TYPE_STRING, reinterpret_cast<uintptr_t>(str)), 1);
    EXPECT_EQ(hoo_get_refcount(str), 2);
    hoo_hashmap_clear(map);
    EXPECT_EQ(hoo_get_refcount(str), 1);

    hoo_release(str);
    hoo_hashmap_release(map);
}

TEST_F(HooAnyCollectionsTest, BoundaryFailuresAreDeterministic) {
    HooAnyValue anyOut{123, 456};
    uint64_t fixedOut = 789;

    EXPECT_EQ(hoo_anyarray_length(nullptr), 0);
    EXPECT_EQ(hoo_anyarray_push(nullptr, HOO_TYPE_INT64, 1), 0);
    EXPECT_EQ(hoo_anyarray_set(nullptr, 0, HOO_TYPE_INT64, 1), 0);
    EXPECT_EQ(hoo_anyarray_get(nullptr, 0, &anyOut), 0);
    EXPECT_EQ(hoo_anyarray_pop(nullptr, &anyOut), 0);

    HooAnyArray arr = hoo_anyarray_new();
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_anyarray_get(arr, -1, &anyOut), 0);
    EXPECT_EQ(hoo_anyarray_get(arr, 0, &anyOut), 0);
    EXPECT_EQ(hoo_anyarray_set(arr, 0, HOO_TYPE_INT64, 1), 0);
    hoo_anyarray_release(arr);

    EXPECT_EQ(hoo_hashmap_count(nullptr), 0);
    EXPECT_EQ(hoo_hashmap_remove_i8(nullptr, 1), 0);
    EXPECT_EQ(hoo_hashmap_set_fixed_i8(nullptr, 1, 2), 0);
    EXPECT_EQ(hoo_hashmap_get_fixed_i8(nullptr, 1, &fixedOut), 0);
    EXPECT_EQ(hoo_hashmap_set_any_i8(nullptr, 1, HOO_TYPE_INT64, 2), 0);
    EXPECT_EQ(hoo_hashmap_get_any_i8(nullptr, 1, &anyOut), 0);

    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_hashmap_get_any_i8(map, 99, &anyOut), 0);
    EXPECT_EQ(hoo_hashmap_remove_i8(map, 99), 0);
    hoo_hashmap_release(map);
}
