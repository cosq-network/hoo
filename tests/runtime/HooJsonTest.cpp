#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "runtime/lib/hoo_json.h"
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_string.h"

class HooJsonTest : public ::testing::Test {
};

// ============================================================================
// hoo_json_parse_to_map
// ============================================================================

TEST_F(HooJsonTest, ParseToMapSimple) {
    const char* json = R"({"name":"Alice","age":"30","active":"true"})";
    HooMap map = hoo_json_parse_to_map(json);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 3);

    const char* name = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "name", &name));
    EXPECT_STREQ(name, "Alice");

    const char* age = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "age", &age));
    EXPECT_STREQ(age, "30");

    const char* active = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "active", &active));
    EXPECT_STREQ(active, "true");

    hoo_map_release(map);
}

TEST_F(HooJsonTest, ParseToMapNumericValues) {
    const char* json = R"({"x":42,"pi":3.14,"neg":-10})";
    HooMap map = hoo_json_parse_to_map(json);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 3);

    const char* x = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "x", &x));
    EXPECT_STREQ(x, "42");

    const char* pi = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "pi", &pi));
    EXPECT_STREQ(pi, "3.14");

    const char* neg = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "neg", &neg));
    EXPECT_STREQ(neg, "-10");

    hoo_map_release(map);
}

TEST_F(HooJsonTest, ParseToMapBoolAndNull) {
    const char* json = R"({"yes":true,"no":false,"nothing":null})";
    HooMap map = hoo_json_parse_to_map(json);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 3);

    const char* yes = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "yes", &yes));
    EXPECT_STREQ(yes, "true");

    const char* no = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "no", &no));
    EXPECT_STREQ(no, "false");

    const char* nothing = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "nothing", &nothing));
    EXPECT_STREQ(nothing, "null");

    hoo_map_release(map);
}

TEST_F(HooJsonTest, ParseToMapUnescapedStrings) {
    const char* json = R"({"msg":"hello\nworld","path":"C:\\dir"})";
    HooMap map = hoo_json_parse_to_map(json);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 2);

    const char* msg = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "msg", &msg));
    EXPECT_STREQ(msg, "hello\nworld");

    const char* path = nullptr;
    ASSERT_TRUE(hoo_map_try_get(map, "path", &path));
    EXPECT_STREQ(path, "C:\\dir");

    hoo_map_release(map);
}

TEST_F(HooJsonTest, ParseToMapEmptyObject) {
    HooMap map = hoo_json_parse_to_map("{}");
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(hoo_map_count(map), 0);
    hoo_map_release(map);
}

TEST_F(HooJsonTest, ParseToMapReturnsNullForArray) {
    HooMap map = hoo_json_parse_to_map("[1,2,3]");
    EXPECT_EQ(map, nullptr);
}

TEST_F(HooJsonTest, ParseToMapReturnsNullForNestedObject) {
    HooMap map = hoo_json_parse_to_map(R"({"outer":{"inner":1}})");
    EXPECT_EQ(map, nullptr);
}

TEST_F(HooJsonTest, ParseToMapReturnsNullForInvalidJson) {
    HooMap map = hoo_json_parse_to_map("{invalid}");
    EXPECT_EQ(map, nullptr);
}

TEST_F(HooJsonTest, ParseToMapReturnsNullForNullInput) {
    HooMap map = hoo_json_parse_to_map(nullptr);
    EXPECT_EQ(map, nullptr);
}

// ============================================================================
// hoo_json_serialize_map
// ============================================================================

TEST_F(HooJsonTest, SerializeMapEmpty) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
    HooString result = hoo_json_serialize_map(map);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "{}");
    hoo_string_release(result);
    hoo_map_release(map);
}

TEST_F(HooJsonTest, SerializeMapStringValues) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
    hoo_map_set(map, "name", "Alice");
    hoo_map_set(map, "city", "Seattle");
    HooString result = hoo_json_serialize_map(map);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    EXPECT_TRUE(std::strstr(str, "\"name\":\"Alice\"") != nullptr);
    EXPECT_TRUE(std::strstr(str, "\"city\":\"Seattle\"") != nullptr);
    EXPECT_TRUE(str[0] == '{' && str[std::strlen(str) - 1] == '}');
    hoo_string_release(result);
    hoo_map_release(map);
}

TEST_F(HooJsonTest, SerializeMapAutoDetectTypes) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
    hoo_map_set(map, "name", "Alice");
    hoo_map_set(map, "age", "30");
    hoo_map_set(map, "pi", "3.14");
    hoo_map_set(map, "active", "true");
    hoo_map_set(map, "data", "null");
    HooString result = hoo_json_serialize_map(map);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    // Strings: quoted
    EXPECT_TRUE(std::strstr(str, "\"name\":\"Alice\"") != nullptr);
    // Numbers: unquoted
    EXPECT_TRUE(std::strstr(str, "\"age\":30") != nullptr);
    // Float: unquoted
    EXPECT_TRUE(std::strstr(str, "\"pi\":3.14") != nullptr);
    // Bool: unquoted
    EXPECT_TRUE(std::strstr(str, "\"active\":true") != nullptr);
    // Null: unquoted
    EXPECT_TRUE(std::strstr(str, "\"data\":null") != nullptr);
    hoo_string_release(result);
    hoo_map_release(map);
}

TEST_F(HooJsonTest, SerializeMapEscapesStrings) {
    HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
    hoo_map_set(map, "msg", "hello\nworld");
    hoo_map_set(map, "path", "C:\\dir");
    HooString result = hoo_json_serialize_map(map);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    EXPECT_TRUE(std::strstr(str, "\"hello\\nworld\"") != nullptr);
    EXPECT_TRUE(std::strstr(str, "\"C:\\\\dir\"") != nullptr);
    hoo_string_release(result);
    hoo_map_release(map);
}

TEST_F(HooJsonTest, SerializeMapReturnsNullForNullInput) {
    HooString result = hoo_json_serialize_map(nullptr);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// hoo_json_minify
// ============================================================================

TEST_F(HooJsonTest, MinifySimple) {
    const char* json = R"({  "name" : "Alice" ,  "age" : 30  })";
    HooString result = hoo_json_minify(json);
    ASSERT_NE(result, nullptr);
    // Fields are reordered alphabetically by std::map
    const char* str = hoo_string_data(result);
    EXPECT_TRUE(str[0] == '{' && str[std::strlen(str) - 1] == '}');
    EXPECT_TRUE(std::strstr(str, "\"age\":30") != nullptr);
    EXPECT_TRUE(std::strstr(str, "\"name\":\"Alice\"") != nullptr);
    // No whitespace outside strings
    EXPECT_TRUE(std::strstr(str, " ") == nullptr);
    hoo_string_release(result);
}

TEST_F(HooJsonTest, MinifyNested) {
    const char* json = R"({
        "user": {
            "name": "Alice",
            "scores": [95, 87, 92]
        }
    })";
    HooString result = hoo_json_minify(json);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    // No whitespace outside strings
    EXPECT_TRUE(std::strstr(str, "\"name\":\"Alice\"") != nullptr);
    EXPECT_TRUE(std::strstr(str, "95,87,92") != nullptr);
    hoo_string_release(result);
}

TEST_F(HooJsonTest, MinifyPreservesStringWhitespace) {
    const char* json = R"({"msg":"hello world"})";
    HooString result = hoo_json_minify(json);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), R"({"msg":"hello world"})");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, MinifyReturnsNullForInvalidJson) {
    HooString result = hoo_json_minify("not json");
    EXPECT_EQ(result, nullptr);
}

TEST_F(HooJsonTest, MinifyReturnsNullForNullInput) {
    HooString result = hoo_json_minify(nullptr);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// hoo_json_beautify
// ============================================================================

TEST_F(HooJsonTest, BeautifySimple) {
    const char* json = R"({"name":"Alice","age":30})";
    HooString result = hoo_json_beautify(json);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    // Should have newlines and indentation
    EXPECT_TRUE(std::strstr(str, "\n  ") != nullptr);
    EXPECT_TRUE(str[0] == '{');
    EXPECT_TRUE(std::strstr(str, "\"}") != nullptr || std::strstr(str, "\"\n}") != nullptr);
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyRoundTrip) {
    const char* json = R"({"name":"Alice","age":30,"active":true})";
    HooString beautified = hoo_json_beautify(json);
    ASSERT_NE(beautified, nullptr);
    HooString minified = hoo_json_minify(hoo_string_data(beautified));
    ASSERT_NE(minified, nullptr);
    // Round-trip: fields are reordered alphabetically by std::map
    const char* result = hoo_string_data(minified);
    // The alphabetically ordered result is: active, age, name
    EXPECT_STREQ(result, R"({"active":true,"age":30,"name":"Alice"})");
    hoo_string_release(minified);
    hoo_string_release(beautified);
}

TEST_F(HooJsonTest, BeautifyNested) {
    const char* json = R"({"user":{"name":"Alice","scores":[95,87,92]}})";
    HooString result = hoo_json_beautify(json);
    ASSERT_NE(result, nullptr);
    const char* str = hoo_string_data(result);
    // Nested indentation
    EXPECT_TRUE(std::strstr(str, "\n    ") != nullptr);
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyEmptyObject) {
    HooString result = hoo_json_beautify("{}");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "{}");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyEmptyArray) {
    HooString result = hoo_json_beautify("[]");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "[]");
    hoo_string_release(result);
}

TEST_F(HooJsonTest, BeautifyReturnsNullForInvalidJson) {
    HooString result = hoo_json_beautify("not json");
    EXPECT_EQ(result, nullptr);
}

TEST_F(HooJsonTest, BeautifyReturnsNullForNullInput) {
    HooString result = hoo_json_beautify(nullptr);
    EXPECT_EQ(result, nullptr);
}
