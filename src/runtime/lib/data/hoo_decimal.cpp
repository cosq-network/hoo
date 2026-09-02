#include "runtime/lib/data/hoo_decimal.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "runtime/lib/core/hoo_exception.h"

// ============================================================================
// Internal representation
// ============================================================================

struct HooDecimalImpl {
    int64_t mantissa;      // Significand * 10^scale
    int32_t precision;     // Total number of digits
    int32_t scale;         // Number of digits after decimal point
};

// ARC header is at offset -16 from the returned pointer:
//   [-16] = refcount (int64_t)
//   [-8]  = typeid  (int64_t)
//   [0..] = HooDecimalImpl data

static constexpr int64_t DECIMAL_TYPE_ID = 125;

// ============================================================================
// Internal helpers
// ============================================================================

static HooDecimalImpl* toImpl(HooDecimal d) {
    return static_cast<HooDecimalImpl*>(d);
}

// Exceptions
// ============================================================================

[[noreturn]] static void throwDecimalOverflow() {
    HooException exc = hoo_exception_create(HOO_EXCEPTION_DECIMAL_OVERFLOW, "Decimal arithmetic overflow");
    hoo_exception_throw(exc);
}

[[noreturn]] static void throwDecimalDivZero() {
    HooException exc = hoo_exception_create(HOO_EXCEPTION_DECIMAL_DIV_ZERO, "Decimal division by zero");
    hoo_exception_throw(exc);
}

[[noreturn]] static void throwDecimalModZero() {
    HooException exc = hoo_exception_create(HOO_EXCEPTION_DECIMAL_MOD_ZERO, "Decimal modulo by zero");
    hoo_exception_throw(exc);
}

// ============================================================================
// Overflow guards
// ============================================================================

static bool addWouldOverflow(int64_t a, int64_t b) {
    if (b > 0 && a > INT64_MAX - b) return true;
    if (b < 0 && a < INT64_MIN - b) return true;
    return false;
}

static bool subWouldOverflow(int64_t a, int64_t b) {
    if (b > 0 && a < INT64_MIN + b) return true;
    if (b < 0 && a > INT64_MAX + b) return true;
    return false;
}

static bool mulWouldOverflow(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return false;
    if (a == -1 && b == INT64_MIN) return true;
    if (b == -1 && a == INT64_MIN) return true;
    if (a > 0 && b > 0 && a > INT64_MAX / b) return true;
    if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
    if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
    if (a < 0 && b < 0 && a < INT64_MAX / b) return true;
    return false;
}

/// Parse a decimal string into mantissa without floating-point intermediate.
/// "19.99" with scale=2 -> mantissa=1999
/// "-0.001" with scale=3 -> mantissa=-1
/// "100" with scale=0 -> mantissa=100
/// Throws a DecimalOverflow exception if the scaled mantissa does not fit
/// in int64 (previously this silently overflowed, producing undefined
/// behavior).
static int64_t parseDecimalMantissa(const char* text, int32_t scale) {
    if (!text || !*text) return 0;

    const char* p = text;
    bool negative = false;

    // Skip leading whitespace
    while (*p == ' ' || *p == '\t') ++p;

    if (*p == '-') { negative = true; ++p; }
    else if (*p == '+') { ++p; }

    int64_t integerPart = 0;
    int64_t fractionalPart = 0;
    int fractionalDigits = 0;

    // Parse integer part
    while (*p >= '0' && *p <= '9') {
        int64_t d = *p - '0';
        if (mulWouldOverflow(integerPart, 10) || addWouldOverflow(integerPart * 10, d)) {
            throwDecimalOverflow();
        }
        integerPart = integerPart * 10 + d;
        ++p;
    }

    // Parse decimal point and fractional part
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9' && fractionalDigits < scale) {
            int64_t d = *p - '0';
            if (mulWouldOverflow(fractionalPart, 10) || addWouldOverflow(fractionalPart * 10, d)) {
                throwDecimalOverflow();
            }
            fractionalPart = fractionalPart * 10 + d;
            ++fractionalDigits;
            ++p;
        }
        // Skip remaining digits beyond scale
        while (*p >= '0' && *p <= '9') ++p;
    }

    // Combine: integerPart * 10^scale + fractionalPart
    int64_t mantissa = integerPart;
    for (int32_t i = 0; i < scale; ++i) {
        if (mulWouldOverflow(mantissa, 10)) throwDecimalOverflow();
        mantissa *= 10;
    }

    // Add fractional digits, padding with zeros if needed
    for (int32_t i = fractionalDigits; i < scale; ++i) {
        if (mulWouldOverflow(fractionalPart, 10)) throwDecimalOverflow();
        fractionalPart *= 10;
    }
    if (addWouldOverflow(mantissa, fractionalPart)) throwDecimalOverflow();
    mantissa += fractionalPart;

    // negative is only applied to a non-negative magnitude, so -mantissa
    // cannot overflow.
    return negative ? -mantissa : mantissa;
}

static int32_t countDigits(int64_t m) {
    if (m == 0) return 1;
    if (m < 0) {
        if (m == INT64_MIN) return 19; // 9223372036854775808
        m = -m;
    }
    int32_t count = 0;
    while (m > 0) {
        m /= 10;
        ++count;
    }
    return count;
}

static bool fitsPrecision(int64_t m, int32_t prec) {
    return countDigits(m) <= prec;
}

/// Normalize: ensure the mantissa fits within precision digits.
static void normalize(HooDecimalImpl* impl) {
    int64_t m = impl->mantissa;
    int32_t s = impl->scale;

    // Remove trailing zeros from mantissa (reduce scale)
    while (s > 0 && m % 10 == 0) {
        m /= 10;
        --s;
    }

    if (!fitsPrecision(m, impl->precision)) {
        throwDecimalOverflow();
    }

    impl->mantissa = m;
    impl->scale = s;
}

// ============================================================================
// Lifecycle
// ============================================================================

extern "C" HooDecimal hoo_decimal_from_literal(const char* text, int32_t precision, int32_t scale) {
    int64_t mantissa = parseDecimalMantissa(text, scale);
    return hoo_decimal_new(mantissa, precision, scale);
}

extern "C" HooDecimal hoo_decimal_new(int64_t mantissa, int32_t precision, int32_t scale) {
    HooDecimalImpl* impl = static_cast<HooDecimalImpl*>(hoo_alloc(sizeof(HooDecimalImpl), DECIMAL_TYPE_ID));
    impl->mantissa = mantissa;
    impl->precision = precision;
    impl->scale = scale;
    normalize(impl);
    return impl;
}

extern "C" void hoo_decimal_release(HooDecimal d) {
    hoo_release(d);
}

extern "C" HooDecimal hoo_decimal_retain(HooDecimal d) {
    return hoo_retain(d);
}

// ============================================================================
// Accessors
// ============================================================================

extern "C" int64_t hoo_decimal_mantissa(HooDecimal d) {
    if (!d) return 0;
    return toImpl(d)->mantissa;
}

extern "C" int32_t hoo_decimal_precision(HooDecimal d) {
    if (!d) return 0;
    return toImpl(d)->precision;
}

extern "C" int32_t hoo_decimal_scale(HooDecimal d) {
    if (!d) return 0;
    return toImpl(d)->scale;
}

// ============================================================================
// Arithmetic
// ============================================================================

extern "C" HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b) {
    if (!a || !b) return nullptr;
    const auto* da = toImpl(a);
    const auto* db = toImpl(b);

    // Align scales: multiply the one with smaller scale
    int64_t ma = da->mantissa;
    int64_t mb = db->mantissa;
    int32_t sa = da->scale;
    int32_t sb = db->scale;

    while (sa < sb) { 
        if (mulWouldOverflow(ma, 10)) throwDecimalOverflow();
        ma *= 10; 
        ++sa; 
    }
    while (sb < sa) { 
        if (mulWouldOverflow(mb, 10)) throwDecimalOverflow();
        mb *= 10; 
        ++sb; 
    }

    if (addWouldOverflow(ma, mb)) throwDecimalOverflow();
    int64_t result = ma + mb;
    int32_t prec = std::max(da->precision, db->precision);
    return hoo_decimal_new(result, prec, sa);
}

extern "C" HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b) {
    if (!a || !b) return nullptr;
    const auto* da = toImpl(a);
    const auto* db = toImpl(b);

    int64_t ma = da->mantissa;
    int64_t mb = db->mantissa;
    int32_t sa = da->scale;
    int32_t sb = db->scale;

    while (sa < sb) { 
        if (mulWouldOverflow(ma, 10)) throwDecimalOverflow();
        ma *= 10; 
        ++sa; 
    }
    while (sb < sa) { 
        if (mulWouldOverflow(mb, 10)) throwDecimalOverflow();
        mb *= 10; 
        ++sb; 
    }

    if (subWouldOverflow(ma, mb)) throwDecimalOverflow();
    int64_t result = ma - mb;
    int32_t prec = std::max(da->precision, db->precision);
    return hoo_decimal_new(result, prec, sa);
}

extern "C" HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b) {
    if (!a || !b) return nullptr;
    const auto* da = toImpl(a);
    const auto* db = toImpl(b);

    if (mulWouldOverflow(da->mantissa, db->mantissa)) throwDecimalOverflow();
    int64_t result = da->mantissa * db->mantissa;
    int32_t resultScale = da->scale + db->scale;
    int32_t prec = da->precision + db->precision;
    return hoo_decimal_new(result, prec, resultScale);
}

extern "C" HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b) {
    if (!a || !b) return nullptr;
    const auto* da = toImpl(a);
    const auto* db = toImpl(b);

    if (db->mantissa == 0) throwDecimalDivZero();

    // Scale up numerator to get desired precision. Clamp the target scale to
    // the result precision so low-precision decimals (e.g. Decimal<8,2>) do
    // not throw an overflow for results that fit their precision.
    int32_t resultPrecision = std::max(da->precision, db->precision);
    int32_t targetScale = std::max(da->scale, db->scale) + 8; // 8 extra digits
    if (targetScale > resultPrecision) targetScale = resultPrecision;
    int64_t numerator = da->mantissa;
    for (int32_t i = 0; i < targetScale - da->scale; ++i) {
        if (mulWouldOverflow(numerator, 10)) throwDecimalOverflow();
        numerator *= 10;
    }

    int64_t denominator = db->mantissa;
    for (int32_t i = 0; i < db->scale; ++i) {
        if (mulWouldOverflow(denominator, 10)) throwDecimalOverflow();
        denominator *= 10;
    }

    if (denominator == -1 && numerator == INT64_MIN) throwDecimalOverflow();
    int64_t quotient = numerator / denominator;
    return hoo_decimal_new(quotient, resultPrecision, targetScale);
}

extern "C" HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b) {
    if (!a || !b) return nullptr;
    const auto* da = toImpl(a);
    const auto* db = toImpl(b);

    if (db->mantissa == 0) throwDecimalModZero();

    // Align scales
    int64_t ma = da->mantissa;
    int64_t mb = db->mantissa;
    int32_t sa = da->scale;
    int32_t sb = db->scale;

    while (sa < sb) { 
        if (mulWouldOverflow(ma, 10)) throwDecimalOverflow();
        ma *= 10; 
        ++sa; 
    }
    while (sb < sa) { 
        if (mulWouldOverflow(mb, 10)) throwDecimalOverflow();
        mb *= 10; 
        ++sb; 
    }

    if (mb == -1 && ma == INT64_MIN) throwDecimalOverflow();
    int64_t result = ma % mb;
    return hoo_decimal_new(result, da->precision, sa);
}

extern "C" HooDecimal hoo_decimal_neg(HooDecimal d) {
    if (!d) return nullptr;
    const auto* impl = toImpl(d);
    if (impl->mantissa == INT64_MIN) throwDecimalOverflow();
    return hoo_decimal_new(-impl->mantissa, impl->precision, impl->scale);
}

// ============================================================================
// Comparison
// ============================================================================

static int compareAligned(const HooDecimalImpl* da, const HooDecimalImpl* db) {
    // Use a 128-bit intermediate so scale alignment never silently overflows
    // the mantissa (previously this was int64 UB near INT64_MAX/INT64_MIN).
#if defined(__SIZEOF_INT128__)
    __int128 ma = da->mantissa;
    __int128 mb = db->mantissa;
#else
    int64_t ma = da->mantissa;
    int64_t mb = db->mantissa;
#endif
    int32_t sa = da->scale;
    int32_t sb = db->scale;

    while (sa < sb) { ma *= 10; ++sa; }
    while (sb < sa) { mb *= 10; ++sb; }

    if (ma < mb) return -1;
    if (ma > mb) return 1;
    return 0;
}

extern "C" int64_t hoo_decimal_eq(HooDecimal a, HooDecimal b) {
    if (!a || !b) return (!a && !b) ? 1 : 0;
    return compareAligned(toImpl(a), toImpl(b)) == 0 ? 1 : 0;
}

extern "C" int64_t hoo_decimal_ne(HooDecimal a, HooDecimal b) {
    if (!a || !b) return (!a && !b) ? 0 : 1;
    return compareAligned(toImpl(a), toImpl(b)) != 0 ? 1 : 0;
}

extern "C" int64_t hoo_decimal_lt(HooDecimal a, HooDecimal b) {
    if (!a || !b) return 0;
    return compareAligned(toImpl(a), toImpl(b)) < 0 ? 1 : 0;
}

extern "C" int64_t hoo_decimal_le(HooDecimal a, HooDecimal b) {
    if (!a || !b) return 0;
    return compareAligned(toImpl(a), toImpl(b)) <= 0 ? 1 : 0;
}

extern "C" int64_t hoo_decimal_gt(HooDecimal a, HooDecimal b) {
    if (!a || !b) return 0;
    return compareAligned(toImpl(a), toImpl(b)) > 0 ? 1 : 0;
}

extern "C" int64_t hoo_decimal_ge(HooDecimal a, HooDecimal b) {
    if (!a || !b) return 0;
    return compareAligned(toImpl(a), toImpl(b)) >= 0 ? 1 : 0;
}

// ============================================================================
// String representation
// ============================================================================

extern "C" HooString hoo_decimal_to_string(HooDecimal d) {
    if (!d) {
        return hoo_string_from_cstr("");
    }
    const auto* impl = toImpl(d);
    int64_t m = impl->mantissa;
    int32_t s = impl->scale;

    // Convert mantissa to string. Use an unsigned magnitude so negating
    // INT64_MIN (which is UB in int64) is handled correctly.
    char buf[96];
    uint64_t mag;
    bool negative = m < 0;
    if (negative) {
        mag = static_cast<uint64_t>(-(m + 1)) + 1;
    } else {
        mag = static_cast<uint64_t>(m);
    }

    char digits[24];
    size_t len = 0;
    if (mag == 0) {
        digits[len++] = '0';
    } else {
        while (mag > 0) {
            digits[len++] = '0' + static_cast<char>(mag % 10);
            mag /= 10;
        }
        for (size_t i = 0; i < len / 2; ++i) {
            char t = digits[i];
            digits[i] = digits[len - 1 - i];
            digits[len - 1 - i] = t;
        }
    }

    size_t idx = 0;
    if (negative) buf[idx++] = '-';
    if (s <= 0) {
        std::memcpy(buf + idx, digits, len);
        idx += len;
    } else if (static_cast<int64_t>(len) > s) {
        size_t intLen = len - static_cast<size_t>(s);
        std::memcpy(buf + idx, digits, intLen);
        idx += intLen;
        buf[idx++] = '.';
        std::memcpy(buf + idx, digits + intLen, static_cast<size_t>(s));
        idx += static_cast<size_t>(s);
    } else {
        buf[idx++] = '0';
        buf[idx++] = '.';
        size_t zeros = static_cast<size_t>(s) - len;
        for (size_t i = 0; i < zeros; ++i) buf[idx++] = '0';
        std::memcpy(buf + idx, digits, len);
        idx += len;
    }
    buf[idx] = '\0';
    return hoo_string_from_cstr(buf);
}
