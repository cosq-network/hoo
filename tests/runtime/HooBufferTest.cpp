#include <gtest/gtest.h>
#include "../src/runtime/lib/hoo_buffer.h"
#include "../src/runtime/lib/hoo_runtime.h"
#include <cstring>
#include <limits>
#include <cstdlib>

class HooBufferTest : public ::testing::Test {
protected:
    void TearDown() override {}
};

TEST_F(HooBufferTest, CreateEmptyBuffer) {
    HooBuffer buf = hoo_buffer_new(0);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), 0);
    EXPECT_GE(hoo_buffer_capacity(buf), 0);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, CreateWithInitialCapacity) {
    HooBuffer buf = hoo_buffer_new(64);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), 0);
    EXPECT_GE(hoo_buffer_capacity(buf), 64);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, CreateFromBytes) {
    const uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    HooBuffer buf = hoo_buffer_from_bytes(data, 5);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), 5);
    const uint8_t* bufData = hoo_buffer_data(buf);
    ASSERT_NE(bufData, nullptr);
    EXPECT_EQ(std::memcmp(bufData, data, 5), 0);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, CreateFromEmptyBytes) {
    HooBuffer buf = hoo_buffer_from_bytes(nullptr, 0);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), 0);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, RejectOversizedByteLength) {
    const uint8_t data = 1;
    HooBuffer buf = hoo_buffer_from_bytes(&data, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(buf, nullptr);
}

TEST_F(HooBufferTest, CopyBuffer) {
    const uint8_t data[] = {'H', 'e', 'l', 'l', 'o'};
    HooBuffer buf = hoo_buffer_from_bytes(data, 5);
    ASSERT_NE(buf, nullptr);

    HooBuffer copy = hoo_buffer_copy(buf);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(hoo_buffer_length(copy), 5);

    const uint8_t* copyData = hoo_buffer_data(copy);
    ASSERT_NE(copyData, nullptr);
    EXPECT_EQ(std::memcmp(copyData, data, 5), 0);

    hoo_buffer_release(buf);
    hoo_buffer_release(copy);
}

TEST_F(HooBufferTest, ReferenceCounting) {
    HooBuffer buf = hoo_buffer_new(0);
    ASSERT_NE(buf, nullptr);

    EXPECT_EQ(hoo_get_refcount(buf), 1);

    HooBuffer buf2 = hoo_buffer_retain(buf);
    EXPECT_EQ(buf, buf2);
    EXPECT_EQ(hoo_get_refcount(buf), 2);

    hoo_buffer_release(buf);
    EXPECT_EQ(hoo_get_refcount(buf2), 1);

    hoo_buffer_release(buf2);
}

TEST_F(HooBufferTest, ByteAtValidIndex) {
    const uint8_t data[] = {10, 20, 30, 40, 50};
    HooBuffer buf = hoo_buffer_from_bytes(data, 5);
    ASSERT_NE(buf, nullptr);

    EXPECT_EQ(hoo_buffer_byte_at(buf, 0), 10);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 2), 30);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 4), 50);

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, ByteAtInvalidIndex) {
    const uint8_t data[] = {1, 2, 3};
    HooBuffer buf = hoo_buffer_from_bytes(data, 3);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 5), -1);
    EXPECT_EQ(hoo_buffer_byte_at(buf, -1), -1);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SetByte) {
    HooBuffer buf = hoo_buffer_new(10);
    ASSERT_NE(buf, nullptr);

    // Append initial bytes so we have space
    hoo_buffer_append(buf, (const uint8_t*)"\x00\x00\x00", 3);
    EXPECT_EQ(hoo_buffer_length(buf), 3);

    // Now set bytes
    EXPECT_EQ(hoo_buffer_set_byte(buf, 0, 0xAB), 0x00);
    EXPECT_EQ(hoo_buffer_set_byte(buf, 1, 0xCD), 0x00);
    EXPECT_EQ(hoo_buffer_set_byte(buf, 2, 0xEF), 0x00);

    // Verify
    EXPECT_EQ(hoo_buffer_byte_at(buf, 0), 0xAB);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 1), 0xCD);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 2), 0xEF);

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SetByteInvalidIndex) {
    HooBuffer buf = hoo_buffer_new(0);
    EXPECT_EQ(hoo_buffer_set_byte(buf, 0, 0xFF), -1);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, AppendBytes) {
    HooBuffer buf = hoo_buffer_new(0);
    ASSERT_NE(buf, nullptr);

    const uint8_t data[] = {1, 2, 3, 4, 5};
    HooBuffer result = hoo_buffer_append(buf, data, 5);
    ASSERT_NE(result, nullptr);
    buf = result;

    EXPECT_EQ(hoo_buffer_length(buf), 5);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 0), 1);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 4), 5);

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, AppendMultiple) {
    HooBuffer buf = hoo_buffer_new(0);
    ASSERT_NE(buf, nullptr);

    buf = hoo_buffer_append(buf, (const uint8_t*)"ab", 2);
    ASSERT_NE(buf, nullptr);
    buf = hoo_buffer_append(buf, (const uint8_t*)"cd", 2);
    ASSERT_NE(buf, nullptr);

    EXPECT_EQ(hoo_buffer_length(buf), 4);
    EXPECT_EQ(hoo_buffer_byte_at(buf, 0), 'a');
    EXPECT_EQ(hoo_buffer_byte_at(buf, 3), 'd');

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, AppendBuffer) {
    HooBuffer buf1 = hoo_buffer_from_bytes((const uint8_t*)"Hello", 5);
    ASSERT_NE(buf1, nullptr);
    HooBuffer buf2 = hoo_buffer_from_bytes((const uint8_t*)" World", 6);
    ASSERT_NE(buf2, nullptr);

    HooBuffer result = hoo_buffer_append_buffer(buf1, buf2);
    ASSERT_NE(result, nullptr);
    buf1 = result;

    EXPECT_EQ(hoo_buffer_length(buf1), 11);
    const uint8_t* data = hoo_buffer_data(buf1);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(std::memcmp(data, "Hello World", 11), 0);

    hoo_buffer_release(buf1);
    hoo_buffer_release(buf2);
}

TEST_F(HooBufferTest, ClearBuffer) {
    HooBuffer buf = hoo_buffer_from_bytes((const uint8_t*)"data", 4);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), 4);

    hoo_buffer_clear(buf);
    EXPECT_EQ(hoo_buffer_length(buf), 0);

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SliceBuffer) {
    const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    HooBuffer buf = hoo_buffer_from_bytes(data, 10);
    ASSERT_NE(buf, nullptr);

    HooBuffer slice = hoo_buffer_slice(buf, 2, 5);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(hoo_buffer_length(slice), 3);
    EXPECT_EQ(hoo_buffer_byte_at(slice, 0), 2);
    EXPECT_EQ(hoo_buffer_byte_at(slice, 2), 4);

    hoo_buffer_release(slice);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SliceFull) {
    const uint8_t data[] = {1, 2, 3};
    HooBuffer buf = hoo_buffer_from_bytes(data, 3);
    ASSERT_NE(buf, nullptr);

    HooBuffer slice = hoo_buffer_slice(buf, 0, 3);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(hoo_buffer_length(slice), 3);

    hoo_buffer_release(slice);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SliceEmpty) {
    HooBuffer buf = hoo_buffer_from_bytes((const uint8_t*)"abc", 3);
    ASSERT_NE(buf, nullptr);

    HooBuffer slice = hoo_buffer_slice(buf, 1, 1);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(hoo_buffer_length(slice), 0);

    hoo_buffer_release(slice);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, SliceInvalidRange) {
    HooBuffer buf = hoo_buffer_from_bytes((const uint8_t*)"abc", 3);
    ASSERT_NE(buf, nullptr);

    EXPECT_EQ(hoo_buffer_slice(buf, 5, 10), nullptr);
    EXPECT_EQ(hoo_buffer_slice(buf, -1, 2), nullptr);
    EXPECT_EQ(hoo_buffer_slice(buf, 2, 1), nullptr);

    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, NullBufferOperations) {
    EXPECT_EQ(hoo_buffer_length(nullptr), 0);
    EXPECT_EQ(hoo_buffer_capacity(nullptr), 0);
    EXPECT_EQ(hoo_buffer_data(nullptr), nullptr);
    EXPECT_EQ(hoo_buffer_byte_at(nullptr, 0), -1);
    EXPECT_EQ(hoo_buffer_set_byte(nullptr, 0, 0), -1);
    EXPECT_EQ(hoo_buffer_append(nullptr, nullptr, 0), nullptr);
    EXPECT_EQ(hoo_buffer_append_buffer(nullptr, nullptr), nullptr);
    EXPECT_EQ(hoo_buffer_clear(nullptr), -1);
    EXPECT_EQ(hoo_buffer_copy(nullptr), nullptr);
    EXPECT_EQ(hoo_buffer_slice(nullptr, 0, 1), nullptr);
}

TEST_F(HooBufferTest, BufferTypeId) {
    HooBuffer buf = hoo_buffer_new(0);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_get_type_id(buf), 113);
    hoo_buffer_release(buf);
}

TEST_F(HooBufferTest, LargeBuffer) {
    int64_t size = 100000;
    HooBuffer buf = hoo_buffer_new(size);
    ASSERT_NE(buf, nullptr);

    uint8_t* raw = (uint8_t*)std::malloc(size);
    ASSERT_NE(raw, nullptr);
    for (int64_t i = 0; i < size; i++) {
        raw[i] = (uint8_t)(i & 0xFF);
    }

    buf = hoo_buffer_append(buf, raw, size);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(hoo_buffer_length(buf), size);

    for (int64_t i = 0; i < size; i += 1000) {
        EXPECT_EQ(hoo_buffer_byte_at(buf, i), (uint8_t)(i & 0xFF));
    }

    std::free(raw);
    hoo_buffer_release(buf);
}
