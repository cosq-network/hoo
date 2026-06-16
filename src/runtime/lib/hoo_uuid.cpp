#include "hoo_uuid.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

struct UUIDData {
    uint8_t bytes[16];
    int refcount;
};

static UUIDData* to_data(HooUUID uuid) {
    return static_cast<UUIDData*>(uuid);
}

static HooUUID from_data(UUIDData* data) {
    return static_cast<HooUUID>(data);
}

static void fill_random_bytes(uint8_t* buf, size_t len) {
#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)len, buf);
        CryptReleaseContext(hProv, 0);
        return;
    }
#else
    FILE* f = std::fopen("/dev/urandom", "rb");
    if (f) {
        size_t n = std::fread(buf, 1, len, f);
        std::fclose(f);
        if (n == len) return;
    }
#endif
    static bool seeded = false;
    if (!seeded) {
        std::srand((unsigned)std::time(nullptr) ^ (unsigned)std::clock());
        seeded = true;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(std::rand() & 0xFF);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

HooUUID hoo_uuid_v4(void) {
    UUIDData* data = (UUIDData*)std::malloc(sizeof(UUIDData));
    if (!data) return NULL;
    data->refcount = 1;
    fill_random_bytes(data->bytes, 16);
    data->bytes[6] = (data->bytes[6] & 0x0F) | 0x40;
    data->bytes[8] = (data->bytes[8] & 0x3F) | 0x80;
    return from_data(data);
}

HooUUID hoo_uuid_nil(void) {
    UUIDData* data = (UUIDData*)std::malloc(sizeof(UUIDData));
    if (!data) return NULL;
    data->refcount = 1;
    std::memset(data->bytes, 0, 16);
    return from_data(data);
}

HooUUID hoo_uuid_from_string(const char* str) {
    if (!str) return NULL;
    size_t slen = std::strlen(str);
    if (slen != 36) return NULL;
    static const char hex_chars[] = "0123456789abcdefABCDEF";
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (str[i] != '-') return NULL;
        } else if (!std::strchr(hex_chars, str[i])) {
            return NULL;
        }
    }
    UUIDData* data = (UUIDData*)std::malloc(sizeof(UUIDData));
    if (!data) return NULL;
    data->refcount = 1;
    unsigned int p[11];
    int n = std::sscanf(str, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &p[0], &p[1], &p[2], &p[3], &p[4],
        &p[5], &p[6], &p[7], &p[8], &p[9], &p[10]);
    if (n != 11) {
        std::free(data);
        return NULL;
    }
    data->bytes[0]  = (uint8_t)((p[0] >> 24) & 0xFF);
    data->bytes[1]  = (uint8_t)((p[0] >> 16) & 0xFF);
    data->bytes[2]  = (uint8_t)((p[0] >> 8) & 0xFF);
    data->bytes[3]  = (uint8_t)(p[0] & 0xFF);
    data->bytes[4]  = (uint8_t)((p[1] >> 8) & 0xFF);
    data->bytes[5]  = (uint8_t)(p[1] & 0xFF);
    data->bytes[6]  = (uint8_t)((p[2] >> 8) & 0xFF);
    data->bytes[7]  = (uint8_t)(p[2] & 0xFF);
    data->bytes[8]  = (uint8_t)(p[3] & 0xFF);
    data->bytes[9]  = (uint8_t)(p[4] & 0xFF);
    data->bytes[10] = (uint8_t)(p[5] & 0xFF);
    data->bytes[11] = (uint8_t)(p[6] & 0xFF);
    data->bytes[12] = (uint8_t)(p[7] & 0xFF);
    data->bytes[13] = (uint8_t)(p[8] & 0xFF);
    data->bytes[14] = (uint8_t)(p[9] & 0xFF);
    data->bytes[15] = (uint8_t)(p[10] & 0xFF);
    return from_data(data);
}

HooUUID hoo_uuid_from_bytes(const uint8_t* bytes) {
    if (!bytes) return NULL;
    UUIDData* data = (UUIDData*)std::malloc(sizeof(UUIDData));
    if (!data) return NULL;
    data->refcount = 1;
    std::memcpy(data->bytes, bytes, 16);
    return from_data(data);
}

char* hoo_uuid_to_string(HooUUID uuid) {
    if (!uuid) return NULL;
    UUIDData* data = to_data(uuid);
    char* result = (char*)std::malloc(37);
    if (!result) return NULL;
    std::snprintf(result, 37, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        (unsigned)(((uint32_t)data->bytes[0] << 24) | ((uint32_t)data->bytes[1] << 16) | ((uint32_t)data->bytes[2] << 8) | data->bytes[3]),
        (unsigned)(((uint32_t)data->bytes[4] << 8) | data->bytes[5]),
        (unsigned)(((uint32_t)data->bytes[6] << 8) | data->bytes[7]),
        data->bytes[8], data->bytes[9],
        data->bytes[10], data->bytes[11], data->bytes[12],
        data->bytes[13], data->bytes[14], data->bytes[15]);
    return result;
}

int64_t hoo_uuid_to_bytes(HooUUID uuid, uint8_t* out_16) {
    if (!uuid || !out_16) return 0;
    UUIDData* data = to_data(uuid);
    std::memcpy(out_16, data->bytes, 16);
    return 1;
}

int64_t hoo_uuid_is_nil(HooUUID uuid) {
    if (!uuid) return 0;
    UUIDData* data = to_data(uuid);
    for (int i = 0; i < 16; i++) {
        if (data->bytes[i] != 0) return 0;
    }
    return 1;
}

int64_t hoo_uuid_equals(HooUUID a, HooUUID b) {
    if (!a || !b) return (a == b) ? 1 : 0;
    UUIDData* da = to_data(a);
    UUIDData* db = to_data(b);
    return (std::memcmp(da->bytes, db->bytes, 16) == 0) ? 1 : 0;
}

int64_t hoo_uuid_compare(HooUUID a, HooUUID b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    UUIDData* da = to_data(a);
    UUIDData* db = to_data(b);
    int r = std::memcmp(da->bytes, db->bytes, 16);
    if (r < 0) return -1;
    if (r > 0) return 1;
    return 0;
}

HooUUID hoo_uuid_retain(HooUUID uuid) {
    if (!uuid) return NULL;
    UUIDData* data = to_data(uuid);
    data->refcount++;
    return uuid;
}

void hoo_uuid_release(HooUUID uuid) {
    if (!uuid) return;
    UUIDData* data = to_data(uuid);
    data->refcount--;
    if (data->refcount <= 0) {
        std::free(data);
    }
}

void hoo_uuid_free_string(char* str) {
    std::free(str);
}

#ifdef __cplusplus
}
#endif
