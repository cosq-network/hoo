#include "runtime/lib/compression/hoo_compression.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <zlib.h>

extern "C" {

struct CompressionHandle { int dummy; };

void* hoo_compression_new(void) {
    CompressionHandle* h = (CompressionHandle*)calloc(1, sizeof(CompressionHandle));
    return h;
}

void hoo_compression_release(void* comp) {
    std::free(comp);
}

int64_t hoo_compression_gzip_compress(const uint8_t* data, int64_t data_len,
                                       uint8_t** out_data, int64_t* out_len) {
    if (!data || data_len < 0 || !out_data || !out_len) return -1;
    *out_data = nullptr; *out_len = 0;

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (deflateInit2(&strm, 6, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return -1;

    uLong bound = deflateBound(&strm, (uLong)data_len);
    uint8_t* buf = (uint8_t*)std::malloc(bound);
    if (!buf) { deflateEnd(&strm); return -1; }

    strm.next_in = (Bytef*)data;
    strm.avail_in = (uInt)data_len;
    strm.next_out = buf;
    strm.avail_out = (uInt)bound;

    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) { std::free(buf); deflateEnd(&strm); return -1; }

    uLong compressed = strm.total_out;
    deflateEnd(&strm);

    uint8_t* result = (uint8_t*)std::malloc(compressed);
    if (!result) { std::free(buf); return -1; }
    std::memcpy(result, buf, compressed);
    std::free(buf);

    *out_data = result;
    *out_len = (int64_t)compressed;
    return 0;
}

int64_t hoo_compression_gzip_decompress(const uint8_t* data, int64_t data_len,
                                         uint8_t** out_data, int64_t* out_len) {
    if (!data || data_len < 0 || !out_data || !out_len) return -1;
    *out_data = nullptr; *out_len = 0;

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15 | 16) != Z_OK) return -1;

    strm.next_in = (Bytef*)data;
    strm.avail_in = (uInt)data_len;

    std::vector<uint8_t> buf;
    buf.reserve(65536);
    int ret;
    do {
        size_t old = buf.size();
        buf.resize(old + 65536);
        strm.next_out = buf.data() + old;
        strm.avail_out = 65536;
        ret = inflate(&strm, Z_NO_FLUSH);
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) { inflateEnd(&strm); return -1; }

    uLong decompressed = strm.total_out;
    inflateEnd(&strm);

    uint8_t* result = (uint8_t*)std::malloc(decompressed);
    if (!result) return -1;
    std::memcpy(result, buf.data(), decompressed);

    *out_data = result;
    *out_len = (int64_t)decompressed;
    return 0;
}

int64_t hoo_compression_deflate_compress(const uint8_t* data, int64_t data_len,
                                          uint8_t** out_data, int64_t* out_len) {
    if (!data || data_len < 0 || !out_data || !out_len) return -1;
    *out_data = nullptr; *out_len = 0;

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (deflateInit2(&strm, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return -1;

    uLong bound = deflateBound(&strm, (uLong)data_len);
    uint8_t* buf = (uint8_t*)std::malloc(bound);
    if (!buf) { deflateEnd(&strm); return -1; }

    strm.next_in = (Bytef*)data;
    strm.avail_in = (uInt)data_len;
    strm.next_out = buf;
    strm.avail_out = (uInt)bound;

    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) { std::free(buf); deflateEnd(&strm); return -1; }

    uLong compressed = strm.total_out;
    deflateEnd(&strm);

    uint8_t* result = (uint8_t*)std::malloc(compressed);
    if (!result) { std::free(buf); return -1; }
    std::memcpy(result, buf, compressed);
    std::free(buf);

    *out_data = result;
    *out_len = (int64_t)compressed;
    return 0;
}

int64_t hoo_compression_deflate_decompress(const uint8_t* data, int64_t data_len,
                                            uint8_t** out_data, int64_t* out_len) {
    if (!data || data_len < 0 || !out_data || !out_len) return -1;
    *out_data = nullptr; *out_len = 0;

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, -15) != Z_OK) return -1;

    strm.next_in = (Bytef*)data;
    strm.avail_in = (uInt)data_len;

    std::vector<uint8_t> buf;
    buf.reserve(65536);
    int ret;
    do {
        size_t old = buf.size();
        buf.resize(old + 65536);
        strm.next_out = buf.data() + old;
        strm.avail_out = 65536;
        ret = inflate(&strm, Z_NO_FLUSH);
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) { inflateEnd(&strm); return -1; }

    uLong decompressed = strm.total_out;
    inflateEnd(&strm);

    uint8_t* result = (uint8_t*)std::malloc(decompressed);
    if (!result) return -1;
    std::memcpy(result, buf.data(), decompressed);

    *out_data = result;
    *out_len = (int64_t)decompressed;
    return 0;
}

void hoo_compression_free_bytes(uint8_t* data) {
    std::free(data);
}

HooBuffer hoo_compression_gzip_compress_buffer(HooBuffer buf) {
    uint8_t* out_data = nullptr;
    int64_t out_len = 0;
    if (hoo_compression_gzip_compress(hoo_buffer_data(buf), hoo_buffer_length(buf), &out_data, &out_len) != 0)
        return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(out_data, out_len);
    std::free(out_data);
    return result;
}

HooBuffer hoo_compression_gzip_decompress_buffer(HooBuffer buf) {
    uint8_t* out_data = nullptr;
    int64_t out_len = 0;
    if (hoo_compression_gzip_decompress(hoo_buffer_data(buf), hoo_buffer_length(buf), &out_data, &out_len) != 0)
        return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(out_data, out_len);
    std::free(out_data);
    return result;
}

HooBuffer hoo_compression_deflate_compress_buffer(HooBuffer buf) {
    uint8_t* out_data = nullptr;
    int64_t out_len = 0;
    if (hoo_compression_deflate_compress(hoo_buffer_data(buf), hoo_buffer_length(buf), &out_data, &out_len) != 0)
        return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(out_data, out_len);
    std::free(out_data);
    return result;
}

HooBuffer hoo_compression_deflate_decompress_buffer(HooBuffer buf) {
    uint8_t* out_data = nullptr;
    int64_t out_len = 0;
    if (hoo_compression_deflate_decompress(hoo_buffer_data(buf), hoo_buffer_length(buf), &out_data, &out_len) != 0)
        return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(out_data, out_len);
    std::free(out_data);
    return result;
}

HooBuffer hoo_compression_gzip_compress_slice(HooByteSliceHandle slice) {
    HooByteSlice view = hoo_byte_slice_view(slice);
    uint8_t* data = nullptr;
    int64_t length = 0;
    if (hoo_compression_gzip_compress(view.data, view.length, &data, &length) != 0) return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(data, length);
    hoo_compression_free_bytes(data);
    return result;
}

HooBuffer hoo_compression_deflate_compress_slice(HooByteSliceHandle slice) {
    HooByteSlice view = hoo_byte_slice_view(slice);
    uint8_t* data = nullptr;
    int64_t length = 0;
    if (hoo_compression_deflate_compress(view.data, view.length, &data, &length) != 0) return nullptr;
    HooBuffer result = hoo_buffer_from_bytes(data, length);
    hoo_compression_free_bytes(data);
    return result;
}

} // extern "C"
