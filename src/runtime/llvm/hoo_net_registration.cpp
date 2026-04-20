#include "../lib/hoo_net.h"
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

void hoo_net_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    #define REGISTER_NET_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_net_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_net_##name), \
                JITSymbolFlags::Exported \
            );

    // URL functions
    REGISTER_NET_FUNC(url_new)
    REGISTER_NET_FUNC(url_get_scheme)
    REGISTER_NET_FUNC(url_get_host)
    REGISTER_NET_FUNC(url_get_port)
    REGISTER_NET_FUNC(url_get_path)
    REGISTER_NET_FUNC(url_get_query)
    REGISTER_NET_FUNC(url_get_fragment)
    REGISTER_NET_FUNC(url_to_string)
    REGISTER_NET_FUNC(url_retain)
    REGISTER_NET_FUNC(url_release)

    // HTTP Response functions
    REGISTER_NET_FUNC(http_response_get_status_code)
    REGISTER_NET_FUNC(http_response_get_status_text)
    REGISTER_NET_FUNC(http_response_get_body)
    REGISTER_NET_FUNC(http_response_is_success)
    REGISTER_NET_FUNC(http_response_retain)
    REGISTER_NET_FUNC(http_response_release)

    // HTTP Client functions
    REGISTER_NET_FUNC(http_client_new)
    REGISTER_NET_FUNC(http_client_set_header)
    REGISTER_NET_FUNC(http_client_set_timeout)
    REGISTER_NET_FUNC(http_client_get)
    REGISTER_NET_FUNC(http_client_post)
    REGISTER_NET_FUNC(http_client_put)
    REGISTER_NET_FUNC(http_client_delete)
    REGISTER_NET_FUNC(http_client_retain)
    REGISTER_NET_FUNC(http_client_release)

    #undef REGISTER_NET_FUNC

    auto err = mainDylib.define(absoluteSymbols(symbols));
    if (err) {
        llvm::errs() << "ERROR: Failed to register Net functions with JIT: "
                     << toString(std::move(err)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

void hoo_net_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    auto ptrTy = PointerType::get(context, 0);
    auto i64Ty = Type::getInt64Ty(context);
    auto voidTy = Type::getVoidTy(context);

    #define DECLARE_NET_FN(name, returnType, ...) \
        do { \
            std::vector<Type*> params = {__VA_ARGS__}; \
            auto funcType = FunctionType::get(returnType, params, false); \
            Function::Create(funcType, Function::ExternalLinkage, "hoo_net_" #name, &module); \
        } while(0);

    // URL functions
    DECLARE_NET_FN(url_new, ptrTy, ptrTy)
    DECLARE_NET_FN(url_get_scheme, ptrTy, ptrTy)
    DECLARE_NET_FN(url_get_host, ptrTy, ptrTy)
    DECLARE_NET_FN(url_get_port, i64Ty, ptrTy)
    DECLARE_NET_FN(url_get_path, ptrTy, ptrTy)
    DECLARE_NET_FN(url_get_query, ptrTy, ptrTy)
    DECLARE_NET_FN(url_get_fragment, ptrTy, ptrTy)
    DECLARE_NET_FN(url_to_string, ptrTy, ptrTy)
    DECLARE_NET_FN(url_retain, ptrTy, ptrTy)
    DECLARE_NET_FN(url_release, voidTy, ptrTy)

    // HTTP Response functions
    DECLARE_NET_FN(http_response_get_status_code, i64Ty, ptrTy)
    DECLARE_NET_FN(http_response_get_status_text, ptrTy, ptrTy)
    DECLARE_NET_FN(http_response_get_body, ptrTy, ptrTy)
    DECLARE_NET_FN(http_response_is_success, i64Ty, ptrTy)
    DECLARE_NET_FN(http_response_retain, ptrTy, ptrTy)
    DECLARE_NET_FN(http_response_release, voidTy, ptrTy)

    // HTTP Client functions
    DECLARE_NET_FN(http_client_new, ptrTy)
    DECLARE_NET_FN(http_client_set_header, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_set_timeout, voidTy, ptrTy, i64Ty)
    DECLARE_NET_FN(http_client_get, ptrTy, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_post, ptrTy, ptrTy, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_put, ptrTy, ptrTy, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_delete, ptrTy, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_retain, ptrTy, ptrTy)
    DECLARE_NET_FN(http_client_release, voidTy, ptrTy)

    #undef DECLARE_NET_FN
}

// ============================================================================
// Auto-Registration
// ============================================================================

HOOC_REGISTER_RUNTIME(
    Net,
    hoo_net_register_with_jit,
    hoo_net_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function
// ============================================================================

namespace hooc {

void _hoo_net_ensure_registration() {
    auto& registry = runtime::RuntimeRegistry::getInstance();
    (void)registry.getRegisteredRuntimes();
}

} // namespace hooc