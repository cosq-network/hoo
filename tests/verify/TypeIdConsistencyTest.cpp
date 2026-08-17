#include <gtest/gtest.h>
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/overload/hoo_overload.h"
#include <set>

/**
 * Tests that verify the type ID constants used by the runtime, overload
 * system, and codegen are consistent and collision-free.
 *
 * These tests guard against regressions where adding a new type or
 * reassigning an existing one silently creates ID overlaps.
 */

class TypeIdConsistencyTest : public ::testing::Test {};

// ── Critical #1: No runtime type ID collisions ───────────────────────────

TEST_F(TypeIdConsistencyTest, AllRuntimeTypeIdsAreUnique) {
    std::set<uint32_t> ids;

    auto insert_unique = [&](uint32_t id, const char* name) {
        EXPECT_TRUE(ids.insert(id).second)
            << "Duplicate type ID " << id << " for " << name;
    };

    insert_unique(HOO_TYPE_OBJECT,       "OBJECT");
    insert_unique(HOO_TYPE_STRING,       "STRING");
    insert_unique(HOO_TYPE_ARRAY,        "ARRAY");
    insert_unique(HOO_TYPE_MAP,          "MAP");
    insert_unique(HOO_TYPE_EXCEPTION,    "EXCEPTION");
    insert_unique(HOO_TYPE_RANDOM,       "RANDOM");
    insert_unique(HOO_TYPE_NET_URL,      "NET_URL");
    insert_unique(HOO_TYPE_NET_HTTP_RES, "NET_HTTP_RES");
    insert_unique(HOO_TYPE_NET_HTTP_CLI, "NET_HTTP_CLI");
    insert_unique(HOO_TYPE_CHARACTER,    "CHARACTER");
    insert_unique(HOO_TYPE_UUID,         "UUID");
    insert_unique(HOO_TYPE_REGEX,        "REGEX");
    insert_unique(HOO_TYPE_JSON,         "JSON");
    insert_unique(HOO_TYPE_BUFFER,       "BUFFER");
    insert_unique(HOO_TYPE_CSV,          "CSV");
    insert_unique(HOO_TYPE_ARGS,         "ARGS");
    insert_unique(HOO_TYPE_COMPRESSION,  "COMPRESSION");
    insert_unique(HOO_TYPE_MUTEX,        "MUTEX");
    insert_unique(HOO_TYPE_DICT,         "DICT");
    insert_unique(HOO_TYPE_LIST,         "LIST");
    insert_unique(HOO_TYPE_DATETIME,     "DATETIME");
    insert_unique(HOO_TYPE_FUTURE,       "FUTURE");
    insert_unique(HOO_TYPE_DECIMAL,      "DECIMAL");
    insert_unique(HOO_TYPE_UV_HANDLE,    "UV_HANDLE");
    insert_unique(HOO_TYPE_TENSOR_SERIALIZED, "TENSOR_SERIALIZED");
    insert_unique(HOO_TYPE_NET_SOCKET,   "NET_SOCKET");
    insert_unique(HOO_TYPE_CONDITION,    "CONDITION");
    insert_unique(HOO_TYPE_SEMAPHORE,    "SEMAPHORE");
    insert_unique(HOO_TYPE_BYTE_SLICE,   "BYTE_SLICE");
    insert_unique(HOO_TYPE_TENSOR,       "TENSOR");
    insert_unique(HOO_TYPE_INT64,        "INT64");
    insert_unique(HOO_TYPE_FLOAT64,      "FLOAT64");

    EXPECT_EQ(ids.size(), 32u) << "Expected 32 unique runtime type IDs";
}

// ── Critical #2: Overload exception IDs don't collide ────────────────────

TEST_F(TypeIdConsistencyTest, OverloadExceptionIdsDoNotCollideWithRuntimeIds) {
    // Overload exception IDs must not share values with any managed-object
    // type ID, because the codegen uses the same ID space for both.
    EXPECT_NE(HOO_TYPE_AMBIGUOUS_OVERLOAD, HOO_TYPE_BYTE_SLICE);
    EXPECT_NE(HOO_TYPE_AMBIGUOUS_OVERLOAD, HOO_TYPE_TENSOR);
    EXPECT_NE(HOO_TYPE_NO_MATCHING_OVERLOAD, HOO_TYPE_BYTE_SLICE);
    EXPECT_NE(HOO_TYPE_NO_MATCHING_OVERLOAD, HOO_TYPE_TENSOR);
    EXPECT_NE(HOO_TYPE_AMBIGUOUS_OVERLOAD, HOO_TYPE_NO_MATCHING_OVERLOAD);
}

TEST_F(TypeIdConsistencyTest, OverloadExceptionIdsAreUniqueAmongThemselves) {
    EXPECT_NE(HOO_TYPE_AMBIGUOUS_OVERLOAD, HOO_TYPE_NO_MATCHING_OVERLOAD);
}

TEST_F(TypeIdConsistencyTest, OverloadExceptionIdsDoNotCollideWithAnyRuntimeId) {
    std::set<uint32_t> runtimeIds = {
        HOO_TYPE_OBJECT, HOO_TYPE_STRING, HOO_TYPE_ARRAY, HOO_TYPE_MAP,
        HOO_TYPE_EXCEPTION, HOO_TYPE_RANDOM, HOO_TYPE_NET_URL,
        HOO_TYPE_NET_HTTP_RES, HOO_TYPE_NET_HTTP_CLI, HOO_TYPE_CHARACTER,
        HOO_TYPE_UUID, HOO_TYPE_REGEX, HOO_TYPE_JSON, HOO_TYPE_BUFFER,
        HOO_TYPE_CSV, HOO_TYPE_ARGS, HOO_TYPE_COMPRESSION, HOO_TYPE_MUTEX,
        HOO_TYPE_DICT, HOO_TYPE_LIST, HOO_TYPE_DATETIME, HOO_TYPE_FUTURE,
        HOO_TYPE_DECIMAL, HOO_TYPE_UV_HANDLE, HOO_TYPE_TENSOR_SERIALIZED,
        HOO_TYPE_NET_SOCKET, HOO_TYPE_CONDITION, HOO_TYPE_SEMAPHORE,
        HOO_TYPE_BYTE_SLICE, HOO_TYPE_TENSOR,
        HOO_TYPE_INT64, HOO_TYPE_FLOAT64
    };

    EXPECT_EQ(runtimeIds.count(HOO_TYPE_AMBIGUOUS_OVERLOAD), 0u)
        << "HOO_TYPE_AMBIGUOUS_OVERLOAD (" << HOO_TYPE_AMBIGUOUS_OVERLOAD
        << ") collides with a runtime type ID";
    EXPECT_EQ(runtimeIds.count(HOO_TYPE_NO_MATCHING_OVERLOAD), 0u)
        << "HOO_TYPE_NO_MATCHING_OVERLOAD (" << HOO_TYPE_NO_MATCHING_OVERLOAD
        << ") collides with a runtime type ID";
}

// ── Specific ID values that were fixed ───────────────────────────────────

TEST_F(TypeIdConsistencyTest, ArgsHasCorrectId) {
    // Args was previously assigned 110 which collided with UUID.
    EXPECT_EQ(HOO_TYPE_ARGS, 120u);
}

TEST_F(TypeIdConsistencyTest, CompressionHasCorrectId) {
    // Compression was previously assigned 111 which collided with REGEX.
    EXPECT_EQ(HOO_TYPE_COMPRESSION, 121u);
}

TEST_F(TypeIdConsistencyTest, RegexMatchesRuntimeId) {
    // Regex was previously assigned 120 in the codegen; must match runtime.
    EXPECT_EQ(HOO_TYPE_REGEX, 111u);
}

TEST_F(TypeIdConsistencyTest, UuidMatchesRuntimeId) {
    // Uuid was previously assigned 122 in the codegen; must match runtime.
    EXPECT_EQ(HOO_TYPE_UUID, 110u);
}

TEST_F(TypeIdConsistencyTest, MutexHasCorrectId) {
    // Mutex was previously 121, reassigned to avoid collision.
    EXPECT_EQ(HOO_TYPE_MUTEX, 122u);
}

TEST_F(TypeIdConsistencyTest, TensorHasCorrectId) {
    // Tensor was previously mapped to 104 (EXCEPTION's ID) in the codegen.
    EXPECT_EQ(HOO_TYPE_TENSOR, 131u);
}

TEST_F(TypeIdConsistencyTest, OverloadExceptionIdsAreHighEnough) {
    // Both overload exception IDs must be above all managed-object IDs
    // to guarantee they never collide even as new types are added.
    EXPECT_GE(HOO_TYPE_AMBIGUOUS_OVERLOAD, 132u);
    EXPECT_GE(HOO_TYPE_NO_MATCHING_OVERLOAD, 133u);
}

// ── ID range sanity checks ──────────────────────────────────────────────

TEST_F(TypeIdConsistencyTest, ManagedObjectIdsAreAbove100) {
    // All managed object type IDs should be in the range [100, 140)
    // to distinguish them from primitive type IDs (1-9).
    EXPECT_GE(HOO_TYPE_OBJECT, 100u);
    EXPECT_GE(HOO_TYPE_STRING, 100u);
    EXPECT_GE(HOO_TYPE_ARRAY, 100u);
    EXPECT_GE(HOO_TYPE_MAP, 100u);
    EXPECT_GE(HOO_TYPE_EXCEPTION, 100u);
    EXPECT_GE(HOO_TYPE_TENSOR, 100u);
    EXPECT_GE(HOO_TYPE_BYTE_SLICE, 100u);
}

TEST_F(TypeIdConsistencyTest, PrimitiveIdsAreBelow100) {
    EXPECT_LT(HOO_TYPE_INT64, 100u);
    EXPECT_LT(HOO_TYPE_FLOAT64, 100u);
}

// ── Runtime can allocate and identify objects with the fixed IDs ─────────

TEST_F(TypeIdConsistencyTest, AllocTensorTypeReturnsCorrectTypeId) {
    void* obj = hoo_alloc(64, HOO_TYPE_TENSOR);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_type_id(obj), HOO_TYPE_TENSOR);
    EXPECT_EQ(hoo_get_type_id(obj), 131u);
    hoo_release(obj);
}

TEST_F(TypeIdConsistencyTest, AllocByteSliceTypeReturnsCorrectTypeId) {
    void* obj = hoo_alloc(64, HOO_TYPE_BYTE_SLICE);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_type_id(obj), HOO_TYPE_BYTE_SLICE);
    EXPECT_EQ(hoo_get_type_id(obj), 130u);
    hoo_release(obj);
}

TEST_F(TypeIdConsistencyTest, AllocUuidTypeReturnsCorrectTypeId) {
    void* obj = hoo_alloc(64, HOO_TYPE_UUID);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_type_id(obj), HOO_TYPE_UUID);
    EXPECT_EQ(hoo_get_type_id(obj), 110u);
    hoo_release(obj);
}

TEST_F(TypeIdConsistencyTest, AllocArgsTypeReturnsCorrectTypeId) {
    void* obj = hoo_alloc(64, HOO_TYPE_ARGS);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_type_id(obj), HOO_TYPE_ARGS);
    EXPECT_EQ(hoo_get_type_id(obj), 120u);
    hoo_release(obj);
}

TEST_F(TypeIdConsistencyTest, TensorAndByteSliceAreDistinctObjects) {
    // Verify two objects with different type IDs are distinguishable.
    void* tensor = hoo_alloc(64, HOO_TYPE_TENSOR);
    void* byteSlice = hoo_alloc(64, HOO_TYPE_BYTE_SLICE);
    ASSERT_NE(tensor, nullptr);
    ASSERT_NE(byteSlice, nullptr);

    EXPECT_EQ(hoo_get_type_id(tensor), HOO_TYPE_TENSOR);
    EXPECT_EQ(hoo_get_type_id(byteSlice), HOO_TYPE_BYTE_SLICE);
    EXPECT_NE(hoo_get_type_id(tensor), hoo_get_type_id(byteSlice));

    hoo_release(tensor);
    hoo_release(byteSlice);
}
