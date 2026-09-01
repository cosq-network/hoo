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

/// Parse a decimal string into mantissa without floating-point intermediate.
/// "19.99" with scale=2 -> mantissa=1999
/// "-0.001" with scale=3 -> mantissa=-1
/// "100" with scale=0 -> mantissa=100
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
    bool hasDecimalPoint = false;

    // Parse integer part
    while (*p >= '0' && *p <= '9') {
        integerPart = integerPart * 10 + (*p - '0');
        ++p;
    }

    // Parse decimal point and fractional part
    if (*p == '.') {
        hasDecimalPoint = true;
        ++p;
        while (*p >= '0' && *p <= '9' && fractionalDigits < scale) {
            fractionalPart = fractionalPart * 10 + (*p - '0');
            ++fractionalDigits;
            ++p;
        }
        // Skip remaining digits beyond scale
        while (*p >= '0' && *p <= '9') ++p;
    }

    // Combine: integerPart * 10^scale + fractionalPart
    int64_t mantissa = integerPart;
    for (int32_t i = 0; i < scale; ++i) {
        mantissa *= 10;
    }

    // Add fractional digits, padding with zeros if needed
    for (int32_t i = fractionalDigits; i < scale; ++i) {
        fractionalPart *= 10;
    }
    mantissa += fractionalPart;

    // Handle case where no decimal point but scale > 0
    // e.g., "100m" with scale=2 should be 10000 (representing 100.00)
    // This is already handled by the loop above.

    return negative ? -mantissa : mantissa;
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

    // Scale up numerator to get desired precision
    int32_t targetScale = std::max(da->scale, db->scale) + 8; // 8 extra digits
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
    return hoo_decimal_new(quotient, da->precision, targetScale);
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
    int64_t ma = da->mantissa;
    int64_t mb = db->mantissa;
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

    // Convert mantissa to string
    char buf[64];
    bool negative = m < 0;
    if (negative) m = -m;

    char* p = buf + sizeof(buf) - 1;
    *p = '\0';

    if (m == 0) {
        *(--p) = '0';
    } else {
        while (m > 0) {
            *(--p) = '0' + static_cast<char>(m % 10);
            m /= 10;
        }
    }

    if (negative) *(--p) = '-';

    // Insert decimal point
    size_t len = std::strlen(p);
    if (s > 0 && static_cast<size_t>(s) < len) {
        // Build result with decimal point
        size_t intLen = len - static_cast<size_t>(s);
        char* result = static_cast<char*>(std::malloc(len + 2)); // +1 for '.', +1 for '\0'
        std::memcpy(result, p, intLen);
        result[intLen] = '.';
        std::memcpy(result + intLen + 1, p + intLen, s);
        result[len + 1] = '\0';
        HooString str = hoo_string_from_cstr(result);
        std::free(result);
        return str;
    } else {
        return hoo_string_from_cstr(p);
    }
}
