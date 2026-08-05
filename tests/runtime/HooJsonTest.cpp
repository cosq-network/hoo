#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "runtime/lib/hoo_anyarray.h"
#include "runtime/lib/hoo_buffer.h"
#include "runtime/lib/hoo_exception.h"
#include "runtime/lib/hoo_hashmap.h"
#include "runtime/lib/hoo_json.h"
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_string.h"
#include "runtime/lib/hoo_tensor.h"

class HooJsonTest : public ::testing::Test {};

static uint64_t testPointerToData(const void* ptr) {
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr));
}

static int64_t testDataToInt64(uint64_t data) {
    int64_t value = 0;
    std::memcpy(&value, &data, sizeof(value));
    return value;
}

static void expectJsonThrowsContaining(void (*fn)(), const char* messagePart) {
    try {
        fn();
        FAIL() << "Expected JSON runtime exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        const char* message = hoo_exception_get_message(exc);
        EXPECT_NE(std::strstr(message, "JSON"), nullptr) << message;
        if (messagePart) {
            EXPECT_NE(std::strstr(message, messagePart), nullptr) << message;
        }
        hoo_exception_clear();
    }
}

static void expectJsonThrows(void (*fn)()) {
    expectJsonThrowsContaining(fn, nullptr);
}

TEST_F(HooJsonTest, SerializeHashMapInt64Values) {
    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_hashmap_set_fixed_i8(map, 1, 42), 1);
    ASSERT_EQ(hoo_hashmap_set_fixed_i8(map, 2, 7), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    const char* text = hoo_string_data(json);
    EXPECT_TRUE(std::strstr(text, "\"1\":42") != nullptr);
    EXPECT_TRUE(std::strstr(text, "\"2\":7") != nullptr);
    EXPECT_EQ(text[0], '{');

    hoo_string_release(json);
    hoo_hashmap_release(map);
}

TEST_F(HooJsonTest, SerializeHashMapStringValuesEscapesOutput) {
    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_STRING);
    ASSERT_NE(map, nullptr);
    HooString value = hoo_string_from_cstr("hello\nworld");
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(hoo_hashmap_set_fixed_i8(map, 5, testPointerToData(value)), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "\"5\":\"hello\\nworld\""), nullptr);

    hoo_string_release(json);
    hoo_hashmap_release(map);
    hoo_string_release(value);
}

TEST_F(HooJsonTest, SerializeAnyArrayMixedValues) {
    HooAnyArray values = hoo_anyarray_new();
    ASSERT_NE(values, nullptr);
    HooString text = hoo_string_from_cstr("two");
    ASSERT_NE(text, nullptr);

    ASSERT_EQ(hoo_anyarray_push(values, HOO_TYPE_INT64, 1), 1);
    ASSERT_EQ(hoo_anyarray_push(values, HOO_TYPE_STRING, testPointerToData(text)), 1);
    ASSERT_EQ(hoo_anyarray_push(values, HOO_TYPE_BOOL, 1), 1);

    HooString json = hoo_json_serialize_anyarray(values);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "[1,\"two\",true]");

    hoo_string_release(json);
    hoo_string_release(text);
    hoo_anyarray_release(values);
}

TEST_F(HooJsonTest, SerializeAnyArrayWithNestedHashMap) {
    HooAnyArray values = hoo_anyarray_new();
    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(values, nullptr);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_hashmap_set_fixed_i8(map, 9, 99), 1);
    ASSERT_EQ(hoo_anyarray_push(values, HOO_TYPE_HASHMAP, testPointerToData(map)), 1);

    HooString json = hoo_json_serialize_anyarray(values);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "[{\"9\":99}]");

    hoo_string_release(json);
    hoo_hashmap_release(map);
    hoo_anyarray_release(values);
}

TEST_F(HooJsonTest, SerializeAndDeserializeBufferUsesTaggedBase64) {
    const uint8_t bytes[] = {0, 1, 2, 250, 255};
    HooBuffer buffer = hoo_buffer_from_bytes(bytes, static_cast<int64_t>(sizeof(bytes)));
    ASSERT_NE(buffer, nullptr);

    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_hashmap_set_any_i8(map, 4, HOO_TYPE_BUFFER, testPointerToData(buffer)), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "__hoo_buffer__"), nullptr);

    HooHashMap decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_hashmap_get_any_i8(decoded, 4, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_BUFFER);
    HooBuffer decodedBuffer = reinterpret_cast<HooBuffer>(value.data);
    ASSERT_EQ(hoo_buffer_length(decodedBuffer), static_cast<int64_t>(sizeof(bytes)));
    EXPECT_EQ(std::memcmp(hoo_buffer_data(decodedBuffer), bytes, sizeof(bytes)), 0);

    hoo_string_release(json);
    hoo_hashmap_release(decoded);
    hoo_hashmap_release(map);
    hoo_buffer_release(buffer);
}

TEST_F(HooJsonTest, SerializeAndDeserializeTensorPreservesShapeAndBits) {
    HooTensor tensor = hoo_tensor_new1(1 /* int64 */, 3);
    ASSERT_NE(tensor, nullptr);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 0, 11), 1);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 1, 22), 1);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 2, 33), 1);

    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_hashmap_set_any_i8(map, 8, HOO_TYPE_TENSOR_SERIALIZED, testPointerToData(tensor)), 1);
    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);

    HooHashMap decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_hashmap_get_any_i8(decoded, 8, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_TENSOR_SERIALIZED);
    HooTensor decodedTensor = reinterpret_cast<HooTensor>(value.data);
    ASSERT_EQ(hoo_tensor_rank(decodedTensor), 1);
    ASSERT_EQ(hoo_tensor_dim(decodedTensor, 0), 3);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 0), 11);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 1), 22);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 2), 33);

    hoo_string_release(json);
    hoo_hashmap_release(decoded);
    hoo_hashmap_release(map);
    hoo_release(tensor);
}

TEST_F(HooJsonTest, SerializeUnsupportedAnyValueThrowsRuntimeException) {
    HooAnyArray values = hoo_anyarray_new();
    ASSERT_NE(values, nullptr);
    ASSERT_EQ(hoo_anyarray_push(values, HOO_TYPE_CHAR, 65), 1);

    try {
        (void)hoo_json_serialize_anyarray(values);
        FAIL() << "Expected JSON serialization exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        EXPECT_NE(std::strstr(hoo_exception_get_message(exc), "unsupported value type"), nullptr);
        hoo_exception_clear();
    }

    hoo_anyarray_release(values);
}

TEST_F(HooJsonTest, DeserializeHashMapObject) {
    HooHashMap map = hoo_json_deserialize_hashmap(R"({"1":42,"2":"two","3":[7,true]})");
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_hashmap_count(map), 3);

    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_hashmap_get_any_i8(map, 1, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(testDataToInt64(value.data), 42);

    ASSERT_EQ(hoo_hashmap_get_any_i8(map, 2, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_STRING);
    EXPECT_STREQ(hoo_string_data(reinterpret_cast<HooString>(value.data)), "two");

    ASSERT_EQ(hoo_hashmap_get_any_i8(map, 3, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_ANYARRAY);
    HooAnyArray nested = reinterpret_cast<HooAnyArray>(value.data);
    EXPECT_EQ(hoo_anyarray_length(nested), 2);

    hoo_hashmap_release(map);
}

TEST_F(HooJsonTest, DeserializeAnyArray) {
    HooAnyArray array = hoo_json_deserialize_anyarray(R"([1,"two",false,null,{"7":8}])");
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(hoo_anyarray_length(array), 5);

    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_anyarray_get(array, 1, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_STRING);
    EXPECT_STREQ(hoo_string_data(reinterpret_cast<HooString>(value.data)), "two");

    ASSERT_EQ(hoo_anyarray_get(array, 2, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_BOOL);
    EXPECT_EQ(value.data, 0ULL);

    ASSERT_EQ(hoo_anyarray_get(array, 3, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_VOID);
    EXPECT_EQ(value.data, 0ULL);

    ASSERT_EQ(hoo_anyarray_get(array, 4, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_HASHMAP);

    hoo_anyarray_release(array);
}

TEST_F(HooJsonTest, DeserializeRejectsWrongRootAndNonIntegerKeys) {
    try {
        (void)hoo_json_deserialize_hashmap("[1,2]");
        FAIL() << "Expected JSON deserialization exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        EXPECT_NE(std::strstr(hoo_exception_get_message(exc), "not an object"), nullptr);
        hoo_exception_clear();
    }

    try {
        (void)hoo_json_deserialize_hashmap(R"({"name":"Alice"})");
        FAIL() << "Expected JSON deserialization exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        EXPECT_NE(std::strstr(hoo_exception_get_message(exc), "valid int64"), nullptr);
        hoo_exception_clear();
    }
}

TEST_F(HooJsonTest, DeserializeRejectsWrongArrayRoot) {
    try {
        (void)hoo_json_deserialize_anyarray(R"({"1":2})");
        FAIL() << "Expected JSON deserialization exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        EXPECT_NE(std::strstr(hoo_exception_get_message(exc), "not an array"), nullptr);
        hoo_exception_clear();
    }
}

TEST_F(HooJsonTest, DeserializeRejectsMalformedJsonWithRuntimeException) {
    expectJsonThrowsContaining([]() { (void)hoo_json_deserialize_hashmap(R"({"1": [2,)"); }, "deserialization failed");
    expectJsonThrowsContaining([]() { (void)hoo_json_deserialize_anyarray("[1,]"); }, "deserialization failed");
}

TEST_F(HooJsonTest, DeserializeRejectsNullInputWithRuntimeException) {
    expectJsonThrowsContaining([]() { (void)hoo_json_deserialize_hashmap(nullptr); }, "input string is nil");
    expectJsonThrowsContaining([]() { (void)hoo_json_deserialize_anyarray(nullptr); }, "input string is nil");
}

TEST_F(HooJsonTest, MinifyValidJson) {
    HooString result = hoo_json_minify("{ \"b\" : [ 1, true, null ], \"a\" : \"x y\" }");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "{\"b\":[1,true,null],\"a\":\"x y\"}");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyValidJson) {
    HooString result = hoo_json_beautify("{\"a\":1,\"b\":[2,3]}");
    ASSERT_NE(result, nullptr);
    const char* text = hoo_string_data(result);
    EXPECT_NE(std::strstr(text, "\n  \"a\": 1"), nullptr);
    EXPECT_NE(std::strstr(text, "\n  \"b\": ["), nullptr);
    hoo_string_release(result);
}

TEST_F(HooJsonTest, FormattingRejectsMalformedNumbersAndStrings) {
    expectJsonThrows([]() { (void)hoo_json_minify("-"); });
    expectJsonThrows([]() { (void)hoo_json_minify("1e"); });
    expectJsonThrows([]() { (void)hoo_json_minify("1."); });
    expectJsonThrows([]() { (void)hoo_json_minify("\"unterminated"); });
    expectJsonThrows([]() { (void)hoo_json_minify("\"bad\\qescape\""); });
    expectJsonThrowsContaining([]() { (void)hoo_json_beautify("{"); }, "beautification failed");
}

TEST_F(HooJsonTest, FormattingRejectsNullInput) {
    expectJsonThrows([]() { (void)hoo_json_minify(nullptr); });
    expectJsonThrows([]() { (void)hoo_json_beautify(nullptr); });
}
