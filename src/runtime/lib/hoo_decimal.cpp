#include "hoo_decimal.h"
#include <cstdlib>
#include <cstring>

struct HooDecimalImpl {
    int64_t mantissa;
    int32_t precision;
    int32_t scale;
};

extern "C" HooDecimal hoo_decimal_from_literal(const char* text, int32_t precision, int32_t scale) {
    auto* impl = new HooDecimalImpl();
    impl->mantissa = static_cast<int64_t>(std::atof(text) * std::pow(10, scale));
    impl->precision = precision;
    impl->scale = scale;
    return impl;
}