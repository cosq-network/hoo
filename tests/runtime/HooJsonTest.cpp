#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <functional>

#include "runtime/lib/anyarray/hoo_list.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include "runtime/lib/exception/hoo_exception.h"
#include "runtime/lib/hashmap/hoo_dict.h"
#include "runtime/lib/json/hoo_json.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/tensor/hoo_tensor.h"

class HooJsonTest : public ::testing::Test {};

static uint64_t testPointerToData(const void* ptr) {
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr));
}

static int64_t testDataToInt64(uint64_t data) {
    int64_t value = 0;
    std::memcpy(&value, &data, sizeof(value));
    return value;
}

static void expectJsonThrowsContaining(std::function<void()> fn, const char* messagePart) {
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

static void expectJsonThrows(std::function<void()> fn) {
    expectJsonThrowsContaining(fn, nullptr);
}

TEST_F(HooJsonTest, SerializeDictInt64Values) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_dict_set_fixed_i8(map, 1, 42), 1);
    ASSERT_EQ(hoo_dict_set_fixed_i8(map, 2, 7), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    const char* text = hoo_string_data(json);
    EXPECT_TRUE(std::strstr(text, "\"1\":42") != nullptr);
    EXPECT_TRUE(std::strstr(text, "\"2\":7") != nullptr);
    EXPECT_EQ(text[0], '{');

    hoo_string_release(json);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, SerializeDictStringValuesEscapesOutput) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_STRING);
    ASSERT_NE(map, nullptr);
    HooString value = hoo_string_from_cstr("hello\nworld");
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(hoo_dict_set_fixed_i8(map, 5, testPointerToData(value)), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "\"5\":\"hello\\nworld\""), nullptr);

    hoo_string_release(json);
    hoo_dict_release(map);
    hoo_string_release(value);
}

TEST_F(HooJsonTest, SerializeListMixedValues) {
    HooList values = hoo_list_new();
    ASSERT_NE(values, nullptr);
    HooString text = hoo_string_from_cstr("two");
    ASSERT_NE(text, nullptr);

    ASSERT_EQ(hoo_list_push(values, HOO_TYPE_INT64, 1), 1);
    ASSERT_EQ(hoo_list_push(values, HOO_TYPE_STRING, testPointerToData(text)), 1);
    ASSERT_EQ(hoo_list_push(values, HOO_TYPE_BOOL, 1), 1);

    HooString json = hoo_json_serialize_anyarray(values);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "[1,\"two\",true]");

    hoo_string_release(json);
    hoo_string_release(text);
    hoo_list_release(values);
}

TEST_F(HooJsonTest, SerializeListWithNestedHashMap) {
    HooList values = hoo_list_new();
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(values, nullptr);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_dict_set_fixed_i8(map, 9, 99), 1);
    ASSERT_EQ(hoo_list_push(values, HOO_TYPE_DICT, testPointerToData(map)), 1);

    HooString json = hoo_json_serialize_anyarray(values);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "[{\"9\":99}]");

    hoo_string_release(json);
    hoo_dict_release(map);
    hoo_list_release(values);
}

TEST_F(HooJsonTest, SerializeAndDeserializeBufferUsesTaggedBase64) {
    const uint8_t bytes[] = {0, 1, 2, 250, 255};
    HooBuffer buffer = hoo_buffer_from_bytes(bytes, static_cast<int64_t>(sizeof(bytes)));
    ASSERT_NE(buffer, nullptr);

    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_dict_set_any_i8(map, 4, HOO_TYPE_BUFFER, testPointerToData(buffer)), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "__hoo_buffer__"), nullptr);

    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 4, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_BUFFER);
    HooBuffer decodedBuffer = reinterpret_cast<HooBuffer>(value.data);
    ASSERT_EQ(hoo_buffer_length(decodedBuffer), static_cast<int64_t>(sizeof(bytes)));
    EXPECT_EQ(std::memcmp(hoo_buffer_data(decodedBuffer), bytes, sizeof(bytes)), 0);

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
    hoo_buffer_release(buffer);
}

TEST_F(HooJsonTest, SerializeAndDeserializeTensorPreservesShapeAndBits) {
    HooTensor tensor = hoo_tensor_new1(1 /* int64 */, 3);
    ASSERT_NE(tensor, nullptr);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 0, 11), 1);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 1, 22), 1);
    ASSERT_EQ(hoo_tensor_set_value(tensor, 2, 33), 1);

    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(hoo_dict_set_any_i8(map, 8, HOO_TYPE_TENSOR_SERIALIZED, testPointerToData(tensor)), 1);
    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);

    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 8, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_TENSOR_SERIALIZED);
    HooTensor decodedTensor = reinterpret_cast<HooTensor>(value.data);
    ASSERT_EQ(hoo_tensor_rank(decodedTensor), 1);
    ASSERT_EQ(hoo_tensor_dim(decodedTensor, 0), 3);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 0), 11);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 1), 22);
    EXPECT_EQ(hoo_tensor_get_int64(decodedTensor, 2), 33);

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
    hoo_release(tensor);
}

TEST_F(HooJsonTest, SerializeUnsupportedAnyValueThrowsRuntimeException) {
    HooList values = hoo_list_new();
    ASSERT_NE(values, nullptr);
    ASSERT_EQ(hoo_list_push(values, HOO_TYPE_CHAR, 65), 1);

    try {
        (void)hoo_json_serialize_anyarray(values);
        FAIL() << "Expected JSON serialization exception";
    } catch (const std::exception&) {
        HooException exc = hoo_exception_current();
        ASSERT_NE(exc, nullptr);
        EXPECT_NE(std::strstr(hoo_exception_get_message(exc), "unsupported value type"), nullptr);
        hoo_exception_clear();
    }

    hoo_list_release(values);
}

TEST_F(HooJsonTest, DeserializeDictObject) {
    HooDict map = hoo_json_deserialize_hashmap(R"({"1":42,"2":"two","3":[7,true]})");
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_dict_count(map), 3);

    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(map, 1, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(testDataToInt64(value.data), 42);

    ASSERT_EQ(hoo_dict_get_any_i8(map, 2, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_STRING);
    EXPECT_STREQ(hoo_string_data(reinterpret_cast<HooString>(value.data)), "two");

    ASSERT_EQ(hoo_dict_get_any_i8(map, 3, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_LIST);
    HooList nested = reinterpret_cast<HooList>(value.data);
    EXPECT_EQ(hoo_list_length(nested), 2);

    hoo_dict_release(map);
}

TEST_F(HooJsonTest, DeserializeList) {
    HooList array = hoo_json_deserialize_anyarray(R"([1,"two",false,null,{"7":8}])");
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(hoo_list_length(array), 5);

    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_list_get(array, 1, &value), 1);
    ASSERT_EQ(value.type_id, HOO_TYPE_STRING);
    EXPECT_STREQ(hoo_string_data(reinterpret_cast<HooString>(value.data)), "two");

    ASSERT_EQ(hoo_list_get(array, 2, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_BOOL);
    EXPECT_EQ(value.data, 0ULL);

    ASSERT_EQ(hoo_list_get(array, 3, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_VOID);
    EXPECT_EQ(value.data, 0ULL);

    ASSERT_EQ(hoo_list_get(array, 4, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_DICT);

    hoo_list_release(array);
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

// ============================================================================
// Float64 round-trip tests
// ============================================================================

TEST_F(HooJsonTest, Float64IntegralValuePreservesDecimalPoint) {
    // 2.0 must serialize as "2.0" (not "2") so it deserializes back as f64
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    double val = 2.0;
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(val));
    ASSERT_EQ(hoo_dict_set_any_i8(map, 1, HOO_TYPE_FLOAT64, bits), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    // Must contain "2.0" not "2"
    EXPECT_NE(std::strstr(hoo_string_data(json), "2.0"), nullptr)
        << "float 2.0 must serialize as 2.0, got: " << hoo_string_data(json);

    // Deserialize back — must be f64, not int64
    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue result{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 1, &result), 1);
    EXPECT_EQ(result.type_id, HOO_TYPE_FLOAT64)
        << "2.0 must deserialize back as f64, not int64";

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, Float64NonIntegralPreservesValue) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    double val = 3.14;
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(val));
    ASSERT_EQ(hoo_dict_set_any_i8(map, 1, HOO_TYPE_FLOAT64, bits), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    // Should contain "3.14"
    EXPECT_NE(std::strstr(hoo_string_data(json), "3.14"), nullptr);

    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue result{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 1, &result), 1);
    EXPECT_EQ(result.type_id, HOO_TYPE_FLOAT64);

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, Float64ZeroPreservesDecimalPoint) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    double val = 0.0;
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(val));
    ASSERT_EQ(hoo_dict_set_any_i8(map, 1, HOO_TYPE_FLOAT64, bits), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    // "0.0" must have decimal point
    EXPECT_NE(std::strstr(hoo_string_data(json), "0.0"), nullptr);

    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue result{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 1, &result), 1);
    EXPECT_EQ(result.type_id, HOO_TYPE_FLOAT64);

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, Float64NegativePreservesDecimalPoint) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    ASSERT_NE(map, nullptr);
    double val = -5.0;
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(val));
    ASSERT_EQ(hoo_dict_set_any_i8(map, 1, HOO_TYPE_FLOAT64, bits), 1);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "-5.0"), nullptr);

    HooDict decoded = hoo_json_deserialize_hashmap(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue result{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(decoded, 1, &result), 1);
    EXPECT_EQ(result.type_id, HOO_TYPE_FLOAT64);

    hoo_string_release(json);
    hoo_dict_release(decoded);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, Float64InArrayRoundTrip) {
    HooList list = hoo_list_new();
    ASSERT_NE(list, nullptr);

    double val = 1.0;
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(val));
    ASSERT_EQ(hoo_list_push(list, HOO_TYPE_FLOAT64, bits), 1);

    HooString json = hoo_json_serialize_anyarray(list);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(hoo_string_data(json), "1.0"), nullptr);

    HooList decoded = hoo_json_deserialize_anyarray(hoo_string_data(json));
    ASSERT_NE(decoded, nullptr);
    HooAnyValue result{0, 0};
    ASSERT_EQ(hoo_list_get(decoded, 0, &result), 1);
    EXPECT_EQ(result.type_id, HOO_TYPE_FLOAT64);

    hoo_string_release(json);
    hoo_list_release(decoded);
    hoo_list_release(list);
}

// ============================================================================
// Empty container tests
// ============================================================================

TEST_F(HooJsonTest, SerializeEmptyDict) {
    HooDict map = hoo_dict_new(HOO_TYPE_INT64, HOO_TYPE_INT64);
    ASSERT_NE(map, nullptr);

    HooString json = hoo_json_serialize_hashmap(map);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "{}");

    hoo_string_release(json);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, SerializeEmptyList) {
    HooList list = hoo_list_new();
    ASSERT_NE(list, nullptr);

    HooString json = hoo_json_serialize_anyarray(list);
    ASSERT_NE(json, nullptr);
    EXPECT_STREQ(hoo_string_data(json), "[]");

    hoo_string_release(json);
    hoo_list_release(list);
}

TEST_F(HooJsonTest, DeserializeEmptyObject) {
    HooDict map = hoo_json_deserialize_hashmap("{}");
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_dict_count(map), 0);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, DeserializeEmptyArray) {
    HooList list = hoo_json_deserialize_anyarray("[]");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(hoo_list_length(list), 0);
    hoo_list_release(list);
}

// ============================================================================
// Boundary integer tests
// ============================================================================

TEST_F(HooJsonTest, DeserializeInt64MaxValue) {
    HooDict map = hoo_json_deserialize_hashmap(R"({"1":9223372036854775807})");
    ASSERT_NE(map, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(map, 1, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(testDataToInt64(value.data), 9223372036854775807LL);
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, DeserializeInt64MinValue) {
    HooDict map = hoo_json_deserialize_hashmap(R"({"1":-9223372036854775808})");
    ASSERT_NE(map, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(map, 1, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(testDataToInt64(value.data), (-9223372036854775807LL - 1));
    hoo_dict_release(map);
}

TEST_F(HooJsonTest, DeserializeInt64Zero) {
    HooDict map = hoo_json_deserialize_hashmap(R"({"1":0})");
    ASSERT_NE(map, nullptr);
    HooAnyValue value{0, 0};
    ASSERT_EQ(hoo_dict_get_any_i8(map, 1, &value), 1);
    EXPECT_EQ(value.type_id, HOO_TYPE_INT64);
    EXPECT_EQ(testDataToInt64(value.data), 0);
    hoo_dict_release(map);
}

// ============================================================================
// Trailing garbage rejection
// ============================================================================

TEST_F(HooJsonTest, DeserializeRejectsTrailingGarbage) {
    expectJsonThrows([]() { (void)hoo_json_deserialize_hashmap("{} x"); });
    expectJsonThrows([]() { (void)hoo_json_deserialize_hashmap("[]  "); });
    expectJsonThrows([]() { (void)hoo_json_minify("42 extra"); });
}

// ============================================================================
// Leading zeros rejection
// ============================================================================

TEST_F(HooJsonTest, DeserializeRejectsLeadingZeros) {
    expectJsonThrows([]() { (void)hoo_json_deserialize_hashmap(R"({"1":07})"); });
    expectJsonThrows([]() { (void)hoo_json_deserialize_anyarray("[00]"); });
}

// ============================================================================
// Recursion depth limit tests
// ============================================================================

TEST_F(HooJsonTest, DeserializeRejectsDeeplyNestedArray) {
    // Build a 300-deep nested array
    std::string deep(300, '[');
    deep += "1";
    deep += std::string(300, ']');
    expectJsonThrowsContaining(
        [&]() { (void)hoo_json_deserialize_anyarray(deep.c_str()); },
        "depth exceeds maximum"
    );
}

TEST_F(HooJsonTest, DeserializeRejectsDeeplyNestedObject) {
    // Build a 300-deep nested object
    std::string deep;
    for (int i = 0; i < 300; i++) deep += "{\"1\":";
    deep += "42";
    deep += std::string(300, '}');
    expectJsonThrowsContaining(
        [&]() { (void)hoo_json_deserialize_hashmap(deep.c_str()); },
        "depth exceeds maximum"
    );
}

TEST_F(HooJsonTest, DeserializeAcceptsMaxDepthExactly) {
    // 255-deep nested array should succeed (parser allows up to 256)
    std::string deep(255, '[');
    deep += "1";
    deep += std::string(255, ']');
    HooList list = hoo_json_deserialize_anyarray(deep.c_str());
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(hoo_list_length(list), 1);
    hoo_list_release(list);
}

// ============================================================================
// Null input rejection for serialize
// ============================================================================

TEST_F(HooJsonTest, SerializeNullDictThrows) {
    expectJsonThrowsContaining([]() { (void)hoo_json_serialize_hashmap(nullptr); }, "serialization failed");
}

TEST_F(HooJsonTest, SerializeNullListThrows) {
    expectJsonThrowsContaining([]() { (void)hoo_json_serialize_anyarray(nullptr); }, "serialization failed");
}

// ============================================================================
// Minify/beautify scalar tests
// ============================================================================

TEST_F(HooJsonTest, MinifyScalar) {
    HooString result = hoo_json_minify("42");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "42");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyScalar) {
    HooString result = hoo_json_beautify("true");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "true");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, MinifyNull) {
    HooString result = hoo_json_minify("null");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "null");
    hoo_string_release(result);
}
