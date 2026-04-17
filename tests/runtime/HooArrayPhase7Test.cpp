#include <gtest/gtest.h>
#include "../src/runtime/lib/hoo_generic_array.h"
#include <cstring>
#include <cmath>

using namespace hooc;

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
        EXPECT_GE(hoo_array_push_int64(arr, i), 0);
    }

    EXPECT_EQ(hoo_array_length(arr), 5);
    EXPECT_FALSE(hoo_array_empty(arr) != 0);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, ClearArray) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    for (int64_t i = 0; i < 10; i++) {
        hoo_array_push_int64(arr, i);
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
        int64_t result = hoo_array_push_int64(arr, values[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 5);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetInt64Values) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    int64_t values[] = {42, -17, 0, 1000000, -1000000};

    for (size_t i = 0; i < 5; i++) {
        hoo_array_push_int64(arr, values[i]);
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
        int64_t result = hoo_array_push_double(arr, values[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 5);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetDoubleValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    double values[] = {3.14, -2.71828, 0.0, 1.414, 1e-10};

    for (size_t i = 0; i < 5; i++) {
        hoo_array_push_double(arr, values[i]);
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
        int64_t result = hoo_array_push_float(arr, values[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 4);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushBoolValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    // Push bool values (1 for true, 0 for false)
    hoo_array_push_bool(arr, 1);
    hoo_array_push_bool(arr, 0);
    hoo_array_push_bool(arr, 1);
    hoo_array_push_bool(arr, 1);

    EXPECT_EQ(hoo_array_length(arr), 4);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, GetBoolValues) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    hoo_array_push_bool(arr, 1);
    hoo_array_push_bool(arr, 0);
    hoo_array_push_bool(arr, 1);

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
        int64_t result = hoo_array_push_char(arr, values[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
    }

    EXPECT_EQ(hoo_array_length(arr), 5);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, PushStringPointers) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    const char* strings[] = {"hello", "world", "test", "phase7"};

    for (size_t i = 0; i < 4; i++) {
        int64_t result = hoo_array_push_string(arr, strings[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
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
        int64_t result = hoo_array_push_object(arr, pointers[i]);
        EXPECT_EQ(result, static_cast<int64_t>(i + 1));
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
        hoo_array_push_object(arr, pointers[i]);
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
    hoo_array_push_object(objects1, &obj1);
    hoo_array_push_object(objects1, &obj2);

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
    hoo_array_push_int64(row1, 1);
    hoo_array_push_int64(row1, 2);

    // Create second row
    HooArray row2 = hoo_array_new();
    hoo_array_push_int64(row2, 3);
    hoo_array_push_int64(row2, 4);

    // Push rows into outer array
    hoo_array_push_array(outer, row1);
    hoo_array_push_array(outer, row2);

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
    hoo_array_push_array(outer, inner);

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
    hoo_array_push_int64(level1_1, 1);
    hoo_array_push_int64(level1_1, 2);
    hoo_array_push_array(level2_1, level1_1);

    // Create [[3, 4]]
    HooArray level2_2 = hoo_array_new();
    HooArray level1_2 = hoo_array_new();
    hoo_array_push_int64(level1_2, 3);
    hoo_array_push_int64(level1_2, 4);
    hoo_array_push_array(level2_2, level1_2);

    // Push both into level 3
    hoo_array_push_array(level3, level2_1);
    hoo_array_push_array(level3, level2_2);

    EXPECT_EQ(hoo_array_length(level3), 2);

    hoo_array_release(level3);
}

// ============================================================================
// Test 23-25: Type Information Functions
// ============================================================================

TEST_F(HooArrayPhase7Test, TypeInformationInt64) {
    HooArray arr = hoo_array_new();
    hoo_array_push_int64(arr, 42);

    const char* type_name = hoo_array_element_type(arr);
    EXPECT_NE(type_name, nullptr);

    // Type name should contain "long" or "int64"
    // (exact name depends on compiler, but should include type info)
    EXPECT_NE(type_name, nullptr);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, TypeInformationDouble) {
    HooArray arr = hoo_array_new();
    hoo_array_push_double(arr, 3.14);

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
    hoo_array_push_int64(arr, 1);
    hoo_array_push_int64(arr, 2);

    int64_t value = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, -1, &value), 0);  // Negative index
    EXPECT_EQ(hoo_array_get_int64(arr, 10, &value), 0);  // Beyond length

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, LargeArrayInt64) {
    HooArray arr = hoo_array_new();

    // Push 1000 elements
    for (int64_t i = 0; i < 1000; i++) {
        int64_t result = hoo_array_push_int64(arr, i);
        EXPECT_GE(result, 0);
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
        int64_t result = hoo_array_push_double(arr, val);
        EXPECT_GE(result, 0);
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
        hoo_array_push_int64(arr, values[i]);
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

TEST_F(HooArrayPhase7Test, ComplexMixedArray) {
    // Create an array with multiple different types mixed
    // This tests the flexibility of std::any
    HooArray arr = hoo_array_new();

    hoo_array_push_int64(arr, 42);      // int64
    hoo_array_push_double(arr, 3.14);   // double
    hoo_array_push_bool(arr, 1);        // bool
    hoo_array_push_float(arr, 2.71f);   // float
    hoo_array_push_char(arr, 'X');      // char
    hoo_array_push_string(arr, "test"); // string pointer
    static int obj = 100;
    hoo_array_push_object(arr, &obj);   // object pointer

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

// ============================================================================
// Test 31-35: Memory Management and Stress Tests
// ============================================================================

TEST_F(HooArrayPhase7Test, RepeatedCreateDestroy) {
    // Create and destroy arrays multiple times
    for (int i = 0; i < 100; i++) {
        HooArray arr = hoo_array_new();
        EXPECT_NE(arr, nullptr);

        for (int j = 0; j < 10; j++) {
            hoo_array_push_int64(arr, j);
        }

        EXPECT_EQ(hoo_array_length(arr), 10);
        hoo_array_release(arr);
    }
}

TEST_F(HooArrayPhase7Test, ClearAndReuse) {
    HooArray arr = hoo_array_new();

    // First use
    for (int64_t i = 0; i < 5; i++) {
        hoo_array_push_int64(arr, i);
    }
    EXPECT_EQ(hoo_array_length(arr), 5);

    // Clear and reuse
    hoo_array_clear(arr);
    EXPECT_EQ(hoo_array_length(arr), 0);

    for (int64_t i = 100; i < 105; i++) {
        hoo_array_push_int64(arr, i);
    }
    EXPECT_EQ(hoo_array_length(arr), 5);

    int64_t val = 0;
    hoo_array_get_int64(arr, 0, &val);
    EXPECT_EQ(val, 100);

    hoo_array_release(arr);
}

TEST_F(HooArrayPhase7Test, RetainReleaseMultiple) {
    HooArray arr = hoo_array_new();
    hoo_array_push_int64(arr, 42);

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

    hoo_array_push_int64(inner1, 1);
    hoo_array_push_int64(inner2, 2);

    hoo_array_push_array(outer, inner1);
    hoo_array_push_array(outer, inner2);

    EXPECT_EQ(hoo_array_length(outer), 2);

    // Clear outer without releasing arrays first
    hoo_array_clear(outer);

    EXPECT_EQ(hoo_array_length(outer), 0);

    hoo_array_release(outer);
    hoo_array_release(inner1);
    hoo_array_release(inner2);
}
