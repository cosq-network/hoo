#include "../lib/hoo_math.h"
#include "RuntimeRegistry.h"
#include "RuntimeFunctionStorage.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <iostream>

using namespace llvm;
using namespace llvm::orc;
using namespace hooc::runtime;

// ============================================================================
// JIT Symbol Registration Callback
// ============================================================================

void hoo_math_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    #define REGISTER_MATH_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_math_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_math_##name), \
                JITSymbolFlags::Exported \
            );

    // Constants
    REGISTER_MATH_FUNC(get_pi)
    REGISTER_MATH_FUNC(get_e)
    REGISTER_MATH_FUNC(get_tau)
    REGISTER_MATH_FUNC(get_inf)
    REGISTER_MATH_FUNC(get_neg_inf)
    REGISTER_MATH_FUNC(get_nan)

    // Register overload aliases for common functions
    REGISTER_MATH_FUNC(abs_int64)
    REGISTER_MATH_FUNC(abs_double)
    REGISTER_MATH_FUNC(min_int64)
    REGISTER_MATH_FUNC(min_double)
    REGISTER_MATH_FUNC(max_int64)
    REGISTER_MATH_FUNC(max_double)
    REGISTER_MATH_FUNC(clamp)
    REGISTER_MATH_FUNC(sign_int64)
    REGISTER_MATH_FUNC(sign_double)

    // Register bare function names as aliases (e.g., sqrt -> hoo_math_sqrt)
    symbols[jit.mangleAndIntern("sqrt")] = symbols[jit.mangleAndIntern("hoo_math_sqrt")];
    symbols[jit.mangleAndIntern("sin")] = symbols[jit.mangleAndIntern("hoo_math_sin")];
    symbols[jit.mangleAndIntern("cos")] = symbols[jit.mangleAndIntern("hoo_math_cos")];
    symbols[jit.mangleAndIntern("tan")] = symbols[jit.mangleAndIntern("hoo_math_tan")];
    symbols[jit.mangleAndIntern("asin")] = symbols[jit.mangleAndIntern("hoo_math_asin")];
    symbols[jit.mangleAndIntern("acos")] = symbols[jit.mangleAndIntern("hoo_math_acos")];
    symbols[jit.mangleAndIntern("atan")] = symbols[jit.mangleAndIntern("hoo_math_atan")];
    symbols[jit.mangleAndIntern("atan2")] = symbols[jit.mangleAndIntern("hoo_math_atan2")];
    symbols[jit.mangleAndIntern("sinh")] = symbols[jit.mangleAndIntern("hoo_math_sinh")];
    symbols[jit.mangleAndIntern("cosh")] = symbols[jit.mangleAndIntern("hoo_math_cosh")];
    symbols[jit.mangleAndIntern("tanh")] = symbols[jit.mangleAndIntern("hoo_math_tanh")];
    symbols[jit.mangleAndIntern("exp")] = symbols[jit.mangleAndIntern("hoo_math_exp")];
    symbols[jit.mangleAndIntern("exp2")] = symbols[jit.mangleAndIntern("hoo_math_exp2")];
    symbols[jit.mangleAndIntern("expm1")] = symbols[jit.mangleAndIntern("hoo_math_expm1")];
    symbols[jit.mangleAndIntern("log")] = symbols[jit.mangleAndIntern("hoo_math_log")];
    symbols[jit.mangleAndIntern("log10")] = symbols[jit.mangleAndIntern("hoo_math_log10")];
    symbols[jit.mangleAndIntern("log2")] = symbols[jit.mangleAndIntern("hoo_math_log2")];
    symbols[jit.mangleAndIntern("log1p")] = symbols[jit.mangleAndIntern("hoo_math_log1p")];
    symbols[jit.mangleAndIntern("floor")] = symbols[jit.mangleAndIntern("hoo_math_floor")];
    symbols[jit.mangleAndIntern("ceil")] = symbols[jit.mangleAndIntern("hoo_math_ceil")];
    symbols[jit.mangleAndIntern("round")] = symbols[jit.mangleAndIntern("hoo_math_round")];
    symbols[jit.mangleAndIntern("trunc")] = symbols[jit.mangleAndIntern("hoo_math_trunc")];
    symbols[jit.mangleAndIntern("fract")] = symbols[jit.mangleAndIntern("hoo_math_fract")];
    symbols[jit.mangleAndIntern("pow")] = symbols[jit.mangleAndIntern("hoo_math_pow")];
    symbols[jit.mangleAndIntern("cbrt")] = symbols[jit.mangleAndIntern("hoo_math_cbrt")];
    symbols[jit.mangleAndIntern("hypot")] = symbols[jit.mangleAndIntern("hoo_math_hypot")];
    symbols[jit.mangleAndIntern("abs")] = symbols[jit.mangleAndIntern("hoo_math_abs_double")];
    symbols[jit.mangleAndIntern("min")] = symbols[jit.mangleAndIntern("hoo_math_min_double")];
    symbols[jit.mangleAndIntern("max")] = symbols[jit.mangleAndIntern("hoo_math_max_double")];
    symbols[jit.mangleAndIntern("sign")] = symbols[jit.mangleAndIntern("hoo_math_sign_double")];
    symbols[jit.mangleAndIntern("clamp")] = symbols[jit.mangleAndIntern("hoo_math_clamp")];
    symbols[jit.mangleAndIntern("isEven")] = symbols[jit.mangleAndIntern("hoo_math_is_even")];
    symbols[jit.mangleAndIntern("isOdd")] = symbols[jit.mangleAndIntern("hoo_math_is_odd")];
    symbols[jit.mangleAndIntern("isPrime")] = symbols[jit.mangleAndIntern("hoo_math_is_prime")];
    symbols[jit.mangleAndIntern("gcd")] = symbols[jit.mangleAndIntern("hoo_math_gcd")];
    symbols[jit.mangleAndIntern("lcm")] = symbols[jit.mangleAndIntern("hoo_math_lcm")];
    symbols[jit.mangleAndIntern("factorial")] = symbols[jit.mangleAndIntern("hoo_math_factorial")];
    symbols[jit.mangleAndIntern("fibonacci")] = symbols[jit.mangleAndIntern("hoo_math_fibonacci")];
    symbols[jit.mangleAndIntern("PI")] = symbols[jit.mangleAndIntern("hoo_math_get_pi")];
    symbols[jit.mangleAndIntern("E")] = symbols[jit.mangleAndIntern("hoo_math_get_e")];
    symbols[jit.mangleAndIntern("TAU")] = symbols[jit.mangleAndIntern("hoo_math_get_tau")];
    symbols[jit.mangleAndIntern("INF")] = symbols[jit.mangleAndIntern("hoo_math_get_inf")];
    symbols[jit.mangleAndIntern("NEG_INF")] = symbols[jit.mangleAndIntern("hoo_math_get_neg_inf")];
    symbols[jit.mangleAndIntern("NAN")] = symbols[jit.mangleAndIntern("hoo_math_get_nan")];
    symbols[jit.mangleAndIntern("Random")] = symbols[jit.mangleAndIntern("hoo_math_random_new")];

    // Power and roots
    REGISTER_MATH_FUNC(pow)
    REGISTER_MATH_FUNC(sqrt)
    REGISTER_MATH_FUNC(cbrt)
    REGISTER_MATH_FUNC(hypot)

    // Trigonometric
    REGISTER_MATH_FUNC(sin)
    REGISTER_MATH_FUNC(cos)
    REGISTER_MATH_FUNC(tan)
    REGISTER_MATH_FUNC(asin)
    REGISTER_MATH_FUNC(acos)
    REGISTER_MATH_FUNC(atan)
    REGISTER_MATH_FUNC(atan2)
    REGISTER_MATH_FUNC(sinh)
    REGISTER_MATH_FUNC(cosh)
    REGISTER_MATH_FUNC(tanh)

    // Exponential and logarithmic
    REGISTER_MATH_FUNC(exp)
    REGISTER_MATH_FUNC(exp2)
    REGISTER_MATH_FUNC(expm1)
    REGISTER_MATH_FUNC(log)
    REGISTER_MATH_FUNC(log10)
    REGISTER_MATH_FUNC(log2)
    REGISTER_MATH_FUNC(log1p)

    // Rounding
    REGISTER_MATH_FUNC(floor)
    REGISTER_MATH_FUNC(ceil)
    REGISTER_MATH_FUNC(round)
    REGISTER_MATH_FUNC(trunc)
    REGISTER_MATH_FUNC(fract)

    // Random
    REGISTER_MATH_FUNC(random_new)
    REGISTER_MATH_FUNC(random_new_with_seed)
    REGISTER_MATH_FUNC(random_next_int)
    REGISTER_MATH_FUNC(random_next_int_max)
    REGISTER_MATH_FUNC(random_next_double)
    REGISTER_MATH_FUNC(random_next_bool)
    REGISTER_MATH_FUNC(random_next_bytes)
    REGISTER_MATH_FUNC(random_retain)
    REGISTER_MATH_FUNC(random_release)

    // Number utilities
    REGISTER_MATH_FUNC(is_even)
    REGISTER_MATH_FUNC(is_odd)
    REGISTER_MATH_FUNC(is_prime)
    REGISTER_MATH_FUNC(gcd)
    REGISTER_MATH_FUNC(lcm)
    REGISTER_MATH_FUNC(factorial)
    REGISTER_MATH_FUNC(fibonacci)

    #undef REGISTER_MATH_FUNC

    // Also register as hoo.math module functions
    SymbolMap hooSymbols;
    hooSymbols[jit.mangleAndIntern("hoo.math.PI")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_pi), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.math.E")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_e), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.math.TAU")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_tau), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.math.INF")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_inf), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.math.NEG_INF")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_neg_inf), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.math.NAN")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_math_get_nan), JITSymbolFlags::Exported);

    auto err1 = mainDylib.define(absoluteSymbols(symbols));
    if (err1) {
        llvm::errs() << "ERROR: Failed to register Math functions with JIT: "
                     << toString(std::move(err1)) << "\n";
        exit(1);
    }

    auto err2 = mainDylib.define(absoluteSymbols(hooSymbols));
    if (err2) {
        llvm::errs() << "ERROR: Failed to register hoo.math.* functions with JIT: "
                     << toString(std::move(err2)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

void hoo_math_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    auto ptrTy = PointerType::get(context, 0);
    auto i64Ty = Type::getInt64Ty(context);
    auto i8Ty = Type::getInt8Ty(context);
    auto doubleTy = Type::getDoubleTy(context);
    auto voidTy = Type::getVoidTy(context);

    #define DECLARE_MATH_FN(name, returnType, ...) \
        do { \
            std::vector<Type*> params = {__VA_ARGS__}; \
            auto funcType = FunctionType::get(returnType, params, false); \
            Function::Create(funcType, Function::ExternalLinkage, "hoo_math_" #name, &module); \
        } while(0);

    // Constants
    DECLARE_MATH_FN(get_pi, doubleTy)
    DECLARE_MATH_FN(get_e, doubleTy)
    DECLARE_MATH_FN(get_tau, doubleTy)
    DECLARE_MATH_FN(get_inf, doubleTy)
    DECLARE_MATH_FN(get_neg_inf, doubleTy)
    DECLARE_MATH_FN(get_nan, doubleTy)

    // Basic functions
    DECLARE_MATH_FN(abs_int64, i64Ty, i64Ty)
    DECLARE_MATH_FN(abs_double, doubleTy, doubleTy)
    DECLARE_MATH_FN(min_int64, i64Ty, i64Ty, i64Ty)
    DECLARE_MATH_FN(min_double, doubleTy, doubleTy, doubleTy)
    DECLARE_MATH_FN(max_int64, i64Ty, i64Ty, i64Ty)
    DECLARE_MATH_FN(max_double, doubleTy, doubleTy, doubleTy)
    DECLARE_MATH_FN(clamp, doubleTy, doubleTy, doubleTy, doubleTy)
    DECLARE_MATH_FN(sign_int64, i64Ty, i64Ty)
    DECLARE_MATH_FN(sign_double, doubleTy, doubleTy)

    // Power and roots
    DECLARE_MATH_FN(pow, doubleTy, doubleTy, doubleTy)
    DECLARE_MATH_FN(sqrt, doubleTy, doubleTy)
    DECLARE_MATH_FN(cbrt, doubleTy, doubleTy)
    DECLARE_MATH_FN(hypot, doubleTy, doubleTy, doubleTy)

    // Declare bare function names as aliases
    {
        auto fty = FunctionType::get(doubleTy, {doubleTy}, false);
        Function::Create(fty, Function::ExternalLinkage, "sqrt", &module);
        Function::Create(fty, Function::ExternalLinkage, "sin", &module);
        Function::Create(fty, Function::ExternalLinkage, "cos", &module);
        Function::Create(fty, Function::ExternalLinkage, "tan", &module);
        Function::Create(fty, Function::ExternalLinkage, "asin", &module);
        Function::Create(fty, Function::ExternalLinkage, "acos", &module);
        Function::Create(fty, Function::ExternalLinkage, "atan", &module);
        Function::Create(fty, Function::ExternalLinkage, "sinh", &module);
        Function::Create(fty, Function::ExternalLinkage, "cosh", &module);
        Function::Create(fty, Function::ExternalLinkage, "tanh", &module);
        Function::Create(fty, Function::ExternalLinkage, "exp", &module);
        Function::Create(fty, Function::ExternalLinkage, "exp2", &module);
        Function::Create(fty, Function::ExternalLinkage, "expm1", &module);
        Function::Create(fty, Function::ExternalLinkage, "log", &module);
        Function::Create(fty, Function::ExternalLinkage, "log10", &module);
        Function::Create(fty, Function::ExternalLinkage, "log2", &module);
        Function::Create(fty, Function::ExternalLinkage, "log1p", &module);
        Function::Create(fty, Function::ExternalLinkage, "floor", &module);
        Function::Create(fty, Function::ExternalLinkage, "ceil", &module);
        Function::Create(fty, Function::ExternalLinkage, "round", &module);
        Function::Create(fty, Function::ExternalLinkage, "trunc", &module);
        Function::Create(fty, Function::ExternalLinkage, "fract", &module);
    }

    // Trigonometric
    DECLARE_MATH_FN(sin, doubleTy, doubleTy)
    DECLARE_MATH_FN(cos, doubleTy, doubleTy)
    DECLARE_MATH_FN(tan, doubleTy, doubleTy)
    DECLARE_MATH_FN(asin, doubleTy, doubleTy)
    DECLARE_MATH_FN(acos, doubleTy, doubleTy)
    DECLARE_MATH_FN(atan, doubleTy, doubleTy)
    DECLARE_MATH_FN(atan2, doubleTy, doubleTy, doubleTy)
    DECLARE_MATH_FN(sinh, doubleTy, doubleTy)
    DECLARE_MATH_FN(cosh, doubleTy, doubleTy)
    DECLARE_MATH_FN(tanh, doubleTy, doubleTy)

    // Exponential and logarithmic
    DECLARE_MATH_FN(exp, doubleTy, doubleTy)
    DECLARE_MATH_FN(exp2, doubleTy, doubleTy)
    DECLARE_MATH_FN(expm1, doubleTy, doubleTy)
    DECLARE_MATH_FN(log, doubleTy, doubleTy)
    DECLARE_MATH_FN(log10, doubleTy, doubleTy)
    DECLARE_MATH_FN(log2, doubleTy, doubleTy)
    DECLARE_MATH_FN(log1p, doubleTy, doubleTy)

    // Rounding
    DECLARE_MATH_FN(floor, doubleTy, doubleTy)
    DECLARE_MATH_FN(ceil, doubleTy, doubleTy)
    DECLARE_MATH_FN(round, doubleTy, doubleTy)
    DECLARE_MATH_FN(trunc, doubleTy, doubleTy)
    DECLARE_MATH_FN(fract, doubleTy, doubleTy)

    // Declare more bare name aliases for int64 and double overloads
    // Note: Avoiding bare 'min'/'max' aliases to prevent conflicts with user-defined functions
    {
        auto i64fty = FunctionType::get(i64Ty, {i64Ty}, false);
        Function::Create(i64fty, Function::ExternalLinkage, "abs", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "sign", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "isEven", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "isOdd", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "isPrime", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "gcd", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "lcm", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "factorial", &module);
        Function::Create(i64fty, Function::ExternalLinkage, "fibonacci", &module);

        auto doublefty = FunctionType::get(doubleTy, {doubleTy}, false);
        Function::Create(doublefty, Function::ExternalLinkage, "cbrt", &module);
        Function::Create(doublefty, Function::ExternalLinkage, "hypot", &module);
        auto double3 = FunctionType::get(doubleTy, {doubleTy, doubleTy}, false);
        Function::Create(double3, Function::ExternalLinkage, "pow", &module);
        Function::Create(double3, Function::ExternalLinkage, "atan2", &module);
        Function::Create(double3, Function::ExternalLinkage, "fmod", &module);

        auto clampfty = FunctionType::get(doubleTy, {doubleTy, doubleTy, doubleTy}, false);
        Function::Create(clampfty, Function::ExternalLinkage, "clamp", &module);
    }

    // Random
    DECLARE_MATH_FN(random_new, ptrTy)
    DECLARE_MATH_FN(random_new_with_seed, ptrTy, i64Ty)
    DECLARE_MATH_FN(random_next_int, i64Ty, ptrTy)
    DECLARE_MATH_FN(random_next_int_max, i64Ty, ptrTy, i64Ty)
    DECLARE_MATH_FN(random_next_double, doubleTy, ptrTy)
    DECLARE_MATH_FN(random_next_bool, i64Ty, ptrTy)
    DECLARE_MATH_FN(random_next_bytes, i64Ty, ptrTy, ptrTy, i64Ty)
    DECLARE_MATH_FN(random_retain, ptrTy, ptrTy)
    DECLARE_MATH_FN(random_release, voidTy, ptrTy)

    // Number utilities
    DECLARE_MATH_FN(is_even, i64Ty, i64Ty)
    DECLARE_MATH_FN(is_odd, i64Ty, i64Ty)
    DECLARE_MATH_FN(is_prime, i64Ty, i64Ty)
    DECLARE_MATH_FN(gcd, i64Ty, i64Ty, i64Ty)
    DECLARE_MATH_FN(lcm, i64Ty, i64Ty, i64Ty)
    DECLARE_MATH_FN(factorial, i64Ty, i64Ty)
    DECLARE_MATH_FN(fibonacci, i64Ty, i64Ty)

    #undef DECLARE_MATH_FN
}

// ============================================================================
// Auto-Registration
// ============================================================================

HOOC_REGISTER_RUNTIME(
    Math,
    hoo_math_register_with_jit,
    hoo_math_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function
// ============================================================================

namespace hooc {

void _hoo_math_ensure_registration() {
    auto& registry = runtime::RuntimeRegistry::getInstance();
    (void)registry.getRegisteredRuntimes();
}

} // namespace hooc