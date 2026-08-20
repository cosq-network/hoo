#include <gtest/gtest.h>
#include "runtime/lib/generic_array/hoo_generic_array.h"
#include <cstring>
#include <cmath>

class HooArrayPhase7Test : public ::testing::Test {
protected:
    void TearDown() override {
        // Clean up any arrays created during tests
    }
};

// ============================================================================
// Test 1-5: Basic Array Operations
// ============================================================================

TEST_F(HooArrayPhase7Test, CreateEmptyArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 0);
    EXPECT_TRUE(hoo_array_empty(arr) != 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ArrayLength) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    // Push 5 elements
    for (int64_t i = 0; i < 5; i++) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
    }

    EXPECT_EQ(hoo_array_length(arr), 5);
    EXPECT_FALSE(hoo_array_empty(arr) != 0);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ClearArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    for (int64_t i = 0; i < 10; i++) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
    }

    EXPECT_EQ(hoo_array_length(arr), 10);

    hoo_array_clear(arr);

    EXPECT_EQ(hoo_array_length(arr), 0);
    EXPECT_TRUE(hoo_array_empty(arr) != 0);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReferenceCountingBasic) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    EXPECT_EQ(hoo_array_refcount(arr), 1);

    HooArray arr2 = hoo_array_retain(arr);
    EXPECT_EQ(arr, arr2);
    EXPECT_EQ(hoo_array_refcount(arr), 2);

    hoo_array_release(arr);
    EXPECT_EQ(hoo_array_refcount(arr2), 1);

    hoo_array_release(arr2);
}

TEST_F(HooArrayPhase7Test, NullHandling) {
    // All functions should handle NULL gracefully
    EXPECT_EQ(hoo_array_length(nullptr), 0);
    EXPECT_TRUE(hoo_array_empty(nullptr) != 0);
    EXPECT_EQ(hoo_array_refcount(nullptr), 0);

    // Release on NULL should be no-op
    hoo_array_release(nullptr);

    // Retain on NULL should return NULL
    HooArray result = hoo_array_retain(nullptr);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// Test 6-13: Type-Specific Push Operations (int64, double, float, bool)
// ============================================================================

TEST_F(HooArrayPhase7Test, PushInt64Values) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    int64_t values[] = {1, 2, 3, -100, 9223372036854775807LL};  // Include max int64

    for (size_t i = 0; i < 5; i++) {
        arr = hoo_array_push_int64(arr, values[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetInt64Values) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    int64_t values[] = {42, -17, 0, 1000000, -1000000};

    for (size_t i = 0; i < 5; i++) {
        arr = hoo_array_push_int64(arr, values[i]);
        ASSERT_NE(arr, nullptr);
    }

    // Verify we can get them back
    for (size_t i = 0; i < 5; i++) {
        int64_t retrieved = 0;
        int result = hoo_array_get_int64(arr, i, &retrieved);
        EXPECT_EQ(result, 1);
        EXPECT_EQ(retrieved, values[i]);
    }

    // Out of bounds should fail
    int64_t dummy = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 10, &dummy), 0);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushDoubleValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    double values[] = {1.5, -3.14, 0.0, 2.71828, 1e100};

    for (size_t i = 0; i < 5; i++) {
        arr = hoo_array_push_double(arr, values[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetDoubleValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    double values[] = {3.14, -2.71828, 0.0, 1.414, 1e-10};

    for (size_t i = 0; i < 5; i++) {
        arr = hoo_array_push_double(arr, values[i]);
        ASSERT_NE(arr, nullptr);
    }

    // Verify with tolerance
    for (size_t i = 0; i < 5; i++) {
        double retrieved = 0.0;
        int result = hoo_array_get_double(arr, i, &retrieved);
        EXPECT_EQ(result, 1);
        EXPECT_DOUBLE_EQ(retrieved, values[i]);
    }

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushFloatValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    float values[] = {1.5f, -3.14f, 0.0f, 2.71f};

    for (size_t i = 0; i < 4; i++) {
        arr = hoo_array_push_float(arr, values[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 4);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushBoolValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    // Push bool values (1 for true, 0 for false)
    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 0); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);

    EXPECT_EQ(hoo_array_length(arr), 4);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetBoolValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 0); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);

    int64_t value1 = 0, value2 = 0, value3 = 0;
    EXPECT_EQ(hoo_array_get_bool(arr, 0, &value1), 1);
    EXPECT_EQ(hoo_array_get_bool(arr, 1, &value2), 1);
    EXPECT_EQ(hoo_array_get_bool(arr, 2, &value3), 1);

    EXPECT_EQ(value1, 1);
    EXPECT_EQ(value2, 0);
    EXPECT_EQ(value3, 1);

    hoo_array_release(arr);
}

// ============================================================================
// Test 14-15: Character and String Arrays
// ============================================================================

TEST_F(HooArrayPhase7Test, PushCharValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    char values[] = {'a', 'b', 'c', 'x', 'z'};

    for (size_t i = 0; i < 5; i++) {
        arr = hoo_array_push_char(arr, values[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 5);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushStringPointers) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    const char* strings[] = {"hello", "world", "test", "phase7"};

    for (size_t i = 0; i < 4; i++) {
        arr = hoo_array_push_string(arr, strings[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 4);

    // Verify we can retrieve string pointers
    for (size_t i = 0; i < 4; i++) {
        const char* retrieved = nullptr;
        int result = hoo_array_get_string(arr, i, &retrieved);
        EXPECT_EQ(result, 1);
        EXPECT_STREQ(retrieved, strings[i]);
    }

    hoo_array_release(arr);
}

// ============================================================================
// Test 16-18: Object Pointer Arrays (Simulating Class Instances)
// ============================================================================

TEST_F(HooArrayPhase7Test, PushObjectPointers) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    // Use addresses of static variables as mock object pointers
    static int obj1 = 1, obj2 = 2, obj3 = 3;
    void* pointers[] = {&obj1, &obj2, &obj3};

    for (size_t i = 0; i < 3; i++) {
        arr = hoo_array_push_object(arr, pointers[i]);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 3);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetObjectPointers) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    static int obj1 = 100, obj2 = 200, obj3 = 300;
    void* pointers[] = {&obj1, &obj2, &obj3};

    for (size_t i = 0; i < 3; i++) {
        arr = hoo_array_push_object(arr, pointers[i]); ASSERT_NE(arr, nullptr);
    }

    // Retrieve and verify pointers
    for (size_t i = 0; i < 3; i++) {
        void* retrieved = nullptr;
        int result = hoo_array_get_object(arr, i, &retrieved);
        EXPECT_EQ(result, 1);
        EXPECT_EQ(retrieved, pointers[i]);
    }

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ObjectPointerArrays) {
    HooArray objects1 = hoo_array_new();
    HooArray objects2 = hoo_array_new();

    static int obj1 = 1, obj2 = 2;
    objects1 = hoo_array_push_object(objects1, &obj1); ASSERT_NE(objects1, nullptr);
    objects1 = hoo_array_push_object(objects1, &obj2); ASSERT_NE(objects1, nullptr);

    EXPECT_EQ(hoo_array_length(objects1), 2);
    EXPECT_EQ(hoo_array_length(objects2), 0);

    hoo_array_release(objects1);
    hoo_array_release(objects2);
}

// ============================================================================
// Test 19-22: Multi-Dimensional Arrays
// ============================================================================

TEST_F(HooArrayPhase7Test, MultiDimensionalArrayBasic) {
    // Create a 2D array: [[1, 2], [3, 4]]
    HooArray outer = hoo_array_new();
    ASSERT_NE(outer, nullptr);

    // Create first row
    HooArray row1 = hoo_array_new();
    row1 = hoo_array_push_int64(row1, 1); ASSERT_NE(row1, nullptr);
    row1 = hoo_array_push_int64(row1, 2); ASSERT_NE(row1, nullptr);

    // Create second row
    HooArray row2 = hoo_array_new();
    row2 = hoo_array_push_int64(row2, 3); ASSERT_NE(row2, nullptr);
    row2 = hoo_array_push_int64(row2, 4); ASSERT_NE(row2, nullptr);

    // Push rows into outer array
    outer = hoo_array_push_array(outer, row1); ASSERT_NE(outer, nullptr);
    outer = hoo_array_push_array(outer, row2); ASSERT_NE(outer, nullptr);

    EXPECT_EQ(hoo_array_length(outer), 2);

    // Retrieve and verify
    HooArray retrieved1 = nullptr;
    HooArray retrieved2 = nullptr;

    int res1 = hoo_array_get_array(outer, 0, &retrieved1);
    int res2 = hoo_array_get_array(outer, 1, &retrieved2);

    EXPECT_EQ(res1, 1);
    EXPECT_EQ(res2, 1);

    // Verify row contents
    if (retrieved1) {
        int64_t val = 0;
        hoo_array_get_int64(retrieved1, 0, &val);
        EXPECT_EQ(val, 1);
        hoo_array_get_int64(retrieved1, 1, &val);
        EXPECT_EQ(val, 2);
    }

    if (retrieved2) {
        int64_t val = 0;
        hoo_array_get_int64(retrieved2, 0, &val);
        EXPECT_EQ(val, 3);
        hoo_array_get_int64(retrieved2, 1, &val);
        EXPECT_EQ(val, 4);
    }

    hoo_array_release(outer);
}

TEST_F(HooArrayPhase7Test, NestedArrayRefCounting) {
    HooArray outer = hoo_array_new();
    HooArray inner = hoo_array_new();

    // Initially inner has refcount 1
    EXPECT_EQ(hoo_array_refcount(inner), 1);

    // Push inner into outer (should retain)
    outer = hoo_array_push_array(outer, inner); ASSERT_NE(outer, nullptr);

    // Now inner should have refcount 2 (one from creation, one from push/retain)
    EXPECT_EQ(hoo_array_refcount(inner), 2);

    // Release our original reference
    hoo_array_release(inner);

    // Inner should still be alive with refcount 1 (held by outer)
    EXPECT_EQ(hoo_array_refcount(inner), 1);

    // Releasing outer should also release inner
    hoo_array_release(outer);
}

TEST_F(HooArrayPhase7Test, TripleDimensionalArray) {
    // Create [[[1, 2]], [[3, 4]]]
    HooArray level3 = hoo_array_new();

    // Create [[1, 2]]
    HooArray level2_1 = hoo_array_new();
    HooArray level1_1 = hoo_array_new();
    level1_1 = hoo_array_push_int64(level1_1, 1); ASSERT_NE(level1_1, nullptr);
    level1_1 = hoo_array_push_int64(level1_1, 2); ASSERT_NE(level1_1, nullptr);
    level2_1 = hoo_array_push_array(level2_1, level1_1); ASSERT_NE(level2_1, nullptr);

    // Create [[3, 4]]
    HooArray level2_2 = hoo_array_new();
    HooArray level1_2 = hoo_array_new();
    level1_2 = hoo_array_push_int64(level1_2, 3); ASSERT_NE(level1_2, nullptr);
    level1_2 = hoo_array_push_int64(level1_2, 4); ASSERT_NE(level1_2, nullptr);
    level2_2 = hoo_array_push_array(level2_2, level1_2); ASSERT_NE(level2_2, nullptr);

    // Push both into level 3
    level3 = hoo_array_push_array(level3, level2_1); ASSERT_NE(level3, nullptr);
    level3 = hoo_array_push_array(level3, level2_2); ASSERT_NE(level3, nullptr);

    EXPECT_EQ(hoo_array_length(level3), 2);

    hoo_array_release(level3);
}

// ============================================================================
// Test 23-25: Type Information Functions
// ============================================================================

TEST_F(HooArrayPhase7Test, TypeInformationInt64) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 42); ASSERT_NE(arr, nullptr);

    const char* type_name = hoo_array_element_type(arr);
    EXPECT_NE(type_name, nullptr);

    // Type name should contain "long" or "int64"
    // (exact name depends on compiler, but should include type info)
    EXPECT_NE(type_name, nullptr);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, TypeInformationDouble) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_double(arr, 3.14); ASSERT_NE(arr, nullptr);

    const char* type_name = hoo_array_element_type(arr);
    EXPECT_NE(type_name, nullptr);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, EmptyArrayTypeInfo) {
    HooArray arr = hoo_array_new();

    // Empty array has no type info
    const char* type_name = hoo_array_element_type(arr);
    EXPECT_EQ(type_name, nullptr);

    hoo_array_release(arr);
}

// ============================================================================
// Test 26-30: Mixed Type Error Handling
// ============================================================================

TEST_F(HooArrayPhase7Test, OutOfBoundsGet) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 1); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 2); ASSERT_NE(arr, nullptr);

    int64_t value = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, -1, &value), 0);  // Negative index
    EXPECT_EQ(hoo_array_get_int64(arr, 10, &value), 0);  // Beyond length

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, LargeArrayInt64) {
    HooArray arr = hoo_array_new();

    // Push 1000 elements
    for (int64_t i = 0; i < 1000; i++) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), i + 1);
    }

    EXPECT_EQ(hoo_array_length(arr), 1000);

    // Verify some random accesses
    int64_t val = 0;
    hoo_array_get_int64(arr, 0, &val);
    EXPECT_EQ(val, 0);

    hoo_array_get_int64(arr, 500, &val);
    EXPECT_EQ(val, 500);

    hoo_array_get_int64(arr, 999, &val);
    EXPECT_EQ(val, 999);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, LargeArrayDouble) {
    HooArray arr = hoo_array_new();

    // Push 500 double values
    for (int64_t i = 0; i < 500; i++) {
        double val = static_cast<double>(i) * 1.5;
        arr = hoo_array_push_double(arr, val);
        ASSERT_NE(arr, nullptr);
        EXPECT_EQ(hoo_array_length(arr), i + 1);
    }

    EXPECT_EQ(hoo_array_length(arr), 500);

    // Verify some accesses
    double retrieved = 0.0;
    hoo_array_get_double(arr, 100, &retrieved);
    EXPECT_DOUBLE_EQ(retrieved, 100.0 * 1.5);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, MixedInt64Array) {
    HooArray arr = hoo_array_new();

    // Push both positive and negative values
    int64_t values[] = {-1000, -1, 0, 1, 1000, -2147483648LL, 2147483647LL};

    for (size_t i = 0; i < 7; i++) {
        arr = hoo_array_push_int64(arr, values[i]); ASSERT_NE(arr, nullptr);
    }

    EXPECT_EQ(hoo_array_length(arr), 7);

    // Verify each value
    for (size_t i = 0; i < 7; i++) {
        int64_t retrieved = 0;
        hoo_array_get_int64(arr, i, &retrieved);
        EXPECT_EQ(retrieved, values[i]);
    }

    hoo_array_release(arr);
}


#if 0 // ComplexMixedArray test disabled
    // This tests the flexibility of std::any
    HooArray arr = hoo_array_new();

    arr = hoo_array_push_int64(arr, 42); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_double(arr, 3.14); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_bool(arr, 1); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_float(arr, 2.71f); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_char(arr, 'X'); ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_string(arr, "test"); ASSERT_NE(arr, nullptr);
    static int obj = 100;
    arr = hoo_array_push_object(arr, &obj); ASSERT_NE(arr, nullptr);

    EXPECT_EQ(hoo_array_length(arr), 7);

    // Retrieve each value with correct type
    int64_t i64 = 0;
    double d = 0.0;
    int64_t b = 0;
    float f = 0.0f;
    char c = 0;
    const char* s = nullptr;
    void* o = nullptr;

    hoo_array_get_int64(arr, 0, &i64);
    EXPECT_EQ(i64, 42);

    hoo_array_get_double(arr, 1, &d);
    EXPECT_DOUBLE_EQ(d, 3.14);

    hoo_array_get_bool(arr, 2, &b);
    EXPECT_EQ(b, 1);

    hoo_array_get_float(arr, 3, &f);
    EXPECT_FLOAT_EQ(f, 2.71f);

    hoo_array_get_char(arr, 4, &c);
    EXPECT_EQ(c, 'X');

    hoo_array_get_string(arr, 5, &s);
    EXPECT_STREQ(s, "test");

    hoo_array_get_object(arr, 6, &o);
    EXPECT_EQ(o, &obj);

    hoo_array_release(arr);
}
#endif // ComplexMixedArray test disabled

// ============================================================================
// Test 31-35: Memory Management and Stress Tests
// ============================================================================

TEST_F(HooArrayPhase7Test, RepeatedCreateDestroy) {
    // Create and destroy arrays multiple times
    for (int i = 0; i < 100; i++) {
        HooArray arr = hoo_array_new();
        EXPECT_NE(arr, nullptr);

        for (int j = 0; j < 10; j++) {
            arr = hoo_array_push_int64(arr, j); ASSERT_NE(arr, nullptr);
        }

        EXPECT_EQ(hoo_array_length(arr), 10);
        hoo_array_release(arr);
    }
}

TEST_F(HooArrayPhase7Test, ClearAndReuse) {
    HooArray arr = hoo_array_new();

    // First use
    for (int64_t i = 0; i < 5; i++) {
        arr = hoo_array_push_int64(arr, i); ASSERT_NE(arr, nullptr);
    }
    EXPECT_EQ(hoo_array_length(arr), 5);

    // Clear and reuse
    hoo_array_clear(arr);
    EXPECT_EQ(hoo_array_length(arr), 0);

    for (int64_t i = 100; i < 105; i++) {
        arr = hoo_array_push_int64(arr, i); ASSERT_NE(arr, nullptr);
    }
    EXPECT_EQ(hoo_array_length(arr), 5);

    int64_t val = 0;
    hoo_array_get_int64(arr, 0, &val);
    EXPECT_EQ(val, 100);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, RetainReleaseMultiple) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 42); ASSERT_NE(arr, nullptr);

    EXPECT_EQ(hoo_array_refcount(arr), 1);

    HooArray arr2 = hoo_array_retain(arr);
    HooArray arr3 = hoo_array_retain(arr);
    HooArray arr4 = hoo_array_retain(arr);

    EXPECT_EQ(hoo_array_refcount(arr), 4);
    EXPECT_EQ(hoo_array_refcount(arr2), 4);
    EXPECT_EQ(hoo_array_refcount(arr3), 4);
    EXPECT_EQ(hoo_array_refcount(arr4), 4);

    hoo_array_release(arr);
    EXPECT_EQ(hoo_array_refcount(arr2), 3);

    hoo_array_release(arr2);
    EXPECT_EQ(hoo_array_refcount(arr3), 2);

    hoo_array_release(arr3);
    EXPECT_EQ(hoo_array_refcount(arr4), 1);

    hoo_array_release(arr4);
}

TEST_F(HooArrayPhase7Test, NestedArrayLifecycle) {
    HooArray outer = hoo_array_new();
    HooArray inner1 = hoo_array_new();
    HooArray inner2 = hoo_array_new();

    inner1 = hoo_array_push_int64(inner1, 1); ASSERT_NE(inner1, nullptr);
    inner2 = hoo_array_push_int64(inner2, 2); ASSERT_NE(inner2, nullptr);

    outer = hoo_array_push_array(outer, inner1); ASSERT_NE(outer, nullptr);
    outer = hoo_array_push_array(outer, inner2); ASSERT_NE(outer, nullptr);

    EXPECT_EQ(hoo_array_length(outer), 2);

    // Clear outer without releasing arrays first
    hoo_array_clear(outer);

    EXPECT_EQ(hoo_array_length(outer), 0);

    hoo_array_release(outer);
    hoo_array_release(inner1);
    hoo_array_release(inner2);
}

// ============================================================================
// Sort & Reverse Tests
// ============================================================================

TEST_F(HooArrayPhase7Test, SortEmptyArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_sort(arr);
    EXPECT_EQ(hoo_array_length(arr), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortSingleElement) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 42);
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_sort(arr);
    EXPECT_EQ(hoo_array_length(arr), 1);
    int64_t val = 0;
    EXPECT_TRUE(hoo_array_get_int64(arr, 0, &val));
    EXPECT_EQ(val, 42);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortInt64Ascending) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    int64_t input[] = {5, 3, 9, 1, 7, -2, 0, 42, -10};
    for (int64_t v : input) {
        arr = hoo_array_push_int64(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    EXPECT_EQ(hoo_array_length(arr), 9);
    int64_t prev = INT64_MIN;
    for (int64_t i = 0; i < 9; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_GE(val, prev);
        prev = val;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortInt64AlreadySorted) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    for (int64_t i = 0; i < 10; i++) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    EXPECT_EQ(hoo_array_length(arr), 10);
    for (int64_t i = 0; i < 10; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_EQ(val, i);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortInt64Descending) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    for (int64_t i = 9; i >= 0; i--) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    for (int64_t i = 0; i < 10; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_EQ(val, i);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortInt64Duplicates) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    int64_t input[] = {7, 3, 7, 1, 3, 7, 1, 1, 3};
    for (int64_t v : input) {
        arr = hoo_array_push_int64(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    int64_t prev = INT64_MIN;
    for (int64_t i = 0; i < 9; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_GE(val, prev);
        prev = val;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortDoubleAscending) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    double input[] = {3.14, -1.5, 2.71, 0.0, -0.5, 100.0, -100.0, 0.001};
    for (double v : input) {
        arr = hoo_array_push_double(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    double prev = -INFINITY;
    for (int64_t i = 0; i < 8; i++) {
        double val = 0;
        EXPECT_TRUE(hoo_array_get_double(arr, i, &val));
        EXPECT_GE(val, prev);
        prev = val;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortDoubleNegativeValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    double input[] = {-5.0, -1.0, -10.0, -0.5, -100.0, -0.001};
    for (double v : input) {
        arr = hoo_array_push_double(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    double prev = -INFINITY;
    for (int64_t i = 0; i < 6; i++) {
        double val = 0;
        EXPECT_TRUE(hoo_array_get_double(arr, i, &val));
        EXPECT_GE(val, prev) << "at index " << i;
        prev = val;
    }
    // Verify ordering: -100, -10, -5, -1, -0.5, -0.001
    double expected[] = {-100.0, -10.0, -5.0, -1.0, -0.5, -0.001};
    for (int64_t i = 0; i < 6; i++) {
        double val = 0;
        EXPECT_TRUE(hoo_array_get_double(arr, i, &val));
        EXPECT_DOUBLE_EQ(val, expected[i]);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortDoubleMixedSign) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    double input[] = {0.0, -1.0, 1.0, -0.0, 3.14, -3.14, 2.71, -2.71};
    for (double v : input) {
        arr = hoo_array_push_double(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    double prev = -INFINITY;
    for (int64_t i = 0; i < 8; i++) {
        double val = 0;
        EXPECT_TRUE(hoo_array_get_double(arr, i, &val));
        EXPECT_GE(val, prev) << "at index " << i;
        prev = val;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortNullReturnsNull) {
    EXPECT_EQ(hoo_array_sort(nullptr), nullptr);
}

TEST_F(HooArrayPhase7Test, ReverseEmptyArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_reverse(arr);
    EXPECT_EQ(hoo_array_length(arr), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseSingleElement) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 99);
    arr = hoo_array_reverse(arr);
    EXPECT_EQ(hoo_array_length(arr), 1);
    int64_t val = 0;
    EXPECT_TRUE(hoo_array_get_int64(arr, 0, &val));
    EXPECT_EQ(val, 99);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseTwoElements) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 1);
    arr = hoo_array_push_int64(arr, 2);
    arr = hoo_array_reverse(arr);
    EXPECT_EQ(hoo_array_length(arr), 2);
    int64_t v0 = 0, v1 = 0;
    EXPECT_TRUE(hoo_array_get_int64(arr, 0, &v0));
    EXPECT_TRUE(hoo_array_get_int64(arr, 1, &v1));
    EXPECT_EQ(v0, 2);
    EXPECT_EQ(v1, 1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseManyElements) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    const int64_t N = 100;
    for (int64_t i = 1; i <= N; i++) {
        arr = hoo_array_push_int64(arr, i);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_reverse(arr);
    for (int64_t i = 0; i < N; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_EQ(val, N - i);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseDoubleArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_double(arr, 1.5);
    arr = hoo_array_push_double(arr, 2.5);
    arr = hoo_array_push_double(arr, 3.5);
    arr = hoo_array_reverse(arr);
    double v0 = 0, v2 = 0;
    EXPECT_TRUE(hoo_array_get_double(arr, 0, &v0));
    EXPECT_TRUE(hoo_array_get_double(arr, 2, &v2));
    EXPECT_DOUBLE_EQ(v0, 3.5);
    EXPECT_DOUBLE_EQ(v2, 1.5);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReversePreservesElementType) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 10);
    arr = hoo_array_push_int64(arr, 20);
    arr = hoo_array_reverse(arr);
    int64_t val = 0;
    EXPECT_TRUE(hoo_array_get_int64(arr, 0, &val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(hoo_array_get_int64(arr, 1, &val));
    EXPECT_EQ(val, 10);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseNullReturnsNull) {
    EXPECT_EQ(hoo_array_reverse(nullptr), nullptr);
}

TEST_F(HooArrayPhase7Test, SortReverseChained) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    int64_t input[] = {3, 1, 4, 1, 5, 9, 2, 6};
    for (int64_t v : input) {
        arr = hoo_array_push_int64(arr, v);
        ASSERT_NE(arr, nullptr);
    }
    arr = hoo_array_sort(arr);
    arr = hoo_array_reverse(arr);
    // Should be descending
    int64_t prev = INT64_MAX;
    for (int64_t i = 0; i < 8; i++) {
        int64_t val = 0;
        EXPECT_TRUE(hoo_array_get_int64(arr, i, &val));
        EXPECT_LE(val, prev);
        prev = val;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortDoesNotReleaseArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 2);
    arr = hoo_array_push_int64(arr, 1);
    int64_t rc_before = hoo_array_refcount(arr);
    arr = hoo_array_sort(arr);
    EXPECT_EQ(hoo_array_refcount(arr), rc_before);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ReverseDoesNotReleaseArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    arr = hoo_array_push_int64(arr, 1);
    arr = hoo_array_push_int64(arr, 2);
    int64_t rc_before = hoo_array_refcount(arr);
    arr = hoo_array_reverse(arr);
    EXPECT_EQ(hoo_array_refcount(arr), rc_before);
    hoo_array_release(arr);
}

// ============================================================================
// Additional C API Tests: pop, set, from_buffer, repeat, push_h,
// binary search, shuffle, sort_range, element_type, type mismatch
// ============================================================================

TEST_F(HooArrayPhase7Test, PopInt64) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 10);
    arr = hoo_array_push_int64(arr, 20);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_EQ(dest, 20);
    EXPECT_EQ(hoo_array_length(arr), 1);
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_EQ(dest, 10);
    EXPECT_EQ(hoo_array_length(arr), 0);
    EXPECT_EQ(hoo_array_pop(arr, &dest), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PopEmptyReturnsZero) {
    HooArray arr = hoo_array_new();
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PopNullReturnsZero) {
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_pop(nullptr, &dest), 0);
}

TEST_F(HooArrayPhase7Test, SetInt64) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 100);
    arr = hoo_array_push_int64(arr, 200);
    int64_t new_val = 999;
    EXPECT_EQ(hoo_array_set(arr, 0, &new_val), 1);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 0, &dest), 1);
    EXPECT_EQ(dest, 999);
    EXPECT_EQ(hoo_array_get_int64(arr, 1, &dest), 1);
    EXPECT_EQ(dest, 200);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SetOutOfBounds) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 10);
    int64_t new_val = 99;
    EXPECT_EQ(hoo_array_set(arr, 5, &new_val), 0);
    EXPECT_EQ(hoo_array_set(arr, -1, &new_val), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, FromBuffer) {
    int64_t data[] = {10, 20, 30, 40, 50};
    HooArray arr = hoo_array_from_buffer(data, 5);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 5);
    int64_t dest = 0;
    for (int64_t i = 0; i < 5; i++) {
        EXPECT_EQ(hoo_array_get_int64(arr, i, &dest), 1);
        EXPECT_EQ(dest, data[i]);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, FromBufferEmpty) {
    HooArray arr = hoo_array_from_buffer(nullptr, 0);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 0);
    EXPECT_EQ(hoo_array_empty(arr), 1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, RepeatInt64) {
    int64_t val = 42;
    HooArray arr = hoo_array_repeat(&val, 5);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 5);
    int64_t dest = 0;
    for (int64_t i = 0; i < 5; i++) {
        EXPECT_EQ(hoo_array_get_int64(arr, i, &dest), 1);
        EXPECT_EQ(dest, 42);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, RepeatZero) {
    int64_t val = 1;
    HooArray arr = hoo_array_repeat(&val, 0);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushH) {
    HooArray arr = hoo_array_new();
    int64_t val1 = 10, val2 = 20;
    EXPECT_EQ(hoo_array_push_h(&arr, &val1), 1);
    EXPECT_EQ(hoo_array_length(arr), 1);
    EXPECT_EQ(hoo_array_push_h(&arr, &val2), 1);
    EXPECT_EQ(hoo_array_length(arr), 2);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 0, &dest), 1);
    EXPECT_EQ(dest, 10);
    EXPECT_EQ(hoo_array_get_int64(arr, 1, &dest), 1);
    EXPECT_EQ(dest, 20);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushHNull) {
    HooArray arr = hoo_array_new();
    int64_t val = 1;
    HooArray null_arr = nullptr;
    EXPECT_EQ(hoo_array_push_h(&null_arr, &val), 0);
    EXPECT_EQ(hoo_array_push_h(&arr, nullptr), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, BinarySearchInt64Found) {
    HooArray arr = hoo_array_new();
    for (int64_t i = 0; i < 10; i++) arr = hoo_array_push_int64(arr, i * 2);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 0), 0);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 4), 2);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 18), 9);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, BinarySearchInt64NotFound) {
    HooArray arr = hoo_array_new();
    for (int64_t i = 0; i < 5; i++) arr = hoo_array_push_int64(arr, i * 10);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 3), -1);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, -1), -1);
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 100), -1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, BinarySearchInt64Empty) {
    HooArray arr = hoo_array_new();
    EXPECT_EQ(hoo_array_binary_search_int64(arr, 1), -1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, BinarySearchInt64Null) {
    EXPECT_EQ(hoo_array_binary_search_int64(nullptr, 1), -1);
}

TEST_F(HooArrayPhase7Test, BinarySearchDoubleFound) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_double(arr, 1.1);
    arr = hoo_array_push_double(arr, 2.2);
    arr = hoo_array_push_double(arr, 3.3);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 1.1), 0);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 2.2), 1);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 3.3), 2);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, BinarySearchDoubleNotFound) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_double(arr, 1.0);
    arr = hoo_array_push_double(arr, 2.0);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 1.5), -1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortRangeInt64) {
    HooArray arr = hoo_array_new();
    for (int64_t i = 0; i < 10; i++) arr = hoo_array_push_int64(arr, 9 - i);
    arr = hoo_array_sort_range(arr, 2, 7);
    int64_t dest = 0;
    int64_t expected[] = {9, 8, 3, 4, 5, 6, 7, 2, 1, 0};
    for (int64_t i = 0; i < 10; i++) {
        EXPECT_EQ(hoo_array_get_int64(arr, i, &dest), 1);
        EXPECT_EQ(dest, expected[i]);
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, SortRangeClampsToLength) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 3);
    arr = hoo_array_push_int64(arr, 1);
    arr = hoo_array_sort_range(arr, 0, 100);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 0, &dest), 1);
    EXPECT_EQ(dest, 1);
    EXPECT_EQ(hoo_array_get_int64(arr, 1, &dest), 1);
    EXPECT_EQ(dest, 3);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ShufflePreservesLength) {
    HooArray arr = hoo_array_new();
    for (int64_t i = 0; i < 100; i++) arr = hoo_array_push_int64(arr, i);
    arr = hoo_array_shuffle(arr);
    EXPECT_EQ(hoo_array_length(arr), 100);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ShuffleNullReturnsNull) {
    EXPECT_EQ(hoo_array_shuffle(nullptr), nullptr);
}

TEST_F(HooArrayPhase7Test, ShuffleSingleElement) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 42);
    arr = hoo_array_shuffle(arr);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 0, &dest), 1);
    EXPECT_EQ(dest, 42);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ElementTypeInt64) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 1);
    EXPECT_STREQ(hoo_array_element_type(arr), "int64");
    EXPECT_EQ(hoo_array_is_type(arr, "int64"), 1);
    EXPECT_EQ(hoo_array_is_type(arr, "double"), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ElementTypeDouble) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_double(arr, 1.0);
    EXPECT_STREQ(hoo_array_element_type(arr), "double");
    EXPECT_EQ(hoo_array_is_type(arr, "double"), 1);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ElementTypeNull) {
    HooArray arr = hoo_array_new();
    EXPECT_EQ(hoo_array_element_type(arr), nullptr);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ElementTypeNullArray) {
    EXPECT_EQ(hoo_array_element_type(nullptr), nullptr);
}

TEST_F(HooArrayPhase7Test, PushInt64TypeMismatchReturnsNull) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_double(arr, 1.0);
    ASSERT_NE(arr, nullptr);
    HooArray result = hoo_array_push_int64(arr, 42);
    EXPECT_EQ(result, nullptr);
    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushDoubleTypeMismatchReturnsNull) {
    HooArray arr = hoo_array_new();
    arr = hoo_array_push_int64(arr, 1);
    ASSERT_NE(arr, nullptr);
    HooArray result = hoo_array_push_double(arr, 1.0);
    EXPECT_EQ(result, nullptr);
    hoo_array_release(arr);
}
