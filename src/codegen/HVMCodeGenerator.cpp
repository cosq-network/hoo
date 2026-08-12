#include "HVMCodeGenerator.h"
#include "ast/AST.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "ast/Primary.h"
#include "ast/Type.h"
#include "ast/ClassDeclaration.h"
#include "ast/QualifiedIdentifier.h"
#include "ast/ImportStatement.h"
#include "core/SymbolMangler.h"
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <typeinfo>
#include <atomic>
#include <sstream>
#include <functional>
#include <climits>

using namespace hvm;

namespace hooc {

// Map argument index to register number, skipping r4 (tp).
// r4 is reserved as the thread pointer and not available for args.
static uint8_t argReg(uint8_t first, size_t i) {
    // Helper for emitting 16‑bit compressed instructions (HVM‑C)
    // Stores raw bytes in a dedicated buffer; later merged into the final module.
    // Parameters:
    //   opcode4: lower 4 bits of the opcode (already fits in 0‑15)
    //   rd, rs1: registers (0‑15) for destination and source
    //   imm4: immediate value (0‑15)
    // The layout matches the decoder: [imm4|opcode4] [rd|rs1]
    // This function will be used by codegen when conditions allow compression.
    // For now, it simply appends the two bytes to compressedInstructions_.
    // Note: registers must be <=15; caller ensures this.
    // Returns nothing.
    // (Actual implementation is added later in the class.)
    uint8_t reg = static_cast<uint8_t>(first + i);
    if (reg >= 4) ++reg;
    return reg;
}

static int32_t alignFrameSize(int32_t size) {
    if (size <= 0) return 0;
    return (size + 15) & ~int32_t{15};
}





static uint32_t dictKeyTypeId(const ast::DictType& type);
static uint32_t mapKeyTypeId(const ast::MapType& type);
static uint32_t mapConstructorKeyTypeId(const ast::NewObjectExpression& expr);
static uint32_t mapConstructorValueTypeId(const ast::NewObjectExpression& expr);

// Built-in classes that support class.method mangling in JIT symbols.
static bool isClassMethodJitClass(const std::string& className) {
    static const std::unordered_set<std::string> cmClasses = {
        "String"
    };
    return cmClasses.count(className) > 0;
}

// Built-in classes that behave as singletons (no instances, all static methods).
static bool isSingletonBuiltinClass(const std::string& className) {
    static const std::unordered_set<std::string> singletons = {
    };
    return singletons.count(className) > 0;
}

// Return type for singleton built-in class methods.
static std::string singletonMethodReturnType(const std::string& className, const std::string& methodName,
                                             const std::vector<uint32_t>& argTypeIds = {}) {
    static const std::unordered_set<std::string> int64Methods = {
        "abs", "min", "max", "sign", "gcd", "factorial", "fibonacci",
        "isEven", "isOdd", "isPrime", "lcm",
        "exists", "count", "has"
    };
    static const std::unordered_set<std::string> doubleMethods = {
        "sqrt", "getPi", "getE", "getTau", "getInf", "getNegInf", "getNan",
        "pow", "cbrt", "hypot", "sin", "cos", "tan", "asin", "acos", "atan",
        "atan2", "sinh", "cosh", "tanh", "exp", "exp2", "expm1", "log",
        "log10", "log2", "log1p", "floor", "ceil", "round", "trunc", "fract"
    };
    if (className == "Math" && (methodName == "abs" || methodName == "min" ||
                                methodName == "max" || methodName == "sign")) {
        for (uint32_t typeId : argTypeIds) {
            if (typeId == 2 || typeId == 9) return "double";
            if (typeId == 5) return "int8";
            if (typeId == 6) return "byte";
        }
    }
    if (int64Methods.count(methodName)) return "int64";
    if (doubleMethods.count(methodName)) return "double";
    return "ptr";
}

// Map built-in class names to their JIT symbol prefix for modules
// that use the "prefix_methodname" convention.
static std::string classToPrefix(const std::string& className) {
    static const std::unordered_map<std::string, std::string> map = {
        {"String", "string"},
        {"DateTime", "datetime"},
        {"Math", "math"},
        {"Fs", "fs"},
        {"System", "system"},
        {"Regex", "regex"},
        {"Net", "net"},
        {"Path", "path"},
        {"Hashing", "hashing"},
        {"Uuid", "uuid"},
        {"Compression", "compression"},
        {"Character", "character"},
        {"Args", "args"},
        {"Csv", "csv"},
        {"Console", "console"},
        {"URL", "net_url"},
        {"HttpClient", "net_http_client"},
        {"HttpResponse", "net_http_response"},
        {"Thread", "thread"},
        {"Condition", "thread_condition"},
        {"Semaphore", "thread_semaphore"},
        {"Decimal", "decimal"},
        {"Mutex", "thread_mutex"},
        {"Array", "array"},
        {"Map", "map"},
        {"Buffer", "buffer"},
        {"Random", "random"},
        {"List", "list"},
        {"Dict", "dict"},
    };
    auto it = map.find(className);
    return it != map.end() ? it->second : "";
}

static uint32_t builtinConstructedTypeId(const std::string& className) {
    static const std::unordered_map<std::string, uint32_t> typeIds = {
        {"String", 101},
        {"Array", 102},
        {"Map", 103},
        {"Character", 109},
        {"Args", 110},
        {"Compression", 111},
        {"Csv", 114},
        {"Buffer", 113},
        {"URL", 106},
        {"HttpClient", 108},
        {"HttpResponse", 107},
        {"Random", 105},
        {"Dict", 117},
        {"List", 118},
        {"DateTime", 119},
        {"Regex", 120},
        {"Mutex", 121},
        {"Condition", 128},
        {"Semaphore", 129},
        {"Uuid", 122},
        {"Decimal", 125},
        {"Socket", 127},
    };
    auto it = typeIds.find(className);
    return it != typeIds.end() ? it->second : 100;
}

static std::string builtinClassNameFromTypeId(uint32_t typeId) {
    static const std::unordered_map<uint32_t, std::string> names = {
        {101, "String"}, {102, "Array"}, {103, "Map"}, {104, "Tensor"},
        {105, "Random"}, {106, "URL"}, {107, "HttpResponse"},
        {108, "HttpClient"}, {109, "Character"}, {110, "Args"},
        {111, "Compression"}, {113, "Buffer"}, {114, "Csv"},
        {117, "Dict"}, {118, "List"}, {119, "DateTime"},
        {120, "Regex"}, {121, "Mutex"}, {122, "Uuid"}, {125, "Decimal"},
        {127, "Socket"},
        {128, "Condition"}, {129, "Semaphore"}
    };
    auto it = names.find(typeId);
    return it == names.end() ? std::string{} : it->second;
}

static std::string builtinConstructorMethodName(const std::string& className, size_t argCount) {
    return "new";
}

static bool isJsonFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "json_serialize_hashmap",
        "json_serialize_anyarray",
        "json_deserialize_hashmap",
        "json_deserialize_anyarray",
        "json_minify",
        "json_beautify",
    };
    return names.count(functionName) > 0;
}

static uint32_t jsonFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "json_deserialize_hashmap") return 117;
    if (functionName == "json_deserialize_anyarray") return 118;
    return 101;
}

static bool isBufferFreeFunction(const std::string& functionName) {
    return functionName == "buffer_fromBytes" || functionName == "byte_slice_from_buffer" ||
           functionName == "byte_slice_release";
}

static bool isCsvFreeFunction(const std::string& functionName) {
    return functionName == "csv_from_opts";
}

static bool isFsFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "fs_exists",
        "fs_read_text",
        "fs_read_bytes",
        "fs_write_text",
        "fs_write_bytes",
        "fs_append_text",
        "fs_copy",
        "fs_move",
        "fs_remove",
        "fs_delete",
        "fs_mkdir",
        "fs_mkdirs",
        "fs_rmdir",
        "fs_list_dir",
        "fs_temp_dir",
        "fs_create_temp_dir",
        "fs_create_temp_file",
        "fs_current_dir",
        "fs_current_exe_dir",
        "fs_is_dir",
        "fs_is_file",
        "fs_size",
        "fs_last_modified",
    };
    return names.count(functionName) > 0;
}

static bool isDatetimeFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "datetime_now",
        "datetime_now_seconds",
        "datetime_now_precise",
        "datetime_new",
        "datetime_parse",
        "datetime_from_iso8601",
        "datetime_format",
        "datetime_iso8601",
        "datetime_add_days",
        "datetime_add_hours",
        "datetime_add_minutes",
        "datetime_add_seconds",
        "datetime_add_milliseconds",
        "datetime_diff_days",
        "datetime_diff_hours",
        "datetime_diff_seconds",
        "datetime_compare",
    };
    return names.count(functionName) > 0;
}

static bool isEncodingFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "encoding_base64_encode",
        "encoding_base64_decode",
        "encoding_hex_encode",
        "encoding_hex_decode",
        "encoding_url_encode",
        "encoding_url_decode",
        "encoding_base64_encode_buffer",
        "encoding_base64_decode_buffer",
        "encoding_hex_encode_buffer",
        "encoding_hex_decode_buffer",
        "encoding_base64_encode_slice",
        "encoding_hex_encode_slice",
    };
    return names.count(functionName) > 0;
}

static bool isMathFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "math_get_pi", "math_get_e", "math_get_tau", "math_get_inf", "math_get_neg_inf", "math_get_nan",
        "math_abs", "math_min", "math_max", "math_clamp", "math_sign", "math_pow", "math_sqrt", "math_cbrt",
        "math_hypot", "math_sin", "math_cos", "math_tan", "math_asin", "math_acos", "math_atan", "math_atan2",
        "math_sinh", "math_cosh", "math_tanh", "math_exp", "math_exp2", "math_expm1", "math_log", "math_log10",
        "math_log2", "math_log1p", "math_floor", "math_ceil", "math_round", "math_trunc", "math_fract",
        "math_is_even", "math_is_odd", "math_is_prime", "math_gcd", "math_lcm", "math_factorial", "math_fibonacci"
    };
    return names.count(functionName) > 0;
}

static bool isHashingFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "hashing_sha256", "hashing_sha256_file", "hashing_sha1", "hashing_md5", "hashing_crc32", "hashing_hmac_sha256",
        "hashing_sha256_buffer", "hashing_sha1_buffer", "hashing_md5_buffer", "hashing_crc32_buffer", "hashing_hmac_sha256_buffer"
        , "hashing_sha256_slice", "hashing_sha1_slice", "hashing_md5_slice", "hashing_crc32_slice"
    };
    return names.count(functionName) > 0;
}

static bool isSystemFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "system_get_env", "system_set_env", "system_unset_env", "system_hostname", "system_os_name",
        "system_os_version", "system_cpu_count", "system_process_id", "system_uptime_ms", "system_exit",
        "system_exec", "system_exec_status", "system_user_home", "system_user_name", "system_current_dir",
        "system_set_current_dir", "system_total_memory", "system_free_memory"
    };
    return names.count(functionName) > 0;
}

static bool isProcessFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "process_self_pid", "process_capture", "process_kill", "process_spawn", "process_wait"
    };
    return names.count(functionName) > 0;
}

static uint32_t processFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "process_capture") return 101; // string (type ID 101)
    return 1; // int64 (type ID 1)
}

static bool isRegexFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "regex_match", "regex_search", "regex_replace", "regex_split"
    };
    return names.count(functionName) > 0;
}

static uint32_t regexFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "regex_replace") return 101; // string
    if (functionName == "regex_split") return 102; // array
    return 1; // int64 (match, search)
}

static bool isThreadFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "thread_self", "thread_spawn", "thread_join"
    };
    return names.count(functionName) > 0;
}

static uint32_t threadFreeFunctionReturnTypeId(const std::string& functionName) {
    return 1; // int64
}

static bool isUuidFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "uuid_v4", "uuid_nil", "uuid_is_nil", "uuid_from_bytes", "uuid_to_bytes", "uuid_equals", "uuid_compare", "uuid_to_string"
    };
    return names.count(functionName) > 0;
}

static uint32_t uuidFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "uuid_from_bytes") return 122; // Uuid
    if (functionName == "uuid_to_bytes") return 113; // Buffer
    if (functionName == "uuid_v4" || functionName == "uuid_nil" || functionName == "uuid_to_string") return 101; // string
    return 1; // int64
}

static bool isCharacterFreeFunction(const std::string& functionName) {
    return functionName == "character_from_utf8";
}

static uint32_t characterFreeFunctionReturnTypeId(const std::string& functionName) {
    return 109; // Character type ID is 109
}

static bool isPathFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "path_separator", "path_join", "path_extension", "path_stem",
        "path_filename", "path_parent", "path_absolute", "path_normalize",
        "path_root", "path_relative", "path_has_extension", "path_split",
        "path_dirname", "path_basename", "path_is_absolute", "path_is_relative",
        "path_list_separator"
    };
    return names.count(functionName) > 0;
}

static uint32_t pathFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "path_separator" || functionName == "path_list_separator") return 6; // byte/char
    if (functionName == "path_is_absolute" || functionName == "path_is_relative" ||
        functionName == "path_has_extension") return 1; // int64
    return 101; // string
}

static bool isArgsFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "args_get", "args_count"
    };
    return names.count(functionName) > 0;
}

static uint32_t argsFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "args_count") return 1; // int64
    return 101; // string
}

static bool isStringFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "string_repeat", "string_from_int64", "string_from_double", "string_join"
    };
    return names.count(functionName) > 0;
}

static uint32_t stringFreeFunctionReturnTypeId(const std::string& functionName) {
    return 101; // string
}

static bool isNetFreeFunction(const std::string& functionName) {
    static const std::unordered_set<std::string> names = {
        "net_socket_new", "net_socket_bind", "net_socket_listen", "net_socket_connect",
        "net_socket_connect_tls", "net_socket_enable_tls_server", "net_socket_set_timeout",
        "net_socket_accept", "net_socket_send", "net_socket_receive", "net_socket_last_error",
        "net_socket_local_port", "net_socket_close", "net_socket_retain", "net_socket_release",
    };
    return names.count(functionName) > 0;
}

static uint32_t netFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "net_socket_new" || functionName == "net_socket_accept" ||
        functionName == "net_socket_retain") return 127; // Socket
    if (functionName == "net_socket_receive") return 113; // Buffer
    if (functionName == "net_socket_last_error") return 101; // String
    if (functionName == "net_socket_release") return 4; // void
    return 1; // int64
}

static bool isHooModuleFreeFunction(const std::string& functionName) {
    return isJsonFreeFunction(functionName) || isBufferFreeFunction(functionName) ||
           isCsvFreeFunction(functionName) || isFsFreeFunction(functionName) ||
           isDatetimeFreeFunction(functionName) || isEncodingFreeFunction(functionName) ||
           isMathFreeFunction(functionName) || isHashingFreeFunction(functionName) ||
           isSystemFreeFunction(functionName) || isProcessFreeFunction(functionName) ||
           isRegexFreeFunction(functionName) || isThreadFreeFunction(functionName) ||
           isUuidFreeFunction(functionName) || isCharacterFreeFunction(functionName) ||
           isPathFreeFunction(functionName) || isArgsFreeFunction(functionName) ||
           isStringFreeFunction(functionName) || isNetFreeFunction(functionName);
}

static uint32_t datetimeFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "datetime_now_seconds") return 1;
    if (functionName == "datetime_now_precise") return 2;
    if (functionName == "datetime_format" || functionName == "datetime_iso8601") return 101; // string is type ID 101
    if (functionName == "datetime_diff_days" ||
        functionName == "datetime_diff_hours" ||
        functionName == "datetime_compare") return 1; // int64
    if (functionName == "datetime_diff_seconds") return 2; // double
    return 119; // DateTime is 119
}

static uint32_t fsFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "fs_exists" || functionName == "fs_is_dir" ||
        functionName == "fs_is_file" || functionName == "fs_size" ||
        functionName == "fs_remove" || functionName == "fs_delete" ||
        functionName == "fs_mkdir" || functionName == "fs_mkdirs" ||
        functionName == "fs_rmdir" || functionName == "fs_copy" ||
        functionName == "fs_move") return 1;
    if (functionName == "fs_read_bytes" || functionName == "fs_read_bytes_default") return 113;
    if (functionName == "fs_read_text" || functionName == "fs_read_text_default") return 101;
    if (functionName == "fs_list_dir") return 102;
    if (functionName == "fs_last_modified") return 101;
    return 100;
}

static uint32_t mathFreeFunctionReturnTypeId(const std::string& functionName, const std::vector<uint32_t>& argTypeIds) {
    static const std::unordered_set<std::string> int64Methods = {
        "math_abs", "math_min", "math_max", "math_sign", "math_gcd", "math_factorial", "math_fibonacci",
        "math_is_even", "math_is_odd", "math_is_prime", "math_lcm"
    };
    static const std::unordered_set<std::string> doubleMethods = {
        "math_sqrt", "math_get_pi", "math_get_e", "math_get_tau", "math_get_inf", "math_get_neg_inf", "math_get_nan",
        "math_pow", "math_cbrt", "math_hypot", "math_sin", "math_cos", "math_tan", "math_asin", "math_acos", "math_atan",
        "math_atan2", "math_sinh", "math_cosh", "math_tanh", "math_exp", "math_exp2", "math_expm1", "math_log",
        "math_log10", "math_log2", "math_log1p", "math_floor", "math_ceil", "math_round", "math_trunc", "math_fract", "math_clamp"
    };
    if (functionName == "math_abs" || functionName == "math_min" ||
        functionName == "math_max" || functionName == "math_sign") {
        for (uint32_t typeId : argTypeIds) {
            if (typeId == 2 || typeId == 9) return 2; // double
            if (typeId == 5) return 5; // int8
            if (typeId == 6) return 6; // byte
        }
    }
    if (int64Methods.count(functionName)) return 1; // int64
    if (doubleMethods.count(functionName)) return 2; // double
    return 100;
}

static uint32_t hashingFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "hashing_crc32" || functionName == "hashing_crc32_buffer") return 1; // int64
    return 101; // string
}

static uint32_t systemFreeFunctionReturnTypeId(const std::string& functionName) {
    if (functionName == "system_get_env" || functionName == "system_hostname" ||
        functionName == "system_os_name" || functionName == "system_os_version" ||
        functionName == "system_exec" || functionName == "system_user_home" ||
        functionName == "system_user_name" || functionName == "system_current_dir") {
        return 101; // string
    }
    if (functionName == "system_exit") return 4; // void
    return 1; // int64
}

static uint32_t hooModuleFreeFunctionReturnTypeId(const std::string& functionName, const std::vector<uint32_t>& argTypeIds) {
    if (isJsonFreeFunction(functionName)) return jsonFreeFunctionReturnTypeId(functionName);
    if (isBufferFreeFunction(functionName)) {
        if (functionName == "byte_slice_from_buffer") return 130;
        if (functionName == "byte_slice_release") return 4;
        return 113;
    }
    if (isCsvFreeFunction(functionName)) return 114;
    if (isFsFreeFunction(functionName)) return fsFreeFunctionReturnTypeId(functionName);
    if (isDatetimeFreeFunction(functionName)) return datetimeFreeFunctionReturnTypeId(functionName);
    if (isEncodingFreeFunction(functionName)) {
        if (functionName == "encoding_base64_decode_buffer" ||
            functionName == "encoding_hex_decode_buffer") {
            return 113; // Buffer type ID is 113
        }
        return 101; // String type ID is 101
    }
    if (isMathFreeFunction(functionName)) return mathFreeFunctionReturnTypeId(functionName, argTypeIds);
    if (isHashingFreeFunction(functionName)) return hashingFreeFunctionReturnTypeId(functionName);
    if (isSystemFreeFunction(functionName)) return systemFreeFunctionReturnTypeId(functionName);
    if (isProcessFreeFunction(functionName)) return processFreeFunctionReturnTypeId(functionName);
    if (isRegexFreeFunction(functionName)) return regexFreeFunctionReturnTypeId(functionName);
    if (isThreadFreeFunction(functionName)) return threadFreeFunctionReturnTypeId(functionName);
    if (isUuidFreeFunction(functionName)) return uuidFreeFunctionReturnTypeId(functionName);
    if (isCharacterFreeFunction(functionName)) return characterFreeFunctionReturnTypeId(functionName);
    if (isPathFreeFunction(functionName)) return pathFreeFunctionReturnTypeId(functionName);
    if (isArgsFreeFunction(functionName)) return argsFreeFunctionReturnTypeId(functionName);
    if (isStringFreeFunction(functionName)) return stringFreeFunctionReturnTypeId(functionName);
    if (isNetFreeFunction(functionName)) return netFreeFunctionReturnTypeId(functionName);
    return 100;
}

HVMCodeGenerator::HVMCodeGenerator() {
    // Initialize compressed instruction buffer
    compressedInstructions_.clear();
    for (int i = 0; i < 32; ++i) usedRegs_[i] = false;
    // Reserved registers
    usedRegs_[0] = true; // r0 is hardwired zero
    usedRegs_[4] = true; // r4 is tp (thread pointer)
    usedRegs_[29] = true; // lr
    usedRegs_[30] = true; // fp
    usedRegs_[31] = true; // sp

    // Pre-populate standard library class layouts
    ClassLayout excLayout;
    excLayout.name = "Exception";
    excLayout.fieldOffsets["typeId"] = 0;
    excLayout.fieldOffsets["typeName"] = 8;
    excLayout.fieldOffsets["message"] = 16;
    excLayout.fieldOffsets["refcount"] = 24;
    excLayout.fieldOffsets["cause"] = 32;
    excLayout.totalSize = 40;
    classes_["Exception"] = excLayout;
    classes_["hoo.Exception"] = excLayout;

    // Pre-populate DateTime class layout (instantiable, serializable-ready)
    ClassLayout dtLayout;
    dtLayout.name = "DateTime";
    dtLayout.fieldOffsets["timestamp"] = 0;
    dtLayout.fieldAccess["timestamp"] = FieldAccess::PUBLIC;
    dtLayout.totalSize = 8;
    dtLayout.isSingleton = false;
    dtLayout.isFinal = false;
    dtLayout.isImmutable = false;
    dtLayout.isService = false;
    classes_["DateTime"] = dtLayout;
    classes_["hoo.DateTime"] = dtLayout;
}
bool HVMCodeGenerator::isModuleImported(const std::string& moduleName) const {
    if (moduleName.empty()) return true;
    
    // Core module "hoo" is satisfied by "import hoo;"
    if (moduleName == "hoo") {
        return importedModules_.count("hoo") > 0;
    }
    
    // Submodules (e.g. "hoo.math") require importing the submodule itself or a prefix longer than "hoo"
    if (importedModules_.count(moduleName) > 0) return true;
    
    std::string prefix = "";
    std::stringstream ss(moduleName);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (!prefix.empty()) prefix += ".";
        prefix += part;
        if (prefix != "hoo" && importedModules_.count(prefix) > 0) return true;
    }
    return false;
}

bool HVMCodeGenerator::isSymbolImported(const std::string& name, const std::string& requiredModule) const {
    if (requiredModule.empty()) return true;
    if (isModuleImported(requiredModule)) return true;
    
    auto it = importedSymbols_.find(name);
    if (it != importedSymbols_.end()) {
        const std::string& importedFrom = it->second;
        if (importedFrom != "hoo" && (requiredModule == importedFrom || requiredModule.rfind(importedFrom + ".", 0) == 0)) {
            return true;
        }
        if (requiredModule == "hoo" && importedFrom == "hoo") {
            return true;
        }
    }
    return false;
}

std::string HVMCodeGenerator::getRequiredModule(const std::string& name) const {
    // Intrinsic data types exempt from imports
    if (name == "String" || name == "Array" || name == "Map" || name == "Exception") {
        return "";
    }

    // Built-in standard library classes
    if (name == "DateTime") return "hoo.datetime";
    if (name == "Math" || name == "Random") return "hoo.math";
    if (name == "Fs") return "hoo.io";
    if (name == "System") return "hoo.system";
    if (name == "Regex") return "hoo.regex";
    if (name == "Net" || name == "URL" || name == "HttpClient" || name == "HttpResponse") return "hoo.net";
    if (name == "Mutex") return "hoo.thread";
    if (name == "Condition" || name == "Semaphore") return "hoo.thread";
    if (name == "Path") return "hoo.path";
    if (name == "Hashing") return "hoo.hashing";
    if (name == "Uuid") return "hoo.uuid";
    if (name == "Compression") return "hoo.compression";
    if (name == "Args") return "hoo.args";
    if (name == "Csv") return "hoo.csv";
    if (name == "Console") return "hoo";
    if (name == "Thread") return "hoo.thread";
    if (name == "Character") return "hoo.character";
    if (name == "Buffer") return "hoo.buffer";
    if (name == "Dict" || name == "List") return "hoo.collections";
    if (name == "StringBuilder") return "hoo.string";

    // Free functions or other symbols with prefixes
    if (name.rfind("datetime_", 0) == 0) return "hoo.datetime";
    if (name.rfind("fs_", 0) == 0) return "hoo.io";
    if (name.rfind("json_", 0) == 0) return "hoo.json";
    if (name.rfind("buffer_", 0) == 0 || name.rfind("byte_slice_", 0) == 0) return "hoo.buffer";
    if (name.rfind("csv_", 0) == 0) return "hoo.csv";
    if (name.rfind("math_", 0) == 0) return "hoo.math";
    if (name.rfind("net_", 0) == 0) return "hoo.net";
    if (name.rfind("path_", 0) == 0) return "hoo.path";
    if (name.rfind("hashing_", 0) == 0) return "hoo.hashing";
    if (name.rfind("encoding_", 0) == 0) return "hoo.encoding";
    if (name.rfind("uuid_", 0) == 0) return "hoo.uuid";
    if (name.rfind("compression_", 0) == 0) return "hoo.compression";
    if (name.rfind("process_", 0) == 0) return "hoo.process";
    if (name.rfind("args_", 0) == 0) return "hoo.args";
    if (name.rfind("thread_", 0) == 0) return "hoo.thread";
    if (name.rfind("character_", 0) == 0) return "hoo.character";
    if (name.rfind("system_", 0) == 0) return "hoo.system";
    if (name.rfind("regex_", 0) == 0) return "hoo.regex";
    if (name.rfind("string_", 0) == 0) return "hoo";

    return "";
}

void HVMCodeGenerator::setModuleContext(const std::string& moduleName) {
    pendingModuleName_ = moduleName;
}

void HVMCodeGenerator::setExternalFunctionImports(
    const std::unordered_map<std::string, std::pair<std::string, std::string>>& functions) {
    externalFunctionImports_ = functions;
}

void HVMCodeGenerator::setExternalFunctionMetadata(
    const std::unordered_map<std::string, ExternalFunctionInfo>& functions) {
    externalFunctionMetadata_ = functions;
    externalFunctionMetadataSets_.clear();
    for (const auto& [name, info] : functions) externalFunctionMetadataSets_[name].push_back(info);
    for (const auto& [name, info] : functions) {
        if (functionReturnTypes_.find(name) == functionReturnTypes_.end()) {
            const std::string& type = info.returnType;
            if (type == "int64") functionReturnTypes_[name] = 1;
            else if (type == "double") functionReturnTypes_[name] = 2;
            else if (type == "bool") functionReturnTypes_[name] = 3;
            else if (type == "void") functionReturnTypes_[name] = 4;
            else if (type == "int8") functionReturnTypes_[name] = 5;
            else if (type == "byte") functionReturnTypes_[name] = 6;
            else if (type == "char") functionReturnTypes_[name] = 7;
            else if (type == "bit") functionReturnTypes_[name] = 8;
            else if (type == "f8") functionReturnTypes_[name] = 9;
            else if (type == "string") functionReturnTypes_[name] = 101;
            else if (type == "tensor") functionReturnTypes_[name] = 104;
            else if (type == "any") functionReturnTypes_[name] = 0;
            else functionReturnTypes_[name] = 100;
        }
    }
}

void HVMCodeGenerator::setExternalFunctionMetadataSets(const ExternalFunctionMetadataSets& functions) {
    externalFunctionMetadataSets_ = functions;
    externalFunctionMetadata_.clear();
    for (const auto& [name, overloads] : functions) {
        if (!overloads.empty()) externalFunctionMetadata_[name] = overloads.front();
    }
}

std::unique_ptr<GeneratedModule> HVMCodeGenerator::generateModule(const ast::CompilationUnit& compilationUnit) {
    // 1. Determine Module Name/Path
    static std::atomic<uint64_t> sSyntheticModuleCounter{0};
    std::string moduleName = pendingModuleName_;
    if (moduleName.empty()) {
        moduleName = "hvm_module_" + std::to_string(++sSyntheticModuleCounter);
    }
    pendingModuleName_.clear();
    modulePath_.clear();
    {
        std::stringstream ss(moduleName);
        std::string part;
        while (std::getline(ss, part, '.')) {
            if (!part.empty()) modulePath_.push_back(part);
        }
    }
    
    // In a real scenario, this would come from the compiler's source tracking.
    // Append any compressed instructions collected during codegen.
    if (!compressedInstructions_.empty()) {
        // Create a dummy section for compressed code (treated as code)
        Section* compSec = module_->getSection(".compcode");
        if (!compSec) {
            Section s; s.name = ".compcode"; s.type = SectionType::SHT_TEXT; s.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE; module_->addSection(std::move(s));
            compSec = module_->getSection(".compcode");
        }
        compSec->data.insert(compSec->data.end(), compressedInstructions_.begin(), compressedInstructions_.end());
        compSec->virtual_size = compSec->data.size();
    }
    // For now we look for a marker or use default.
    module_ = std::make_unique<hvm::HOModule>(moduleName);
    moduleUsesNullChecks_ = false;
    instructions_.clear();
    currentByteOffset_ = 0;
    errors_.clear();
    scopeStack_.clear();
    currentStackOffset_ = 0;
    allLabels_.clear();
    symbolFixups_.clear();
    functionReturnTypes_.clear();
    functionReturnClass_.clear();
    functionOverloadReturns_.clear();
    functionFutureElementTypes_.clear();
    for (const auto& [name, info] : externalFunctionMetadata_) {
        const std::string& type = info.returnType;
        uint32_t typeId = 100;
        if (type == "int64") typeId = 1;
        else if (type == "double") typeId = 2;
        else if (type == "bool") typeId = 3;
        else if (type == "void") typeId = 4;
        else if (type == "int8") typeId = 5;
        else if (type == "byte") typeId = 6;
        else if (type == "char") typeId = 7;
        else if (type == "bit") typeId = 8;
        else if (type == "f8") typeId = 9;
        else if (type == "string") typeId = 101;
        else if (type == "tensor") typeId = 104;
        else if (type == "any") typeId = 0;
        functionReturnTypes_[name] = typeId;
        if (!info.returnClass.empty()) functionReturnClass_[name] = info.returnClass;
        if (!info.parameterTypes.empty()) {
            OverloadReturnInfo overload;
            for (const auto& parameter : info.parameterTypes) {
                uint32_t parameterId = 100;
                if (parameter == "int64") parameterId = 1;
                else if (parameter == "double") parameterId = 2;
                else if (parameter == "bool") parameterId = 3;
                else if (parameter == "int8") parameterId = 5;
                else if (parameter == "byte") parameterId = 6;
                else if (parameter == "char") parameterId = 7;
                else if (parameter == "bit") parameterId = 8;
                else if (parameter == "f8") parameterId = 9;
                else if (parameter == "string") parameterId = 101;
                else if (parameter == "tensor") parameterId = 104;
                else if (parameter == "any") parameterId = 0;
                overload.parameterTypes.push_back(parameterId);
                overload.parameterIsNullable.push_back(false);
            }
            overload.result.typeId = typeId;
            functionOverloadReturns_[name].push_back(std::move(overload));
        }
    }
    auto externalTypeId = [](const std::string& type) -> uint32_t {
        if (type == "int64") return 1;
        if (type == "double") return 2;
        if (type == "bool") return 3;
        if (type == "void") return 4;
        if (type == "int8") return 5;
        if (type == "byte") return 6;
        if (type == "char") return 7;
        if (type == "bit") return 8;
        if (type == "f8") return 9;
        if (type == "string") return 101;
        if (type == "tensor") return 104;
        if (type == "any") return 0;
        return 100;
    };
    for (const auto& [name, overloads] : externalFunctionMetadataSets_) {
        for (size_t index = externalFunctionMetadata_.count(name) ? 1 : 0;
             index < overloads.size(); ++index) {
            const auto& info = overloads[index];
            OverloadReturnInfo overload;
            for (const auto& parameter : info.parameterTypes) {
                overload.parameterTypes.push_back(externalTypeId(parameter));
                overload.parameterIsNullable.push_back(false);
            }
            overload.result.typeId = externalTypeId(info.returnType);
            overload.result.className = info.returnClass;
            functionOverloadReturns_[name].push_back(std::move(overload));
        }
    }
    classDeclarations_.clear();

    importedModules_.clear();
    importedSymbols_.clear();
    for (const auto& imp : compilationUnit.getImports()) {
        if (auto basicImport = dynamic_cast<const ast::BasicImport*>(imp.get())) {
            const ast::ModulePath* modulePath = basicImport->getModule();
            if (modulePath) {
                std::string fullModuleName = "";
                for (const auto& comp : modulePath->getComponents()) {
                    if (!fullModuleName.empty()) fullModuleName += ".";
                    fullModuleName += comp;
                }
                importedModules_.insert(fullModuleName);
            }
        } else if (auto fromImport = dynamic_cast<const ast::FromImport*>(imp.get())) {
            const ast::ModulePath* modulePath = fromImport->getModule();
            if (modulePath) {
                std::string fullModuleName = "";
                for (const auto& comp : modulePath->getComponents()) {
                    if (!fullModuleName.empty()) fullModuleName += ".";
                    fullModuleName += comp;
                }
                importedModules_.insert(fullModuleName);
                for (const auto& item : fromImport->getItems()) {
                    if (item) {
                        importedSymbols_[item->getName()] = fullModuleName;
                    }
                }
            }
        }
    }

    // Register top-level function return types before emitting any body so
    // direct calls can mangle f8/bit and other return types regardless of order.
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(decl.get())) {
            if (funcDecl->isAsync()) {
                uint32_t elementType = 4;
                if (auto futureType = dynamic_cast<const ast::FutureType*>(funcDecl->getReturnType())) {
                    elementType = typeIdFromDeclaredType(&futureType->getElementType());
                }
                functionFutureElementTypes_[funcDecl->getName()] = elementType;
            }
            if (funcDecl->getReturnType()) {
                std::string clsName;
                functionReturnTypes_[funcDecl->getName()] = typeIdFromDeclaredType(funcDecl->getReturnType(), &clsName);
                if (!clsName.empty()) {
                    functionReturnClass_[funcDecl->getName()] = clsName;
                }
            } else {
                functionReturnTypes_[funcDecl->getName()] = 4;
            }
        } else if (auto overList = dynamic_cast<const ast::OverloadList*>(decl.get())) {
            for (const auto& funcDecl : overList->getFunctions()) {
                isOverloadedFunction_[funcDecl->getName()] = true;
                OverloadReturnInfo overloadInfo;
                for (const auto& param : funcDecl->getParameters()) {
                    overloadInfo.parameterTypes.push_back(typeIdFromDeclaredType(&param->getType()));
                    overloadInfo.parameterIsNullable.push_back(isNullableDeclaredType(&param->getType()));
                }
                if (funcDecl->getReturnType()) {
                    std::string clsName;
                    overloadInfo.result.typeId = typeIdFromDeclaredType(funcDecl->getReturnType(), &clsName);
                    overloadInfo.result.className = clsName;
                    functionReturnTypes_[funcDecl->getName()] = overloadInfo.result.typeId;
                    if (!clsName.empty()) {
                        functionReturnClass_[funcDecl->getName()] = clsName;
                    }
                } else {
                    overloadInfo.result.typeId = 4;
                }
                functionOverloadReturns_[funcDecl->getName()].push_back(std::move(overloadInfo));
            }
        }
    }

    // Register class modifier metadata before validation so forward references
    // between serializable/service classes can be resolved consistently.
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto classDecl = dynamic_cast<const ast::ClassDeclaration*>(decl.get())) {
            classDeclarations_[classDecl->getName()] = classDecl;
            ClassLayout layout;
            layout.name = classDecl->getName();
            layout.isSingleton = classDecl->hasModifier(ast::ClassModifier::SINGLETON);
            layout.isFinal = classDecl->hasModifier(ast::ClassModifier::FINAL);
            layout.isImmutable = classDecl->hasModifier(ast::ClassModifier::IMMUTABLE);
            layout.isService = classDecl->hasModifier(ast::ClassModifier::SERVICE);
            layout.isSerializable = classDecl->hasModifier(ast::ClassModifier::SERIALIZABLE);
            if (classDecl->hasBaseClass()) {
                layout.baseClass = classDecl->getBaseClass();
            }
            classes_[layout.name] = layout;
        }
    }

    // 2. Process Imports (SHT_IMPORT)
    for (const auto& imp : compilationUnit.getImports()) {
        const ast::ModulePath* pathNode = nullptr;
        if (auto basic = dynamic_cast<const ast::BasicImport*>(imp.get())) {
            pathNode = basic->getModule();
        } else if (auto fromImp = dynamic_cast<const ast::FromImport*>(imp.get())) {
            pathNode = fromImp->getModule();
        }

        if (pathNode) {
            std::string fullName;
            for (const auto& part : pathNode->getComponents()) {
                if (!fullName.empty()) fullName += ".";
                fullName += part;
            }
            module_->addDependency(fullName, ModuleType::Compiled);
        }
    }

    // 3. Process all top-level declarations
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(decl.get())) {
            visitFunction(*funcDecl);
        } else if (auto varDecl = dynamic_cast<const ast::VariableDeclaration*>(decl.get())) {
            // Allocate space for global variable
            uint32_t dataOffset = 0;
            std::string secName = varDecl->isConstant() ? ".rodata" : ".data";
            SectionType secType = varDecl->isConstant() ? SectionType::SHT_RODATA : SectionType::SHT_DATA;
            uint32_t secFlags = varDecl->isConstant() ? SectionFlags::ALLOC : (SectionFlags::ALLOC | SectionFlags::WRITE);
            
            Section* sec = module_->getSection(secName);
            if (!sec) {
                Section s;
                s.name = secName;
                s.type = secType;
                s.flags = secFlags;
                module_->addSection(std::move(s));
                sec = module_->getSection(secName);
            }
            
            dataOffset = static_cast<uint32_t>(sec->data.size());
            // Reserve 8 bytes (all globals 64-bit for now)
            for (int i = 0; i < 8; ++i) sec->data.push_back(0);
            sec->virtual_size = sec->data.size();

            Symbol sym;
            if (!modulePath_.empty()) {
                sym.name = SymbolMangler::mangleModuleSymbol(modulePath_, varDecl->getName());
            } else {
                sym.name = varDecl->getName();
            }
            sym.value = dataOffset;
            sym.type = Symbol::STT_OBJECT;
            sym.binding = Symbol::STB_GLOBAL;
            sym.section_index = 0; 
            module_->addSymbol(sym);

        } else if (auto classDecl = dynamic_cast<const ast::ClassDeclaration*>(decl.get())) {
            ClassLayout layout;
            layout.name = classDecl->getName();
            layout.isSingleton = classDecl->hasModifier(ast::ClassModifier::SINGLETON);
            layout.isFinal = classDecl->hasModifier(ast::ClassModifier::FINAL);
            layout.isImmutable = classDecl->hasModifier(ast::ClassModifier::IMMUTABLE);
            layout.isService = classDecl->hasModifier(ast::ClassModifier::SERVICE);
            layout.isSerializable = classDecl->hasModifier(ast::ClassModifier::SERIALIZABLE);
            
            // Service validation: cannot be combined with singleton, immutable, final, or serializable
            if (layout.isService) {
                if (layout.isSingleton) {
                    addError("Service class '" + layout.name + "' cannot also be singleton");
                }
                if (layout.isImmutable) {
                    addError("Service class '" + layout.name + "' cannot also be immutable");
                }
                if (layout.isFinal) {
                    addError("Service class '" + layout.name + "' cannot also be final");
                }
                if (layout.isSerializable) {
                    addError("Service class '" + layout.name + "' cannot also be serializable");
                }
                // Validate constructor parameters
                for (const auto& member : classDecl->getBody().getMembers()) {
                    if (auto ctor = member->getConstructor()) {
                        for (const auto& param : ctor->getParameters()) {
                            auto* bt = dynamic_cast<const ast::BaseType*>(&param->getType());
                            if (bt && bt->isPrimitive()) {
                                addError("Service class '" + layout.name + "' constructor parameter '" + param->getName() + "' cannot be primitive type");
                            } else {
                                std::string typeName = bt ? bt->getIdentifier() : "object";
                                auto depIt = classes_.find(typeName);
                                if (depIt == classes_.end() || !depIt->second.isService) {
                                    addError("Service class '" + layout.name + "' constructor parameter '" + param->getName() + "' must be a service class, got '" + typeName + "'");
                                }
                            }
                        }
                    }
                }
            }
            
            // Final check: validate base class is not final
            if (classDecl->hasBaseClass()) {
                auto baseIt = classes_.find(classDecl->getBaseClass());
                if (baseIt != classes_.end() && baseIt->second.isFinal) {
                    addError("Cannot extend final class '" + classDecl->getBaseClass() + "'");
                }
            }
            
            // Calculate field offsets
            int32_t currentOffset = 0;
            // A derived object starts after its base subobject.  Preserve the
            // base layout so field access, constructors, and generated
            // serialization all agree on one stable ABI.
            if (classDecl->hasBaseClass()) {
                auto baseIt = classes_.find(classDecl->getBaseClass());
                if (baseIt != classes_.end()) {
                    layout.fieldOffsets = baseIt->second.fieldOffsets;
                    layout.fieldTypeIds = baseIt->second.fieldTypeIds;
                    layout.fieldClassNames = baseIt->second.fieldClassNames;
                    layout.fieldElementTypeIds = baseIt->second.fieldElementTypeIds;
                    layout.fieldAccess = baseIt->second.fieldAccess;
                    layout.fieldIsNullable = baseIt->second.fieldIsNullable;
                    currentOffset = baseIt->second.totalSize;
                }
            }
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                        layout.fieldOffsets[var->getName()] = currentOffset;
                        std::string fieldClassName;
                        layout.fieldTypeIds[var->getName()] =
                            var->getType() ? typeIdFromDeclaredType(var->getType(), &fieldClassName) : 100;
                        layout.fieldClassNames[var->getName()] = fieldClassName;
                        layout.fieldIsNullable[var->getName()] =
                            var->getType() ? isNullableDeclaredType(var->getType()) : false;
                        if (auto arrType = var->getType()
                                ? dynamic_cast<const ast::ArrayType*>(var->getType()) : nullptr) {
                            layout.fieldElementTypeIds[var->getName()] =
                                typeIdFromDeclaredType(&arrType->getBaseType());
                        }
                        if (var->isPrivate()) {
                            layout.fieldAccess[var->getName()] = FieldAccess::PRIVATE;
                        } else if (var->isPublic()) {
                            layout.fieldAccess[var->getName()] = FieldAccess::PUBLIC;
                        } else {
                            layout.fieldAccess[var->getName()] = FieldAccess::DEFAULT_VAR;
                        }
                        currentOffset += 8;
                    }
                }
            }
            layout.totalSize = currentOffset;
            if (classDecl->hasBaseClass()) {
                layout.baseClass = classDecl->getBaseClass();
            }
            classes_[layout.name] = layout;

            // Index methods for fallback resolution. A method name may be
            // declared by unrelated classes, so retain all candidates.
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto fn = dynamic_cast<const ast::FunctionDeclaration*>(declMember)) {
                        auto& candidates = methodNameToClasses_[fn->getName()];
                        if (std::find(candidates.begin(), candidates.end(), layout.name) == candidates.end()) {
                            candidates.push_back(layout.name);
                        }
                        layout.privateMethods[fn->getName()] = fn->isPrivate();
                        if (fn->getReturnType()) {
                            std::string returnClass;
                            layout.methodReturnTypes[fn->getName()] =
                                typeIdFromDeclaredType(fn->getReturnType(), &returnClass);
                            layout.methodReturnClasses[fn->getName()] = returnClass;
                        } else {
                            layout.methodReturnTypes[fn->getName()] = 4; // void
                        }
                    } else if (auto overList = dynamic_cast<const ast::OverloadList*>(declMember)) {
                        for (const auto& fn : overList->getFunctions()) {
                            auto& candidates = methodNameToClasses_[fn->getName()];
                            if (std::find(candidates.begin(), candidates.end(), layout.name) == candidates.end()) {
                                candidates.push_back(layout.name);
                            }
                            isOverloadedMethod_[layout.name][fn->getName()] = true;
                            layout.privateMethods[fn->getName()] = fn->isPrivate();
                            OverloadReturnInfo overloadInfo;
                            for (const auto& param : fn->getParameters()) {
                                overloadInfo.parameterTypes.push_back(typeIdFromDeclaredType(&param->getType()));
                                overloadInfo.parameterIsNullable.push_back(isNullableDeclaredType(&param->getType()));
                            }
                            if (fn->getReturnType()) {
                                std::string returnClass;
                                layout.methodReturnTypes[fn->getName()] =
                                    typeIdFromDeclaredType(fn->getReturnType(), &returnClass);
                                layout.methodReturnClasses[fn->getName()] = returnClass;
                                overloadInfo.result.typeId = layout.methodReturnTypes[fn->getName()];
                                overloadInfo.result.className = returnClass;
                            } else {
                                layout.methodReturnTypes[fn->getName()] = 4;
                                overloadInfo.result.typeId = 4;
                            }
                            layout.methodOverloadReturns[fn->getName()].push_back(std::move(overloadInfo));
                        }
                    }
                }
            }
            // Store all method metadata, including return classes, after the
            // indexing pass. Keeping only privateMethods here discarded the
            // information required for chained return-type inference.
            classes_[layout.name] = layout;

            // Serializable validation
            if (layout.isSerializable) {
                validateSerializableClass(*classDecl, layout, layout.name);
            }

            // Singleton validation: constructor must have no arguments
            if (layout.isSingleton) {
                for (const auto& member : classDecl->getBody().getMembers()) {
                    if (auto ctor = member->getConstructor()) {
                        if (ctor->isFactory()) {
                            addError("Singleton class '" + layout.name + "' cannot declare factory constructors");
                        }
                        if (!ctor->getParameters().empty()) {
                            addError("Singleton class '" + layout.name + "' constructor must have no parameters");
                        }
                    }
                }
                // Reserve .data slot for singleton instance pointer
                Section* dataSec = module_->getSection(".data");
                if (!dataSec) {
                    Section s;
                    s.name = ".data";
                    s.type = SectionType::SHT_DATA;
                    s.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
                    module_->addSection(std::move(s));
                    dataSec = module_->getSection(".data");
                }
                layout.singletonDataOffset = static_cast<uint32_t>(dataSec->data.size());
                for (int i = 0; i < 8; ++i) dataSec->data.push_back(0);
                dataSec->virtual_size = dataSec->data.size();
                pendingSingletons_.push_back({layout.name, layout.singletonDataOffset});
                // Update the layout in classes_ after allocation
                classes_[layout.name] = layout;
            }

            // Process methods
            currentClass_ = &classes_[layout.name];
            inConstructor_ = false;
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto fn = dynamic_cast<const ast::FunctionDeclaration*>(declMember)) {
                        visitMethod(*fn);
                    }
                } else if (auto ctor = member->getConstructor()) {
                    if (ctor->isFactory()) {
                        classes_[layout.name].factoryNames.push_back(ctor->getName());
                    }
                    visitConstructor(*ctor);
                }
            }
            // Emit generated serialize/deserialize methods for serializable classes
            if (layout.isSerializable) {
                emitSerializeMethod(layout, *classDecl);
                emitDeserializeMethod(layout, *classDecl);
            }
            currentClass_ = nullptr;
        }
    }

    // Serializable class cycle detection (must run after all classes are loaded)
    if (!serializableAdjacency_.empty()) {
        detectSerializableCycles();
    }
    if (hasErrors()) {
        return nullptr;
    }

    // Emit module_init function if needed (e.g., singleton initialization)
    if (!pendingSingletons_.empty()) {
        emitModuleInit();
    }

    if (hasErrors()) {
        return nullptr;
    }

    // Resolve all symbol fixups
    for (const auto& fixup : symbolFixups_) {
        auto* sym = module_->getSymbol(fixup.symbolName);
        if (!sym) continue;

        auto& inst = instructions_[fixup.instructionIndex];
        int32_t wordOffset = (static_cast<int32_t>(sym->value) - static_cast<int32_t>(fixup.instructionByteOffset)) / 4;
        
        auto operands = inst.getOperands();
        if (std::holds_alternative<OperandsJ>(operands)) {
            auto& ops = std::get<OperandsJ>(operands);
            ops.offset = wordOffset;
            inst.setOperands(ops);
        }
    }

    // Finalize instructions
    std::vector<uint8_t> textData = module_->encodeInstructions(instructions_);
    // Append any 16‑bit compressed instructions emitted earlier
    if (!compressedInstructions_.empty()) {
        textData.insert(textData.end(), compressedInstructions_.begin(), compressedInstructions_.end());
        compressedInstructions_.clear();
    }
    Section textSection;
    textSection.name = ".text";
    textSection.type = SectionType::SHT_TEXT;
    textSection.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    textSection.data = std::move(textData);
    textSection.virtual_size = textSection.data.size();
    module_->addSection(std::move(textSection));

    if (moduleUsesNullChecks_) {
        // The module embeds kSysThrowToHandler (SYSCALL 9) after a failed
        // null check; declare the HVM_NZ feature so loaders accept it.
        module_->setRequiredFeatures(static_cast<uint64_t>(hvm::HVMFeature::HVM_NZ));
    }

    return std::make_unique<HVMGeneratedModule>(std::move(module_));
}

std::unique_ptr<GeneratedFunction> HVMCodeGenerator::generateFunction(const ast::FunctionDeclaration& funcDecl) {
    visitFunction(funcDecl);
    return std::make_unique<HVMGeneratedFunction>(0);
}

std::unique_ptr<GeneratedValue> HVMCodeGenerator::generateExpression(const ast::Expression& expr) {
    uint8_t reg = visitExpression(expr);
    return std::make_unique<HVMGeneratedValue>(HVMGeneratedValue::Kind::Register, reg);
}

void HVMCodeGenerator::generateStatement(const ast::Statement& stmt) {
    visitStatement(stmt);
}

std::unique_ptr<GeneratedType> HVMCodeGenerator::generateType(const ast::Type& type) {
    Section* typesSec = module_->getSection(".types");
    if (!typesSec) {
        Section s;
        s.name = ".types";
        s.type = SectionType::SHT_TYPES;
        s.flags = SectionFlags::ALLOC;
        module_->addSection(std::move(s));
        typesSec = module_->getSection(".types");
    }

    std::string typeStr = type.toString();
    for (char c : typeStr) {
        typesSec->data.push_back(static_cast<uint8_t>(c));
    }
    typesSec->data.push_back(0); // Null terminator
    typesSec->virtual_size = typesSec->data.size();

    return std::make_unique<HVMGeneratedType>(0);
}

// ============================================================================
// Internal Visitors
// ============================================================================

HVMCodeGenerator::FunctionPrologueInfo HVMCodeGenerator::beginFunction(
    const ast::FunctionDeclaration* decl,
    const ast::ConstructorDeclaration* ctorDecl,
    bool isMethod, bool isConstructor)
{
    currentFunctionHasReturn_ = false;
    currentFunctionIsAsync_ = decl && decl->isAsync();
    if (decl && decl->isAsync() && decl->getReturnType() &&
        typeIdFromDeclaredType(decl->getReturnType()) != 4 &&
        !dynamic_cast<const ast::FutureType*>(decl->getReturnType())) {
        addError("Async function '" + decl->getName() + "' must return Future<T>");
    }
    FunctionPrologueInfo info;
    info.funcStartOffset = currentByteOffset_;
    info.enterIdx = instructions_.size();
    info.isPrivate = decl && decl->isPrivate();
    bool isFactory = ctorDecl && ctorDecl->isFactory();
    scopeStack_.push_back({});
    emit(Opcode::ENTER, OperandsI{0, 0, 0});

    uint8_t firstArgReg = isMethod ? 2 : 1;
    // Available arg regs: r1,r2,r3,r5,r6,r7,r8 (plain, 7 max) or r2,r3,r5,r6,r7,r8 (method, 6 max)
    uint8_t maxArgRegs = isMethod ? 6 : 7;

    auto mapParams = [&](const auto& params) {
        for (size_t i = 0; i < params.size() && i < maxArgRegs; ++i) {
            std::string paramClassName;
            uint32_t paramTypeId = getTypeId(&params[i]->getType(), nullptr, &paramClassName);
            bool paramNullable = isNullableDeclaredType(&params[i]->getType());
            int32_t offset = reserveLocal(params[i]->getName(), paramTypeId, paramClassName, 0, 0, paramNullable);
            emit(Opcode::ST_D, OperandsI{argReg(firstArgReg, i), 30, static_cast<int16_t>(offset)});
        }
    };

    if (isMethod) {
        int32_t thisOffset = reserveLocal("this", 100, currentClass_ ? currentClass_->name : "");
        emit(Opcode::ST_D, OperandsI{1, 30, static_cast<int16_t>(thisOffset)});
    }

    if (decl) {
        mapParams(decl->getParameters());
        if (decl->isAsync()) {
            uint32_t elemTypeId = 4; // async functions without a declared result are Future<void>
            if (decl->getReturnType()) {
                if (auto futureType = dynamic_cast<const ast::FutureType*>(decl->getReturnType())) {
                    elemTypeId = typeIdFromDeclaredType(&futureType->getElementType());
                }
            }
            uint8_t elemTypeReg = emitConstant(static_cast<int64_t>(elemTypeId));
            emit(Opcode::MOV, OperandsR{1, elemTypeReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_future_new_native_i64");
            freeRegister(elemTypeReg);
            
            asyncFutureOffset_ = reserveLocal("__async_future__", 123, "Future");
            emit(Opcode::ST_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
        }
        visitStatement(decl->getBody());
    } else if (ctorDecl) {
        mapParams(ctorDecl->getParameters());
        visitStatement(ctorDecl->getBody());
    }

    if (decl && decl->getReturnType() && typeIdFromDeclaredType(decl->getReturnType()) != 4 && !currentFunctionHasReturn_) {
        addError("Non-void function '" + decl->getName() + "' has no return statement");
    }

    if (isFactory && !currentFunctionHasReturn_) {
        addError("Factory constructor '" + ctorDecl->getName() + "' must return an instance");
    }

    if (instructions_.empty() || instructions_.back().getOpcode() != Opcode::RET) {
        if (decl && decl->isAsync() && asyncFutureOffset_ != 0) {
            /* A fallthrough async body is a successful Future<void> result. */
            emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
            emit(Opcode::MOV, OperandsR{2, 0, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_future_set_value_native_v_p_p");
            emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
            emit(Opcode::RETAIN, OperandsR{1, 1, 0, 0});
        }
        emitScopeCleanup(scopeStack_.size(), 0);
        emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
        emit(Opcode::RET, OperandsR{0, 0, 0, 0});
    }

    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    if (currentClass_) mp.className = currentClass_->name;
    mp.isConstructor = isConstructor;
    if (isFactory) {
        mp.isFactoryConstructor = true;
        mp.functionName = ctorDecl->getName();
        mp.returnType = "ptr";
    }

    if (decl) {
        mp.functionName = decl->getName();
        if (decl->isPublic()) {
            mp.functionModifiers.push_back("PUBLIC");
        }
        if (decl->isPrivate()) {
            mp.functionModifiers.push_back("PRIVATE");
        }
        if (isMethod && currentClass_) {
            mp.isOverload = isOverloadedMethod_[currentClass_->name][decl->getName()];
        } else {
            mp.isOverload = isOverloadedFunction_[decl->getName()];
        }
        if (isMethod) {
            mp.returnType = "ptr";
        } else if (decl->isAsync()) {
            mp.returnType = "ptr";
        } else if (decl->getReturnType()) {
            mp.returnType = mangleTypeId(typeIdFromDeclaredType(decl->getReturnType()),
                                         isNullableDeclaredType(decl->getReturnType()));
        } else {
            mp.returnType = "void";
        }
    }

    auto addParamTypes = [&](const auto& params) {
        for (const auto& param : params) {
            if (mp.isOverload) {
                mp.parameterTypes.push_back(mangleTypeId(typeIdFromDeclaredType(&param->getType(), nullptr),
                                                         isNullableDeclaredType(&param->getType())));
            } else {
                mp.parameterTypes.push_back("ptr");
            }
        }
    };

    if (decl) {
        addParamTypes(decl->getParameters());
    } else if (ctorDecl) {
        addParamTypes(ctorDecl->getParameters());
    }

    bool shouldMangle = !modulePath_.empty() || currentClass_ != nullptr || mp.isOverload;
    if (isFactory) {
        info.mangledName = shouldMangle ? SymbolMangler::mangleFunctionName(mp) : ctorDecl->getName();
    } else if (isConstructor) {
        info.mangledName = shouldMangle ? SymbolMangler::mangleFunctionName(mp) : "constructor";
    } else if (decl) {
        info.mangledName = shouldMangle ? SymbolMangler::mangleFunctionName(mp) : decl->getName();
    }

    return info;
}

static bool isArcManagedTypeId(uint32_t typeId) {
    // Types that participate in ARC and should be released by scope cleanup.
    // Non-ARC types manage their own lifecycle via explicit _release() methods
    // using free/delete, not hoo_release, so they must be excluded from scope cleanup.
    // Unknown/default (100) is excluded because it may be assigned to raw pointers
    // or primitive values that cannot be passed to hoo_release.
    switch (typeId) {
        case 100: // Unknown - could be raw pointer, int64, etc.
        case 110: // Args - uses calloc and dedicated args_release cleanup
        case 111: // Compression - uses std::free
        case 120: // Regex - uses delete with custom refcounting
        case 121: // Mutex - uses delete
        case 122: // Uuid - uses std::free with custom refcounting
        case 128: // Condition - explicit destroy
        case 129: // Semaphore - explicit destroy
            return false;
        default:
            return typeId >= 100;
    }
}

// Named reference types whose instances are allocated with hoo_alloc and must
// be released with hoo_release (_F_hoo_release_v_p). Types that manage their
// own lifecycle (Args/Compression/Regex/Mutex/Uuid) or require a dedicated
// release helper (Exception -> hoo_exception_release) are excluded.
static bool isHooReleaseManagedClassName(const std::string& className) {
    static const std::unordered_set<std::string> nonArcClasses = {
        "Args", "Compression", "Regex", "Mutex", "Uuid", "Exception",
        "Condition", "Semaphore",
    };
    return !className.empty() && nonArcClasses.count(className) == 0;
}

void HVMCodeGenerator::emitScopeCleanup(size_t from, size_t to) {
    uint8_t saveReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{saveReg, 1, 0, 0});
    for (size_t i = from; i > to; --i) {
        auto& scope = scopeStack_[i - 1];
        for (const auto& [name, local] : scope) {
            if (local.arcManaged && !local.explicitlyReleased) {
                emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(local.offset)});
                emitCall(Opcode::CALL, local.cleanupSymbol.empty()
                    ? "_F_hoo_release_v_p" : local.cleanupSymbol);
            }
        }
    }
    emit(Opcode::MOV, OperandsR{1, saveReg, 0, 0});
    freeRegister(saveReg);
}

bool HVMCodeGenerator::isManagedTemporary(const ast::Expression& expr) {
    const ast::ASTNode* current = &expr;
    while (true) {
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(current)) {
            current = &pe->getPrimary();
        } else if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(current)) {
            current = &paren->getExpression();
        } else {
            break;
        }
    }
    if (dynamic_cast<const ast::StringLiteral*>(current)) return true;
    if (dynamic_cast<const ast::InterpolatedString*>(current)) return true;
    if (dynamic_cast<const ast::NewObjectExpression*>(current)) return true;
    if (dynamic_cast<const ast::NewDictExpression*>(current)) return true;
    if (dynamic_cast<const ast::ArrayLiteral*>(current)) return true;
    if (dynamic_cast<const ast::TensorLiteral*>(current)) return true;
    return false;
}

void HVMCodeGenerator::endFunction(const FunctionPrologueInfo& info) {
    int32_t frameSize = alignFrameSize(-currentStackOffset_);
    instructions_[info.enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    Symbol sym;
    sym.name = info.mangledName;
    sym.value = info.funcStartOffset;
    sym.type = Symbol::STT_FUNC;
    sym.binding = info.isPrivate ? Symbol::STB_LOCAL : Symbol::STB_GLOBAL;
    sym.section_index = 0;
    module_->addSymbol(sym);

    scopeStack_.clear();
    currentStackOffset_ = 0;
}

void HVMCodeGenerator::visitFunction(const ast::FunctionDeclaration& decl) {
    // Populate function return type inference info before processing body
    if (decl.isAsync()) {
        functionReturnTypes_[decl.getName()] = 123;
        functionReturnClass_[decl.getName()] = "Future";
        uint32_t elementType = 4;
        if (auto futureType = dynamic_cast<const ast::FutureType*>(decl.getReturnType())) {
            elementType = typeIdFromDeclaredType(&futureType->getElementType());
        }
        functionFutureElementTypes_[decl.getName()] = elementType;
    } else if (decl.getReturnType()) {
        std::string clsName;
        functionReturnTypes_[decl.getName()] = typeIdFromDeclaredType(decl.getReturnType(), &clsName);
        if (!clsName.empty()) {
            functionReturnClass_[decl.getName()] = clsName;
        }
    } else {
        functionReturnTypes_[decl.getName()] = 4; // void
    }
    auto info = beginFunction(&decl, nullptr, false, false);
    endFunction(info);
}

void HVMCodeGenerator::visitConstructor(const ast::ConstructorDeclaration& decl) {
    // Factory constructors have no 'this' (isMethod=false) and do not run the
    // generative constructor path: inConstructor_ stays false so immutable
    // field writes are rejected and 'this' is unbound.
    if (decl.isFactory()) {
        auto info = beginFunction(nullptr, &decl, false, false);
        endFunction(info);
        return;
    }
    inConstructor_ = true;
    auto info = beginFunction(nullptr, &decl, true, true);
    endFunction(info);
    inConstructor_ = false;
}

void HVMCodeGenerator::validateSerializableClass(
    const ast::ClassDeclaration& classDecl,
    const ClassLayout& layout,
    const std::string& name)
{
    // Phase 3: Field count validation — at least one public field
    bool hasPublicField = false;
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto declMember = member->getDeclaration()) {
            if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                if (var->isPublic()) {
                    hasPublicField = true;
                    break;
                }
            }
        }
    }
    if (!hasPublicField) {
        addError("Serializable class '" + name + "' must have at least one public field");
        return;
    }

    // Phase 3: Constructor validation — exactly one generative constructor with
    // zero parameters. Named factory constructors are not counted.
    int ctorCount = 0;
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto ctor = member->getConstructor()) {
            if (ctor->isFactory()) continue;
            ++ctorCount;
            if (!ctor->getParameters().empty()) {
                addError("Serializable class '" + name + "' constructor must have no parameters");
            }
        }
    }
    if (ctorCount == 0) {
        addError("Serializable class '" + name + "' must have exactly one constructor");
    } else if (ctorCount > 1) {
        addError("Serializable class '" + name + "' must have exactly one constructor");
    }

    // Phase 2: Field type validation — verify each public field type is allowed
    // Also build adjacency for cycle detection
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto declMember = member->getDeclaration()) {
            if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                if (!var->isPublic()) continue;
                const ast::Type* type = var->getType();
                if (!type) {
                    addError("Serializable class '" + name + "' field '" + var->getName() + "' must have an explicit type");
                    continue;
                }
                if (!isValidSerializableType(*type, name, var->getName())) {
                    addError("Serializable class '" + name + "' field '" + var->getName() + "' has unsupported type for serialization");
                }
                // Record adjacency for serializable class references
                if (auto bt = dynamic_cast<const ast::BaseType*>(type)) {
                    if (!bt->isPrimitive()) {
                        std::string typeName = bt->getIdentifier();
                        auto it = classes_.find(typeName);
                        if (it != classes_.end() && it->second.isSerializable) {
                            serializableAdjacency_[name].push_back(typeName);
                        }
                    }
                }
            }
        }
    }
}

bool HVMCodeGenerator::isValidSerializableType(
    const ast::Type& type,
    const std::string& className,
    const std::string& fieldName)
{
    // Check for DictType
    if (auto hmType = dynamic_cast<const ast::DictType*>(&type)) {
        const ast::Type& valueType = hmType->getValueType();
        // Value type must be a restricted primitive (no float, char, or serializable class)
        if (auto bt = dynamic_cast<const ast::BaseType*>(&valueType)) {
            if (bt->isPrimitive()) {
                auto kind = bt->getPrimitiveType()->getKind();
                // Allowed: int8, byte, int64, double, f64, f8, string, bool, bit, buffer
                // Rejected: float, char
                switch (kind) {
                    case ast::PrimitiveTypeKind::INT8:
                    case ast::PrimitiveTypeKind::BYTE:
                    case ast::PrimitiveTypeKind::INT64:
                    case ast::PrimitiveTypeKind::DOUBLE:
                    case ast::PrimitiveTypeKind::F64:
                    case ast::PrimitiveTypeKind::F8:
                    case ast::PrimitiveTypeKind::STRING:
                    case ast::PrimitiveTypeKind::BOOL:
                    case ast::PrimitiveTypeKind::BIT:
                    case ast::PrimitiveTypeKind::BUFFER:
                        return true;
                    case ast::PrimitiveTypeKind::FLOAT:
                        addError("Serializable class '" + className + "' field '" + fieldName + "': float not allowed as Dict value type");
                        return false;
                    case ast::PrimitiveTypeKind::CHAR:
                        addError("Serializable class '" + className + "' field '" + fieldName + "': char not allowed as Dict value type");
                        return false;
                    default:
                        return false;
                }
            }
            // Check if it's a BaseType referencing a class name
            if (bt->getIdentifier() == "String" || bt->getIdentifier() == "string") return true;
            if (bt->getIdentifier() == "Buffer" || bt->getIdentifier() == "buffer") return true;
            // Serializable class as Dict value is NOT allowed
            addError("Serializable class '" + className + "' field '" + fieldName + "': serializable class not allowed as Dict value type");
            return false;
        }
        if (dynamic_cast<const ast::TensorType*>(&valueType)) {
            return true;
        }
        addError("Serializable class '" + className + "' field '" + fieldName + "': unsupported Dict value type");
        return false;
    }

    // Check for ListType
    if (dynamic_cast<const ast::ListType*>(&type)) {
        return true;
    }

    // Check for TensorType
    if (auto tensorType = dynamic_cast<const ast::TensorType*>(&type)) {
        const ast::BaseType& elemType = tensorType->getElementType();
        if (elemType.isPrimitive()) {
            auto kind = elemType.getPrimitiveType()->getKind();
            switch (kind) {
                case ast::PrimitiveTypeKind::INT8:
                case ast::PrimitiveTypeKind::BYTE:
                case ast::PrimitiveTypeKind::INT64:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:
                case ast::PrimitiveTypeKind::F8:
                case ast::PrimitiveTypeKind::BOOL:
                case ast::PrimitiveTypeKind::BIT:
                case ast::PrimitiveTypeKind::BUFFER:
                    return true;
                case ast::PrimitiveTypeKind::FLOAT:
                case ast::PrimitiveTypeKind::CHAR:
                case ast::PrimitiveTypeKind::STRING:
                default:
                    return false;
            }
        }
        return false;
    }

    // Check for BaseType (primitives or class references)
    if (auto bt = dynamic_cast<const ast::BaseType*>(&type)) {
        if (bt->isPrimitive()) {
            auto kind = bt->getPrimitiveType()->getKind();
            switch (kind) {
                case ast::PrimitiveTypeKind::INT8:
                case ast::PrimitiveTypeKind::BYTE:
                case ast::PrimitiveTypeKind::INT64:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:
                case ast::PrimitiveTypeKind::F8:
                case ast::PrimitiveTypeKind::STRING:
                case ast::PrimitiveTypeKind::BOOL:
                case ast::PrimitiveTypeKind::BIT:
                case ast::PrimitiveTypeKind::BUFFER:
                    return true;
                case ast::PrimitiveTypeKind::FLOAT:
                    addError("Serializable class '" + className + "' field '" + fieldName + "': float not allowed");
                    return false;
                case ast::PrimitiveTypeKind::CHAR:
                    addError("Serializable class '" + className + "' field '" + fieldName + "': char not allowed");
                    return false;
                default:
                    return false;
            }
        }
        // Class reference — must be a serializable class
        std::string typeName = bt->getIdentifier();
        auto it = classes_.find(typeName);
        if (it != classes_.end() && it->second.isSerializable) {
            return true;
        }
        // Also check common wrapper types
        if (typeName == "String" || typeName == "Buffer" || typeName == "List") {
            return true;
        }
        addError("Serializable class '" + className + "' field '" + fieldName + "' references non-serializable class '" + typeName + "'");
        return false;
    }

    // ArrayType — not allowed
    if (dynamic_cast<const ast::ArrayType*>(&type)) {
        addError("Serializable class '" + className + "' field '" + fieldName + "': Array type not allowed for serialization");
        return false;
    }

    // MapType — not allowed
    if (dynamic_cast<const ast::MapType*>(&type)) {
        addError("Serializable class '" + className + "' field '" + fieldName + "': Map type not allowed for serialization (use Dict)");
        return false;
    }

    return false;
}

void HVMCodeGenerator::detectSerializableCycles() {
    enum class Color { WHITE, GRAY, BLACK };
    std::unordered_map<std::string, Color> color;
    std::vector<std::string> path;

    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        color[node] = Color::GRAY;
        path.push_back(node);
        auto adjIt = serializableAdjacency_.find(node);
        if (adjIt != serializableAdjacency_.end()) {
            for (const auto& neighbor : adjIt->second) {
                auto colorIt = color.find(neighbor);
                if (colorIt != color.end() && colorIt->second == Color::GRAY) {
                    // Cycle found: from neighbor to node back to neighbor
                    std::string cyclePath;
                    bool inCycle = false;
                    for (const auto& p : path) {
                        if (p == neighbor) inCycle = true;
                        if (inCycle) {
                            if (!cyclePath.empty()) cyclePath += " -> ";
                            cyclePath += p;
                        }
                    }
                    cyclePath += " -> " + neighbor;
                    addError("Serializable class '" + node + "' forms a cycle: " + cyclePath);
                    path.pop_back();
                    color[node] = Color::BLACK;
                    return true;
                } else if (colorIt == color.end() || colorIt->second == Color::WHITE) {
                    dfs(neighbor);
                }
            }
        }
        path.pop_back();
        color[node] = Color::BLACK;
        return false;
    };

    for (const auto& [className, _] : serializableAdjacency_) {
        if (color[className] == Color::WHITE) {
            dfs(className);
        }
    }
}

void HVMCodeGenerator::visitMethod(const ast::FunctionDeclaration& decl) {
    auto info = beginFunction(&decl, nullptr, true, false);
    endFunction(info);
}

void HVMCodeGenerator::visitStatement(const ast::Statement& stmt) {
    if (auto block = dynamic_cast<const ast::Block*>(&stmt)) {
        scopeStack_.push_back({});
        for (const auto& s : block->getStatements()) {
            visitStatement(*s);
        }
        emitScopeCleanup(scopeStack_.size(), scopeStack_.size() - 1);
        scopeStack_.pop_back();
    } else if (auto scope = dynamic_cast<const ast::ScopeStatement*>(&stmt)) {
        // An explicit scope is a real lifetime boundary. Reuse the existing
        // block lowering so ARC cleanup and break/continue cleanup remain
        // identical to ordinary nested blocks.
        visitStatement(scope->getBody());
    } else if (auto ret = dynamic_cast<const ast::ReturnStatement*>(&stmt)) {
        currentFunctionHasReturn_ = true;
        if (currentFunctionIsAsync_ && asyncFutureOffset_ != 0) {
            if (ret->hasExpression()) {
                uint8_t reg = visitExpression(*ret->getExpression());
                emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
                emit(Opcode::MOV, OperandsR{2, reg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_future_set_value_native_v_p_p");
                if (isArcManagedTypeId(inferExpressionTypeId(*ret->getExpression()))) {
                    emit(Opcode::RELEASE, OperandsR{reg, 0, 0, 0});
                }
                freeRegister(reg);
            } else {
                emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
                emit(Opcode::MOV, OperandsR{2, 0, 0, 0}); // NULL
                emitCall(Opcode::CALL, "_F_hoo_future_set_value_native_v_p_p");
            }
            emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
            /* Transfer the return Future to the caller before local cleanup. */
            emit(Opcode::RETAIN, OperandsR{1, 1, 0, 0});
            emitScopeCleanup(scopeStack_.size(), 0);
            emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
            emit(Opcode::RET, OperandsR{0, 0, 0, 0});
        } else {
            if (ret->hasExpression()) {
                uint8_t reg = visitExpression(*ret->getExpression());
                emit(Opcode::MOV, OperandsR{1, reg, 0, 0});
                emit(Opcode::RETAIN, OperandsR{1, 1, 0, 0}); // ARC retain for return value
                freeRegister(reg);
            }
            emitScopeCleanup(scopeStack_.size(), 0);
            emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
            emit(Opcode::RET, OperandsR{0, 0, 0, 0});
        }
    } else if (auto varDecl = dynamic_cast<const ast::VariableDeclarationStatement*>(&stmt)) {
        auto& decl = varDecl->getDeclaration();
        std::string varClassName;
        uint32_t typeId = getTypeId(decl.getType(), decl.getInitializer(), &varClassName);
        uint32_t elemTypeId = 0;
        uint32_t keyTypeId = 0;
        if (!decl.getType() && decl.getInitializer()) {
            const auto inferred = inferExpressionTypeInfo(*decl.getInitializer());
            if (varClassName.empty()) varClassName = inferred.className;
            elemTypeId = inferred.elementTypeId;
            keyTypeId = inferred.keyTypeId;
        }
        if (decl.getType()) {
            if (auto arrType = dynamic_cast<const ast::ArrayType*>(decl.getType())) {
                elemTypeId = typeIdFromDeclaredType(&arrType->getBaseType());
            } else if (auto tensorType = dynamic_cast<const ast::TensorType*>(decl.getType())) {
                elemTypeId = tensorElementTypeIdFromType(*tensorType);
            } else if (auto hashMapType = dynamic_cast<const ast::DictType*>(decl.getType())) {
                keyTypeId = dictKeyTypeId(*hashMapType);
                elemTypeId = typeIdFromDeclaredType(&hashMapType->getValueType());
            } else if (auto mapType = dynamic_cast<const ast::MapType*>(decl.getType())) {
                keyTypeId = mapKeyTypeId(*mapType);
                elemTypeId = typeIdFromDeclaredType(&mapType->getValueType());
            } else if (dynamic_cast<const ast::ListType*>(decl.getType())) {
                elemTypeId = 0;
            } else if (auto futureType = dynamic_cast<const ast::FutureType*>(decl.getType())) {
                elemTypeId = typeIdFromDeclaredType(&futureType->getElementType());
            }
        } else if (decl.getInitializer()) {
            if (auto newHash = dynamic_cast<const ast::NewDictExpression*>(decl.getInitializer())) {
                keyTypeId = dictKeyTypeId(newHash->getDictType());
                elemTypeId = typeIdFromDeclaredType(&newHash->getDictType().getValueType());
            } else if (auto newObj = dynamic_cast<const ast::NewObjectExpression*>(decl.getInitializer())) {
                if (newObj->getClassName() == "Map") {
                    keyTypeId = mapConstructorKeyTypeId(*newObj);
                    elemTypeId = mapConstructorValueTypeId(*newObj);
                }
            } else if (auto arrLit = dynamic_cast<const ast::ArrayLiteral*>(decl.getInitializer())) {
                auto& elements = arrLit->getElements()->getExpressions();
                uint32_t commonType = 100;
                for (const auto& elem : elements) {
                    uint32_t t = getTypeId(nullptr, elem.get());
                    if (t != 100) {
                        if (commonType == 100) commonType = t;
                        else if (commonType != t) { commonType = 100; break; }
                    }
                }
                if (arrLit->isList()) {
                    elemTypeId = (commonType == 100) ? 0 : commonType;
                } else {
                    elemTypeId = commonType;
                }
            } else if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(decl.getInitializer())) {
                if (auto tensorLit = dynamic_cast<const ast::TensorLiteral*>(&pe->getPrimary())) {
                    elemTypeId = tensorElementTypeIdFromLiteral(*tensorLit);
                } else if (auto arrLit = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
                    if (arrLit->isList()) {
                        uint32_t commonType = 100;
                        if (arrLit->getElements()) {
                            for (const auto& elem : arrLit->getElements()->getExpressions()) {
                                uint32_t t = getTypeId(nullptr, elem.get());
                                if (t != 100) {
                                    if (commonType == 100) commonType = t;
                                    else if (commonType != t) { commonType = 100; break; }
                                }
                            }
                        }
                        elemTypeId = (commonType == 100) ? 0 : commonType;
                    }
                }
            } else if (auto binExpr = dynamic_cast<const ast::BinaryExpression*>(decl.getInitializer())) {
                auto inferTensorElemType = [&](const ast::Expression& operand) -> uint32_t {
                    if (auto pe2 = dynamic_cast<const ast::PrimaryExpression*>(&operand)) {
                        if (auto id2 = dynamic_cast<const ast::Identifier*>(&pe2->getPrimary())) {
                            if (getLocalTypeId(id2->getName()) == 104) {
                                return getLocalElementTypeId(id2->getName());
                            }
                        }
                    }
                    return 100;
                };
                uint32_t leftElem = inferTensorElemType(binExpr->getLeft());
                uint32_t rightElem = inferTensorElemType(binExpr->getRight());
                if (leftElem != 100) elemTypeId = leftElem;
                else if (rightElem != 100) elemTypeId = rightElem;
            } else if (auto logicAnd = dynamic_cast<const ast::LogicalAnd*>(decl.getInitializer())) {
                auto inferElem = [&](const ast::Expression& operand) -> uint32_t {
                    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&operand)) {
                        if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                            if (getLocalTypeId(id->getName()) == 104) {
                                return getLocalElementTypeId(id->getName());
                            }
                        }
                    }
                    return 100;
                };
                uint32_t leftElem = inferElem(logicAnd->getLeft());
                uint32_t rightElem = inferElem(logicAnd->getRight());
                if (leftElem != 100) elemTypeId = leftElem;
                else if (rightElem != 100) elemTypeId = rightElem;
            } else if (auto logicOr = dynamic_cast<const ast::LogicalOr*>(decl.getInitializer())) {
                auto inferElem = [&](const ast::Expression& operand) -> uint32_t {
                    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&operand)) {
                        if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                            if (getLocalTypeId(id->getName()) == 104) {
                                return getLocalElementTypeId(id->getName());
                            }
                        }
                    }
                    return 100;
                };
                uint32_t leftElem = inferElem(logicOr->getLeft());
                uint32_t rightElem = inferElem(logicOr->getRight());
                if (leftElem != 100) elemTypeId = leftElem;
                else if (rightElem != 100) elemTypeId = rightElem;
            }
        }
        bool declNullable = isNullableDeclaredType(decl.getType());
        if (decl.getInitializer()) {
            validateAssignmentNullSafety(declNullable, typeId, decl.getInitializer(), decl.getName());
        }
        int32_t offset = reserveLocal(decl.getName(), typeId, varClassName, elemTypeId, keyTypeId, declNullable);
        if (typeId == 110 && decl.getInitializer()) {
            auto& local = scopeStack_.back()[decl.getName()];
            local.arcManaged = true;
            local.cleanupSymbol = "_F_M_hoo_E_args_release_v";
        }
        if (decl.getInitializer()) {
            // Set decimal context from declared type if applicable
            int32_t savedPrec = currentDecimalPrecision_;
            int32_t savedScale = currentDecimalScale_;
            if (decl.getType()) {
                if (auto decType = dynamic_cast<const ast::DecimalType*>(decl.getType())) {
                    currentDecimalPrecision_ = decType->getPrecision();
                    currentDecimalScale_ = decType->getScale();
                }
            }
            uint8_t reg = visitExpression(*decl.getInitializer());
            currentDecimalPrecision_ = savedPrec;
            currentDecimalScale_ = savedScale;
            emit(Opcode::ST_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            freeRegister(reg);
        } else if (auto tensorType = dynamic_cast<const ast::TensorType*>(decl.getType())) {
            uint32_t elemType = tensorElementTypeIdFromType(*tensorType);
            const auto& dims = tensorType->getDimensions();
            uint8_t reg = emitTensorNewCall(elemType, dims.size(),
                [this, &dims](size_t i) -> uint8_t { return visitExpression(*dims[i]); });
            emit(Opcode::ST_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            freeRegister(reg);
        }
    } else if (auto exprStmt = dynamic_cast<const ast::ExpressionStatement*>(&stmt)) {
        uint8_t reg = visitExpression(exprStmt->getExpression());
        if (isManagedTemporary(exprStmt->getExpression())) {
            emit(Opcode::MOV, OperandsR{1, reg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_release_v_p");
        }
        freeRegister(reg);
    } else if (auto ifStmt = dynamic_cast<const ast::IfStatement*>(&stmt)) {
        Label* elseLabel = createLabel();
        Label* endLabel = createLabel();
        uint8_t condReg = visitExpression(ifStmt->getCondition());
        emitBranch(Opcode::BEQ, condReg, 0, elseLabel);
        freeRegister(condReg);
        visitStatement(ifStmt->getThenBlock());
        emitJump(Opcode::JMP, 0, endLabel);
        bindLabel(elseLabel);
        if (ifStmt->hasElse()) {
            visitStatement(*ifStmt->getElseBlock());
        }
        bindLabel(endLabel);
    } else if (auto whileStmt = dynamic_cast<const ast::WhileStatement*>(&stmt)) {
        Label* startLabel = createLabel();
        Label* endLabel = createLabel();
        RegisterMask loopExitMask = captureRegisterMask();
        bindLabel(startLabel);
        uint8_t condReg = visitExpression(whileStmt->getCondition());
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(condReg);
        RegisterMask loopBodyMask = captureRegisterMask();
        controlFlowStack_.push({endLabel, startLabel, scopeStack_.size(), loopExitMask, loopBodyMask});
        visitStatement(whileStmt->getBody());
        restoreRegisterMask(loopBodyMask);
        controlFlowStack_.pop();
        emitJump(Opcode::JMP, 0, startLabel);
        bindLabel(endLabel);
        restoreRegisterMask(loopExitMask);
    } else if (auto doWhile = dynamic_cast<const ast::DoWhileStatement*>(&stmt)) {
        Label* startLabel = createLabel();
        Label* conditionLabel = createLabel();
        Label* endLabel = createLabel();
        RegisterMask loopExitMask = captureRegisterMask();
        bindLabel(startLabel);
        RegisterMask loopBodyMask = captureRegisterMask();
        controlFlowStack_.push({endLabel, conditionLabel, scopeStack_.size(), loopExitMask, loopBodyMask});
        visitStatement(doWhile->getBody());
        restoreRegisterMask(loopBodyMask);
        controlFlowStack_.pop();
        bindLabel(conditionLabel);
        uint8_t condReg = visitExpression(doWhile->getCondition());
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(condReg);
        emitJump(Opcode::JMP, 0, startLabel);
        bindLabel(endLabel);
        restoreRegisterMask(loopExitMask);
    } else if (auto switchStmt = dynamic_cast<const ast::SwitchStatement*>(&stmt)) {
        auto supportsSwitchCompare = [](uint32_t typeId) {
            return typeId == 0 || typeId == 1 || typeId == 3 || typeId == 5 ||
                   typeId == 6 || typeId == 7 || typeId == 8;
        };
        uint32_t discriminantType = inferExpressionTypeId(switchStmt->getDiscriminant());
        if (!supportsSwitchCompare(discriminantType)) {
            addError("switch only supports integer-like discriminants");
        }
        uint8_t discReg = visitExpression(switchStmt->getDiscriminant());
        Label* endLabel = createLabel();
        std::vector<Label*> caseLabels;
        for (size_t i = 0; i < switchStmt->getCases().size(); i++) {
            caseLabels.push_back(createLabel());
        }
        Label* defaultLabel = createLabel();

        for (size_t i = 0; i < switchStmt->getCases().size(); i++) {
            const auto& caseClause = switchStmt->getCases()[i];
            uint32_t caseType = inferExpressionTypeId(*caseClause.value);
            if (!supportsSwitchCompare(caseType)) {
                addError("switch case values must be integer-like");
            }
            uint8_t valReg = visitExpression(*caseClause.value);
            uint8_t cmpReg = allocateRegister();
            emit(Opcode::CMP, OperandsR{cmpReg, discReg, valReg, 0}); // eq
            freeRegister(valReg);
            emitBranch(Opcode::BNE, cmpReg, 0, caseLabels[i]); // if match, jump to case body
            freeRegister(cmpReg);
        }

        if (switchStmt->hasDefault()) {
            emitJump(Opcode::JMP, 0, defaultLabel);
        } else {
            emitJump(Opcode::JMP, 0, endLabel);
        }

        RegisterMask switchBodyMask = captureRegisterMask();
        RegisterMask switchExitMask = switchBodyMask;
        if (discReg >= 9 && discReg <= 20) {
            switchExitMask.set(discReg, false);
        }
        controlFlowStack_.push({endLabel, nullptr, scopeStack_.size(), switchBodyMask, switchBodyMask});

        for (size_t i = 0; i < switchStmt->getCases().size(); i++) {
            bindLabel(caseLabels[i]);
            const auto& caseClause = switchStmt->getCases()[i];
            for (const auto& s : caseClause.statements) {
                visitStatement(*s);
            }
        }

        if (switchStmt->hasDefault()) {
            bindLabel(defaultLabel);
            for (const auto& s : switchStmt->getDefaultStatements()) {
                visitStatement(*s);
            }
        }

        restoreRegisterMask(switchBodyMask);
        controlFlowStack_.pop();
        bindLabel(endLabel);
        freeRegister(discReg);
        restoreRegisterMask(switchExitMask);
    } else if (auto breakStmt = dynamic_cast<const ast::BreakStatement*>(&stmt)) {
        if (controlFlowStack_.empty() || !controlFlowStack_.top().breakLabel) {
            addError("break statement outside of loop");
        } else {
            restoreRegisterMask(controlFlowStack_.top().breakRegisterMask);
            emitScopeCleanup(scopeStack_.size(), controlFlowStack_.top().scopeDepth);
            emitJump(Opcode::JMP, 0, controlFlowStack_.top().breakLabel);
        }
    } else if (auto continueStmt = dynamic_cast<const ast::ContinueStatement*>(&stmt)) {
        Label* continueLabel = nullptr;
        size_t continueScopeDepth = 0;
        RegisterMask continueRegisterMask;
        auto scopes = controlFlowStack_;
        while (!scopes.empty()) {
            if (scopes.top().continueLabel) {
                continueLabel = scopes.top().continueLabel;
                continueScopeDepth = scopes.top().scopeDepth;
                continueRegisterMask = scopes.top().continueRegisterMask;
                break;
            }
            scopes.pop();
        }
        if (!continueLabel) {
            addError("continue statement outside of loop");
        } else {
            restoreRegisterMask(continueRegisterMask);
            emitScopeCleanup(scopeStack_.size(), continueScopeDepth);
            emitJump(Opcode::JMP, 0, continueLabel);
        }
    } else if (auto forRange = dynamic_cast<const ast::ForRangeStatement*>(&stmt)) {
        RegisterMask loopExitMask = captureRegisterMask();
        int32_t offset = reserveLocal(forRange->getVariable(), 1);
        bool isStepOne = true;
        if (forRange->getStep()) {
            isStepOne = false;
            if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(forRange->getStep())) {
                if (auto il = dynamic_cast<const ast::IntegerLiteral*>(&pe->getPrimary())) {
                    if (il->getValue() == 1) {
                        isStepOne = true;
                    }
                }
            }
        }

        if (isStepOne) {
            uint8_t startReg = visitExpression(forRange->getStart());
            uint8_t endReg = visitExpression(forRange->getEnd());
            uint8_t countReg = allocateRegister();
            emit(Opcode::ARITH, OperandsR{countReg, endReg, startReg, 1}); // sub
            
            Label* endLabel = createLabel();
            uint8_t zeroReg = emitConstant(0);
            uint8_t condReg = allocateRegister();
            emit(Opcode::CMP, OperandsR{condReg, zeroReg, countReg, 2}); // lt (0 < count)
            freeRegister(zeroReg);
            emitBranch(Opcode::BEQ, condReg, 0, endLabel);
            freeRegister(condReg);
            
            emit(Opcode::ST_D, OperandsI{startReg, 30, static_cast<int16_t>(offset)});
            freeRegister(startReg);
            freeRegister(endReg);
            
            size_t loopSetIdx = instructions_.size();
            emit(Opcode::LOOP_SET, OperandsI{0, countReg, 0});
            freeRegister(countReg);
            
            Label* startLabel = createLabel();
            Label* stepLabel = createLabel();
            bindLabel(startLabel);
            RegisterMask loopBodyMask = captureRegisterMask();
            controlFlowStack_.push({endLabel, stepLabel, scopeStack_.size(), loopExitMask, loopBodyMask});
            visitStatement(forRange->getBody());
            restoreRegisterMask(loopBodyMask);
            controlFlowStack_.pop();
            
            bindLabel(stepLabel);
            uint8_t iReg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{iReg, 30, static_cast<int16_t>(offset)});
            uint8_t oneReg = emitConstant(1);
            uint8_t nextIReg = allocateRegister();
            emit(Opcode::ARITH, OperandsR{nextIReg, iReg, oneReg, 0}); // add
            emit(Opcode::ST_D, OperandsI{nextIReg, 30, static_cast<int16_t>(offset)});
            freeRegister(iReg);
            freeRegister(oneReg);
            freeRegister(nextIReg);
            
            uint32_t loopDecbrOffset = currentByteOffset_;
            emitBranch(Opcode::LOOP_DECBR, 0, 0, startLabel);
            
            auto& loopSetInst = instructions_[loopSetIdx];
            auto ops = std::get<OperandsI>(loopSetInst.getOperands());
            int32_t byteOffset = startLabel->targetByteOffset - static_cast<int32_t>(loopDecbrOffset);
            int32_t wordOffset = byteOffset / 4;
            ops.imm15 = static_cast<int16_t>(wordOffset);
            loopSetInst.setOperands(ops);
            
            bindLabel(endLabel);
            restoreRegisterMask(loopExitMask);
        } else {
            uint8_t startReg = visitExpression(forRange->getStart());
            emit(Opcode::ST_D, OperandsI{startReg, 30, static_cast<int16_t>(offset)});
            freeRegister(startReg);
            Label* startLabel = createLabel();
            Label* endLabel = createLabel();
            Label* stepLabel = createLabel();
            bindLabel(startLabel);
            uint8_t iReg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{iReg, 30, static_cast<int16_t>(offset)});
            uint8_t endReg = visitExpression(forRange->getEnd());
            uint8_t condReg = allocateRegister();
            emit(Opcode::CMP, OperandsR{condReg, iReg, endReg, 2});
            emitBranch(Opcode::BEQ, condReg, 0, endLabel);
            freeRegister(iReg);
            freeRegister(endReg);
            freeRegister(condReg);
            RegisterMask loopBodyMask = captureRegisterMask();
            controlFlowStack_.push({endLabel, stepLabel, scopeStack_.size(), loopExitMask, loopBodyMask});
            visitStatement(forRange->getBody());
            restoreRegisterMask(loopBodyMask);
            controlFlowStack_.pop();
            bindLabel(stepLabel);
            iReg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{iReg, 30, static_cast<int16_t>(offset)});
            uint8_t stepReg = visitExpression(*forRange->getStep());
            uint8_t nextIReg = allocateRegister();
            emit(Opcode::ARITH, OperandsR{nextIReg, iReg, stepReg, 0});
            emit(Opcode::ST_D, OperandsI{nextIReg, 30, static_cast<int16_t>(offset)});
            freeRegister(iReg);
            freeRegister(stepReg);
            freeRegister(nextIReg);
            emitJump(Opcode::JMP, 0, startLabel);
            bindLabel(endLabel);
            restoreRegisterMask(loopExitMask);
        }
    } else if (auto forIn = dynamic_cast<const ast::ForInStatement*>(&stmt)) {
        RegisterMask loopExitMask = captureRegisterMask();
        uint8_t iterReg = visitExpression(forIn->getIterable());
        
        // Convert string to character array, or map to key array
        uint32_t forInTypeId = inferExpressionTypeId(forIn->getIterable());
        if (forInTypeId == 101) { // String
            uint8_t oldReg = iterReg;
            emit(Opcode::MOV, OperandsR{1, iterReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_String_to_characters_p_p");
            iterReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{iterReg, 1, 0, 0});
            freeRegister(oldReg);
        } else if (forInTypeId == 103) { // Map
            uint32_t mapKeyTypeId = 0;
            if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&forIn->getIterable())) {
                if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                    mapKeyTypeId = getLocalKeyTypeId(id->getName());
                }
            }
            uint8_t oldReg = iterReg;
            emit(Opcode::MOV, OperandsR{1, iterReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Map_keys_p");
            iterReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{iterReg, 1, 0, 0});
            freeRegister(oldReg);
        }

        // Lowered: Get length via runtime call
        uint8_t lenReg = allocateRegister();
        emit(Opcode::MOV, OperandsR{1, iterReg, 0, 0});
        emitCall(Opcode::CALL, "_F_array_length_v_p");
        emit(Opcode::MOV, OperandsR{lenReg, 1, 0, 0});
        
        Label* endLabel = createLabel();
        
        // Check if length <= 0, if so, jump to endLabel
        uint8_t zeroReg = emitConstant(0);
        uint8_t condReg = allocateRegister();
        emit(Opcode::CMP, OperandsR{condReg, zeroReg, lenReg, 2}); // lt (0 < lenReg)
        freeRegister(zeroReg);
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(condReg);
        
        // Loop counter setup
        uint8_t iReg = emitConstant(0);
        
        // Emit LOOP_SET
        size_t loopSetIdx = instructions_.size();
        emit(Opcode::LOOP_SET, OperandsI{0, lenReg, 0});
        
        Label* startLabel = createLabel();
        Label* stepLabel = createLabel();
        bindLabel(startLabel);
        
        uint32_t forInElemTypeId = (forInTypeId == 101) ? 109 : 100;
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&forIn->getIterable())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                uint32_t et = forInTypeId == 103
                    ? getLocalKeyTypeId(id->getName())
                    : getLocalElementTypeId(id->getName());
                if (et != 0) forInElemTypeId = et;
            }
        }

        // Lowered: item = iter[i] via a type-correct runtime accessor.
        emit(Opcode::MOV, OperandsR{1, iterReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, iReg, 0, 0});
        if (forInElemTypeId == 2 || forInElemTypeId == 9) {
            emitCall(Opcode::CALL, "_F_array_get_double_v_p_p");
        } else if (forInElemTypeId == 3 || forInElemTypeId == 8) {
            emitCall(Opcode::CALL, "_F_array_get_bool_v_p_p");
        } else if (forInElemTypeId == 101) {
            emitCall(Opcode::CALL, "_F_array_get_object_v_p_p");
        } else if (forInElemTypeId == 100 || forInElemTypeId == 109) {
            emitCall(Opcode::CALL, "_F_array_get_object_v_p_p");
        } else {
            emitCall(Opcode::CALL, "_F_array_get_int64_v_p_p");
        }
        uint8_t itemReg = allocateRegister();
        emit(Opcode::MOV, OperandsR{itemReg, 1, 0, 0});
        int32_t itemOffset = reserveLocal(forIn->getVariable(), forInElemTypeId);
        emit(Opcode::ST_D, OperandsI{itemReg, 30, static_cast<int16_t>(itemOffset)});
        freeRegister(itemReg);
        
        RegisterMask loopBodyMask = captureRegisterMask();
        controlFlowStack_.push({endLabel, stepLabel, scopeStack_.size(), loopExitMask, loopBodyMask});
        visitStatement(forIn->getBody());
        restoreRegisterMask(loopBodyMask);
        controlFlowStack_.pop();
        
        bindLabel(stepLabel);
        // i = i + 1
        uint8_t oneReg = emitConstant(1);
        uint8_t nextIReg = allocateRegister();
        emit(Opcode::ARITH, OperandsR{nextIReg, iReg, oneReg, 0});
        emit(Opcode::MOV, OperandsR{iReg, nextIReg, 0, 0});
        freeRegister(oneReg);
        freeRegister(nextIReg);
        
        // Emit LOOP_DECBR
        uint32_t loopDecbrOffset = currentByteOffset_;
        emitBranch(Opcode::LOOP_DECBR, 0, 0, startLabel);
        
        // Fixup LOOP_SET backedge displacement
        auto& loopSetInst = instructions_[loopSetIdx];
        auto ops = std::get<OperandsI>(loopSetInst.getOperands());
        int32_t byteOffset = startLabel->targetByteOffset - static_cast<int32_t>(loopDecbrOffset);
        int32_t wordOffset = byteOffset / 4;
        ops.imm15 = static_cast<int16_t>(wordOffset);
        loopSetInst.setOperands(ops);
        
        bindLabel(endLabel);
        restoreRegisterMask(loopExitMask);
    } else if (auto tryCatch = dynamic_cast<const ast::TryCatchStatement*>(&stmt)) {
        Label* catchStartLabel = createLabel();
        Label* finallyLabel = createLabel();
        Label* unhandledFinallyLabel = createLabel();
        Label* unhandledLabel = createLabel();
        Label* endLabel = createLabel();
        Label* afterTryLabel = createLabel();
        // The handler bridge places the exception in r1, but popping the
        // handler is itself a runtime call and may overwrite r1. Reserve a
        // temporary for the exception before entering the protected region.
        uint8_t exceptionReg = allocateRegister();
        
        // 1. Register handler. Handler transfer is a control-flow operation,
        // so use the dedicated syscall ABI rather than an ordinary CALL (the
        // JIT/interpreter route the returned PC through syscall handling).
        uint8_t handlerAddrReg = allocateRegister();
        // Handler PCs are text-section offsets, not rodata addresses. Use an
        // integer immediate so LDA's rs=0 rodata addressing special case
        // cannot reinterpret the handler as a data pointer.
        emit(Opcode::ADDI, OperandsI{handlerAddrReg, 0, 0});
        size_t ldaIdx = instructions_.size() - 1;
        
        // The state-ABI bridge reads the handler PC from argument register r2.
        emit(Opcode::MOV, OperandsR{2, handlerAddrReg, 0, 0});
        emit(Opcode::SYSCALL, OperandsI{0, 0, 7});
        freeRegister(handlerAddrReg);

        visitStatement(tryCatch->getTryBlock());
        
        // 2. Normal path: pop handler and go to finally
        emit(Opcode::SYSCALL, OperandsI{0, 0, 8});
        emitJump(Opcode::JMP, 0, finallyLabel);

        bindLabel(catchStartLabel);
        // Fixup LDA to point to catch handler
        int32_t catchOffset = catchStartLabel->targetByteOffset;
        auto ldaOps = instructions_[ldaIdx].getOperands();
        auto& ldaOpsI = std::get<OperandsI>(ldaOps);
        ldaOpsI.imm15 = static_cast<int16_t>(catchOffset);
        instructions_[ldaIdx].setOperands(ldaOpsI);

        // 3. Exception path: pop handler and select the first compatible
        // catch clause. The shadow handler places the exception in r1.
        emit(Opcode::MOV, OperandsR{exceptionReg, 1, 0, 0});
        emit(Opcode::SYSCALL, OperandsI{0, 0, 8});
        std::vector<Label*> catchLabels;
        for (size_t i = 0; i < tryCatch->getCatchClauses().size(); ++i) {
            catchLabels.push_back(createLabel());
        }
        for (size_t i = 0; i < tryCatch->getCatchClauses().size(); ++i) {
            const auto& clause = tryCatch->getCatchClauses()[i];
            Label* nextClause = (i + 1 < catchLabels.size()) ? catchLabels[i + 1] : unhandledLabel;
            uint32_t catchTypeId = getTypeId(clause.type.get(), nullptr, nullptr);
            if (catchTypeId == 100) {
                // `Exception` is the open base type in the Hoo type system;
                // every runtime exception is compatible with it.
                emitJump(Opcode::JMP, 0, catchLabels[i]);
                continue;
            }
            emit(Opcode::MOV, OperandsR{1, exceptionReg, 0, 0});
            uint8_t expectedReg = emitConstant(static_cast<int64_t>(catchTypeId));
            emit(Opcode::MOV, OperandsR{2, expectedReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_exception_matches_type_i8_p_i8");
            freeRegister(expectedReg);
            emitBranch(Opcode::BEQ, 1, 0, nextClause);
            emitJump(Opcode::JMP, 0, catchLabels[i]);
        }

        for (size_t i = 0; i < tryCatch->getCatchClauses().size(); ++i) {
            const auto& clause = tryCatch->getCatchClauses()[i];
            bindLabel(catchLabels[i]);
            emit(Opcode::MOV, OperandsR{1, exceptionReg, 0, 0});
            uint8_t excReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{excReg, exceptionReg, 0, 0});
            std::string catchClassName;
            uint32_t catchTypeId = getTypeId(clause.type.get(), nullptr, &catchClassName);
            int32_t itemOffset = reserveLocal(clause.variable, catchTypeId, catchClassName);
            emit(Opcode::ST_D, OperandsI{excReg, 30, static_cast<int16_t>(itemOffset)});
            freeRegister(excReg);
            visitStatement(*clause.block);
            emitJump(Opcode::JMP, 0, finallyLabel);
        }

        freeRegister(exceptionReg);
        bindLabel(unhandledLabel);
        if (tryCatch->getFinallyBlock()) {
            emitJump(Opcode::JMP, 0, unhandledFinallyLabel);
        } else {
            emit(Opcode::SYSCALL, OperandsI{0, 0, 10});
            emitJump(Opcode::JMP, 0, endLabel);
        }

        // 4. Finally block — executed on both normal and catch paths
        bindLabel(finallyLabel);
        if (tryCatch->getFinallyBlock()) {
            visitStatement(*tryCatch->getFinallyBlock());
        }
        bindLabel(endLabel);
        if (tryCatch->getFinallyBlock()) {
            emitJump(Opcode::JMP, 0, afterTryLabel);
            bindLabel(unhandledFinallyLabel);
            visitStatement(*tryCatch->getFinallyBlock());
            emit(Opcode::SYSCALL, OperandsI{0, 0, 10});
        }
        bindLabel(afterTryLabel);
    } else if (auto throwStmt = dynamic_cast<const ast::ThrowStatement*>(&stmt)) {
        if (throwStmt->isRethrow()) {
            emit(Opcode::SYSCALL, OperandsI{0, 0, 10});
        } else {
            uint8_t excReg = visitExpression(*throwStmt->getExpression());
            // The syscall ABI consumes the thrown handle from r2.
            emit(Opcode::MOV, OperandsR{2, excReg, 0, 0});
            emit(Opcode::SYSCALL, OperandsI{0, 0, 9});
            freeRegister(excReg);
        }
    }
}

uint8_t HVMCodeGenerator::visitExpression(const ast::Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        const auto& primary = primaryExpr->getPrimary();
        if (auto intLit = dynamic_cast<const ast::IntegerLiteral*>(&primary)) {
            return emitConstant(intLit->getValue());
        }
        if (auto id = dynamic_cast<const ast::Identifier*>(&primary)) {
            int32_t offset = getLocalOffset(id->getName());
            uint8_t reg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            return reg;
        }
        if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primary)) {
            return visitExpression(paren->getExpression());
        }
        if (auto floatLit = dynamic_cast<const ast::FloatingLiteral*>(&primary)) {
            double val = floatLit->getValue();
            Section* rodata = module_->getSection(".rodata");
            if (!rodata) {
                Section s;
                s.name = ".rodata";
                s.type = SectionType::SHT_RODATA;
                s.flags = SectionFlags::ALLOC;
                module_->addSection(std::move(s));
                rodata = module_->getSection(".rodata");
            }
            // Align to 8 bytes
            while (rodata->data.size() % 8 != 0) rodata->data.push_back(0);
            
            uint32_t offset = static_cast<uint32_t>(rodata->data.size());
            uint64_t bits;
            std::memcpy(&bits, &val, sizeof(bits));
            for (int i = 0; i < 8; ++i) {
                rodata->data.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
            }
            rodata->virtual_size = rodata->data.size();
            
            uint8_t addrReg = emitRoDataAddress(offset);
            uint8_t dest = allocateRegister();
            emit(Opcode::LD_D, OperandsI{dest, addrReg, 0});
            freeRegister(addrReg);
            return dest;
        }
        if (auto f8Lit = dynamic_cast<const ast::F8Literal*>(&primary)) {
            double val = f8Lit->getValue();
            Section* rodata = module_->getSection(".rodata");
            if (!rodata) {
                Section s;
                s.name = ".rodata";
                s.type = SectionType::SHT_RODATA;
                s.flags = SectionFlags::ALLOC;
                module_->addSection(std::move(s));
                rodata = module_->getSection(".rodata");
            }
            while (rodata->data.size() % 8 != 0) rodata->data.push_back(0);

            uint32_t offset = static_cast<uint32_t>(rodata->data.size());
            uint64_t bits;
            std::memcpy(&bits, &val, sizeof(bits));
            for (int i = 0; i < 8; ++i) {
                rodata->data.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
            }
            rodata->virtual_size = rodata->data.size();

            uint8_t addrReg = emitRoDataAddress(offset);
            uint8_t dest = allocateRegister();
            emit(Opcode::LD_D, OperandsI{dest, addrReg, 0});
            freeRegister(addrReg);
            return dest;
        }
   if (auto decimalLit = dynamic_cast<const ast::DecimalLiteral*>(&primary)) {
    std::string value = decimalLit->getValue();
    if (!value.empty() && (value.back() == 'm' || value.back() == 'M')) {
        value.pop_back();
    }

    Section* rodata = module_->getSection(".rodata");
    if (!rodata) {
        Section s;
        s.name = ".rodata";
        s.type = SectionType::SHT_RODATA;
        s.flags = SectionFlags::ALLOC;
        module_->addSection(std::move(s));
        rodata = module_->getSection(".rodata");
    }
    uint32_t offset = static_cast<uint32_t>(rodata->data.size());
    for (char c : value) rodata->data.push_back(static_cast<uint8_t>(c));
    rodata->data.push_back(0);
    rodata->virtual_size = rodata->data.size();

    uint8_t addrReg = emitRoDataAddress(offset);
    uint8_t precReg = emitConstant(static_cast<int64_t>(currentDecimalPrecision_));
    uint8_t scaleReg = emitConstant(static_cast<int64_t>(currentDecimalScale_));
    emit(Opcode::MOV, OperandsR{1, addrReg, 0, 0});
    emit(Opcode::MOV, OperandsR{2, precReg, 0, 0});
    emit(Opcode::MOV, OperandsR{3, scaleReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Decimal_from_literal_p_p_i8_i8");
    freeRegister(addrReg);
    freeRegister(precReg);
    freeRegister(scaleReg);
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    return dest;
}
        if (auto charLit = dynamic_cast<const ast::CharacterLiteral*>(&primary)) {
            uint8_t cpReg = emitConstant(static_cast<int64_t>(charLit->getValue()));
            emit(Opcode::MOV, OperandsR{1, cpReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Character_from_codepoint_p_i8");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(cpReg);
            return dest;
        }
        if (auto boolLit = dynamic_cast<const ast::BooleanLiteral*>(&primary)) {
            return emitConstant(boolLit->getValue() ? 1 : 0);
        }
        if (auto bitLit = dynamic_cast<const ast::BitLiteral*>(&primary)) {
            return emitConstant(bitLit->getValue());
        }
        if (auto nullLit = dynamic_cast<const ast::NullLiteral*>(&primary)) {
            return emitConstant(0);
        }
        if (auto thisLit = dynamic_cast<const ast::ThisLiteral*>(&primary)) {
            uint8_t reg = allocateRegister();
            int32_t thisOffset = getLocalOffset("this");
            if (thisOffset != 0) {
                emit(Opcode::LD_D, OperandsI{reg, 30, static_cast<int16_t>(thisOffset)});
            } else {
                emit(Opcode::MOV, OperandsR{reg, 1, 0, 0});
            }
            return reg;
        }
        if (auto arrayLit = dynamic_cast<const ast::ArrayLiteral*>(&primary)) {
            auto& elements = arrayLit->getElements()->getExpressions();

            if (arrayLit->isList()) {
                emitCall(Opcode::CALL, "_F_hoo_list_new_p");
                uint8_t arrReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});

                for (const auto& elem : elements) {
                    uint8_t elemReg = visitExpression(*elem);
                    uint32_t elemType = getTypeId(nullptr, elem.get());
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(elemType));
                    emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{3, elemReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_list_push_i8_p_i8_i8");
                    freeRegister(typeReg);
                    if (isManagedTemporary(*elem)) {
                        emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
                        emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                    }
                    freeRegister(elemReg);
                }

                return arrReg;
            }
            
            emitCall(Opcode::CALL, "_F_hoo_Array_new_p");
            uint8_t arrReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});

            uint32_t commonElemType = 0;
            for (const auto& elem : elements) {
                uint32_t t = getTypeId(nullptr, elem.get());
                if (t == 100) { commonElemType = 100; break; }
                if (commonElemType == 0) commonElemType = t;
                else if (commonElemType != t) { commonElemType = 100; break; }
            }
            if (commonElemType >= 100 && commonElemType != 100 && commonElemType != 109) {
                uint8_t offReg = emitConstant(16);
                uint8_t typeReg = emitConstant(static_cast<int64_t>(commonElemType));
                emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, offReg, 0, 0});
                emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_set_field_v_p_i8_p");
                freeRegister(offReg);
                freeRegister(typeReg);
            }
            
            for (const auto& elem : elements) {
                uint8_t elemReg = visitExpression(*elem);
                uint32_t elemType = getTypeId(nullptr, elem.get());
                
                bool isNestedArray = false;
                if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(elem.get())) {
                    if (dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
                        isNestedArray = true;
                    }
                }
                
                emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, elemReg, 0, 0});
                
                if (elemType == 1 || elemType == 8) {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushInt64_p_i8");
                } else if (elemType == 101) {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushObject_p_p");
                } else if (isNestedArray) {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushArray_p_p");
                } else if (elemType == 2 || elemType == 9) {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushDouble_p_d");
                } else if (elemType == 3) {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushBool_p_i8");
                } else {
                    emitCall(Opcode::CALL, "_F_hoo_Array_pushObject_p_p");
                }
                
                emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});
                if (isManagedTemporary(*elem)) {
                    emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                }
                freeRegister(elemReg);
            }
            
            return arrReg;
        }
        if (auto tensorLit = dynamic_cast<const ast::TensorLiteral*>(&primary)) {
            return emitTensorLiteral(*tensorLit);
        }
        if (auto strLit = dynamic_cast<const ast::StringLiteral*>(&primary)) {
            std::string val = strLit->getValue();
            Section* rodata = module_->getSection(".rodata");
            if (!rodata) {
                Section s;
                s.name = ".rodata";
                s.type = SectionType::SHT_RODATA;
                s.flags = SectionFlags::ALLOC;
                module_->addSection(std::move(s));
                rodata = module_->getSection(".rodata");
            }
            uint32_t offset = static_cast<uint32_t>(rodata->data.size());
            for (char c : val) rodata->data.push_back(c);
            rodata->data.push_back('\0');
            rodata->virtual_size = rodata->data.size();
            uint8_t addrReg = emitRoDataAddress(offset);

            emit(Opcode::MOV, OperandsR{1, addrReg, 0, 0});
            emitCall(Opcode::CALL, "_F_M_hoo_E_String_fromCStr_static_p_p");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});

            freeRegister(addrReg);
            return dest;
        }
        if (auto interpStr = dynamic_cast<const ast::InterpolatedString*>(&primary)) {
            uint8_t resReg = 0;

            auto appendToRes = [&](uint8_t partReg) {
                if (resReg == 0) {
                    resReg = allocateRegister();
                    emit(Opcode::MOV, OperandsR{resReg, partReg, 0, 0});
                } else {
                    uint8_t nextRes = allocateRegister();
                    emit(Opcode::MOV, OperandsR{1, resReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, partReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_M_hoo_E_String_concat_p_p");
                    emit(Opcode::MOV, OperandsR{nextRes, 1, 0, 0});
                    
                    emit(Opcode::MOV, OperandsR{1, resReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                    emit(Opcode::MOV, OperandsR{1, partReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                    
                    freeRegister(resReg);
                    freeRegister(partReg);
                    resReg = nextRes;
                }
            };

            auto emitStringPart = [&](const std::string& text) {
                Section* rodata = module_->getSection(".rodata");
                if (!rodata) {
                    Section s;
                    s.name = ".rodata";
                    s.type = SectionType::SHT_RODATA;
                    s.flags = SectionFlags::ALLOC;
                    module_->addSection(std::move(s));
                    rodata = module_->getSection(".rodata");
                }
                uint32_t offset = static_cast<uint32_t>(rodata->data.size());
                for (char c : text) rodata->data.push_back(c);
                rodata->data.push_back('\0');
                rodata->virtual_size = rodata->data.size();
                uint8_t addrReg = emitRoDataAddress(offset);
                
                emit(Opcode::MOV, OperandsR{1, addrReg, 0, 0});
                emitCall(Opcode::CALL, "_F_M_hoo_E_String_fromCStr_static_p_p");
                uint8_t strReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{strReg, 1, 0, 0});
                freeRegister(addrReg);
                return strReg;
            };

            for (const auto& part : interpStr->getParts()) {
                uint8_t partReg = 0;
                if (part.isExpression) {
                    uint8_t valReg = visitExpression(*part.expression);
                    
                    int64_t typeId = 100; // Default: Object
                    const ast::Expression* actualExpr = part.expression.get();
                    
                    // Unfold PrimaryExpression to find literals
                    const ast::ASTNode* targetNode = actualExpr;
                    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(actualExpr)) {
                        targetNode = &pe->getPrimary();
                    }

                    if (dynamic_cast<const ast::IntegerLiteral*>(targetNode)) typeId = 1;
                    else if (dynamic_cast<const ast::FloatingLiteral*>(targetNode)) typeId = 2;
                    else if (dynamic_cast<const ast::BooleanLiteral*>(targetNode)) typeId = 3;
                    else if (dynamic_cast<const ast::StringLiteral*>(targetNode)) typeId = 101;
                    else if (dynamic_cast<const ast::CharacterLiteral*>(targetNode)) typeId = 109;
                    else if (auto id = dynamic_cast<const ast::Identifier*>(targetNode)) {
                        for (auto si = scopeStack_.rbegin(); si != scopeStack_.rend(); ++si) {
                            auto it = si->find(id->getName());
                            if (it != si->end()) {
                                typeId = it->second.typeId != 0 ? it->second.typeId : 100;
                                break;
                            }
                        }
                    }

                    uint8_t typeIdReg = emitConstant(typeId);
                    emit(Opcode::MOV, OperandsR{1, valReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeIdReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_M_hoo_E_String_fromAny_static_p_i8_i8");
                    
                    partReg = allocateRegister();
                    emit(Opcode::MOV, OperandsR{partReg, 1, 0, 0});
                    
                    freeRegister(valReg);
                    freeRegister(typeIdReg);
                } else {
                    partReg = emitStringPart(part.literal);
                }
                appendToRes(partReg);
            }

            if (resReg == 0) {
                emitCall(Opcode::CALL, "_F_M_hoo_E_String_new_static_p");
                resReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{resReg, 1, 0, 0});
            }

            return resReg;
        }
    }

    if (auto awaitExpr = dynamic_cast<const ast::AwaitExpression*>(&expr)) {
        if (!currentFunctionIsAsync_) {
            addError("await expression must be used inside an async function");
            return 0;
        }
        uint32_t futureTypeId = inferExpressionTypeId(awaitExpr->getFuture());
        if (futureTypeId != 123) {
            addError(std::string("await expression must be used with a Future type"));
            return 0;
        }
        uint8_t futureReg = visitExpression(awaitExpr->getFuture());
        emit(Opcode::MOV, OperandsR{1, futureReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_future_await_unwrap_native_p_p");
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        /* Identifier expressions borrow a local Future; call expressions
         * produce an owned temporary which must be released after await. */
        const ast::ASTNode* futureSource = &awaitExpr->getFuture();
        while (auto primary = dynamic_cast<const ast::PrimaryExpression*>(futureSource)) {
            futureSource = &primary->getPrimary();
        }
        while (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(futureSource)) {
            futureSource = &paren->getExpression();
        }
        if (dynamic_cast<const ast::FunctionCall*>(futureSource)) {
            emit(Opcode::RELEASE, OperandsR{futureReg, 0, 0, 0});
        }
        freeRegister(futureReg);
        return dest;
    }

    if (auto newHash = dynamic_cast<const ast::NewDictExpression*>(&expr)) {
        std::string requiredModule = getRequiredModule("Dict");
        if (!isSymbolImported("Dict", requiredModule)) {
            addError("Use of 'Dict' requires 'import " + requiredModule + ";'");
            return 0;
        }
        const auto& type = newHash->getDictType();
        uint8_t keyTypeReg = emitConstant(static_cast<int64_t>(dictKeyTypeId(type)));
        uint8_t valueTypeReg = emitConstant(static_cast<int64_t>(typeIdFromDeclaredType(&type.getValueType())));
        emit(Opcode::MOV, OperandsR{1, keyTypeReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, valueTypeReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_dict_new_p_i8_i8");
        freeRegister(keyTypeReg);
        freeRegister(valueTypeReg);
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        return dest;
    }

    if (auto newExpr = dynamic_cast<const ast::NewObjectExpression*>(&expr)) {
        std::string className = newExpr->getClassName();
        std::string requiredModule = getRequiredModule(className);
        if (!isSymbolImported(className, requiredModule)) {
            addError("Use of '" + className + "' requires 'import " + requiredModule + ";'");
            return 0;
        }
        // Factory construction: new <Class>.<factoryName>(args). The factory is
        // a plain function that returns an instance — no hoo_alloc, no
        // generative-constructor call. The dotted prefix may be a
        // module-qualified reference; the last dotted component is the class.
        if (!newExpr->getFactoryName().empty()) {
            const std::string& factoryName = newExpr->getFactoryName();
            auto classIt = classes_.find(className);
            if (classIt == classes_.end()) {
                addError("Unknown class: " + className);
                return 0;
            }
            if (classIt->second.isService) {
                addError("Cannot create instance of service class '" + className + "'");
                return 0;
            }
            auto& factoryNames = classIt->second.factoryNames;
            if (std::find(factoryNames.begin(), factoryNames.end(), factoryName) == factoryNames.end()) {
                addError("Class '" + className + "' has no factory constructor named '" + factoryName + "'");
                return 0;
            }

            // Evaluate arguments into the shared temporary registers r1..
            const auto* argumentList = newExpr->getArguments();
            const size_t argCount = argumentList ? argumentList->getArguments().size() : 0;
            std::vector<uint8_t> argRegs;
            for (size_t i = 0; argumentList && i < argCount && i < 7; ++i) {
                argRegs.push_back(visitExpression(*argumentList->getArguments()[i]));
            }
            for (size_t i = 0; i < argRegs.size(); ++i) {
                emit(Opcode::MOV, OperandsR{argReg(1, i), argRegs[i], 0, 0});
                freeRegister(argRegs[i]);
            }

            MangledFunctionParams mp;
            mp.modulePath = modulePath_;
            mp.className = className;
            mp.isFactoryConstructor = true;
            mp.functionName = factoryName;
            mp.returnType = "ptr";

            if (argumentList) {
                for (size_t i = 0; i < argCount; ++i) {
                    auto typeInfo = inferExpressionTypeInfo(*argumentList->getArguments()[i]);
                    mp.parameterTypes.push_back(typeIdToMangleType(typeInfo.typeId));
                }
            }
            emitCall(Opcode::CALL, SymbolMangler::mangleFunctionName(mp));
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        }
        if (className == "Exception") {
            const auto* argumentList = newExpr->getArguments();
            const size_t argCount = argumentList ? argumentList->getArguments().size() : 0;
            if (argCount > 1) {
                addError("Exception constructor expects zero or one argument");
                return 0;
            }
            // Exception values are owned by the native runtime. The Hoo
            // constructor is lowered to its canonical runtime exception
            // factory instead of an unresolved class constructor symbol.
            emitCall(Opcode::CALL, "_F_hoo_exception_runtime_p");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        }
        if (isBuiltinClassName(className) && classes_.find(className) == classes_.end()) {
            if (builtinConstructedTypeId(className) == 100) {
                addError("Built-in class '" + className + "' does not have a constructor");
                return 0;
            }
            const auto* argumentList = newExpr->getArguments();
            const size_t argCount = argumentList ? argumentList->getArguments().size() : 0;
            std::vector<uint8_t> argRegs;
            for (size_t i = 0; argumentList && i < argCount && i < 7; ++i) {
                argRegs.push_back(visitExpression(*argumentList->getArguments()[i]));
            }
            for (size_t i = 0; i < argRegs.size(); ++i) {
                emit(Opcode::MOV, OperandsR{argReg(1, i), argRegs[i], 0, 0});
                freeRegister(argRegs[i]);
            }

            if (className == "List") {
                if (argCount != 0) {
                    addError("List constructor expects zero arguments");
                    return 0;
                }
                emitCall(Opcode::CALL, "_F_hoo_list_new_p");
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }

            if (className == "Buffer" && argCount != 0) {
                addError("Buffer constructor expects no arguments");
                return 0;
            }

            if (className == "Map" && argCount != 2) {
                addError("Map constructor expects exactly two arguments");
                return 0;
            }

            if (className == "Uuid" && argCount != 1) {
                addError("Uuid constructor expects exactly one argument");
                return 0;
            }

            if (className == "Character" && argCount != 1) {
                addError("Character constructor expects exactly one argument");
                return 0;
            }

            MangledFunctionParams mp;
            mp.modulePath = {"hoo"};
            mp.functionName = classToPrefix(className) + "_" + builtinConstructorMethodName(className, argCount);
            mp.returnType = "void";
            for (size_t i = 0; i < argCount; ++i) {
                mp.parameterTypes.push_back("ptr");
            }

            emitCall(Opcode::CALL, SymbolMangler::mangleFunctionName(mp));
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        }

        auto it = classes_.find(className);
        if (it == classes_.end()) {
            addError("Unknown class: " + className);
            return 0;
        }
        
        // Service classes cannot be instantiated with 'new'
        if (it->second.isService) {
            addError("Cannot create instance of service class '" + className + "'");
            return 0;
        }
        
        // Singleton: load pre-allocated instance from .data
        if (it->second.isSingleton) {
            uint8_t addrReg = allocateRegister();
            emit(Opcode::LDA, OperandsI{addrReg, 1, static_cast<int16_t>(it->second.singletonDataOffset)});
            uint8_t dest = allocateRegister();
            emit(Opcode::LD_D, OperandsI{dest, addrReg, 0});
            freeRegister(addrReg);
            return dest;
        }
        
        // 1. Allocate: CALL hoo_alloc(size, typeId)
        uint8_t sizeReg = emitConstant(static_cast<int64_t>(it->second.totalSize));
        uint8_t typeReg = emitConstant(100); // Generic Object typeId
        emit(Opcode::MOV, OperandsR{1, sizeReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_alloc_p_i8_i8");
        
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        // New objects start with refcount=1 from hoo_alloc — RETAIN not needed
        freeRegister(sizeReg);
        freeRegister(typeReg);

        // Constructor calls share the same HVM temporary registers as the
        // caller, so preserve the new instance through the call.
        currentStackOffset_ -= 8;
        int32_t instanceTempOffset = currentStackOffset_;
        emit(Opcode::ST_D, OperandsI{dest, 30, static_cast<int16_t>(instanceTempOffset)});
        
        // 2. Call constructor
        MangledFunctionParams mp;
        mp.modulePath = modulePath_;
        mp.className = className;
        mp.isConstructor = true;

        for (const auto& param : newExpr->getArguments()->getArguments()) {
            mp.parameterTypes.push_back("ptr");
        }
        std::string ctorName = SymbolMangler::mangleFunctionName(mp);

        // Set 'this' in r1
        emit(Opcode::MOV, OperandsR{1, dest, 0, 0});
        
        if (newExpr->getArguments()) {
            auto& args = newExpr->getArguments()->getArguments();
            std::vector<uint8_t> argRegs;
            for (size_t i = 0; i < args.size() && i < 6; ++i) {
                argRegs.push_back(visitExpression(*args[i]));
            }
            for (size_t i = 0; i < argRegs.size(); ++i) {
                emit(Opcode::MOV, OperandsR{argReg(2, i), argRegs[i], 0, 0});
                freeRegister(argRegs[i]);
            }
        }

        emitCall(Opcode::CALL, ctorName); 
        emit(Opcode::LD_D, OperandsI{dest, 30, static_cast<int16_t>(instanceTempOffset)});

        return dest;
    }

    if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(&expr)) {
        const auto receiverInfo = inferExpressionTypeInfo(memberAccess->getObject());
        uint8_t objReg = visitExpression(memberAccess->getObject());
        if (receiverInfo.isNullable) {
            emitNullCheck(objReg);
        }
        int32_t offset = 0; 
        std::string foundClass;
        std::string receiverClass = receiverInfo.className;
        if (receiverClass.empty()) receiverClass = builtinClassNameFromTypeId(receiverInfo.typeId);
        if (!receiverClass.empty()) {
            auto classIt = classes_.find(receiverClass);
            if (classIt != classes_.end()) {
                auto it = classIt->second.fieldOffsets.find(memberAccess->getMember());
                if (it != classIt->second.fieldOffsets.end()) {
                    offset = it->second;
                    foundClass = receiverClass;
                }
            }
        }
        if (foundClass.empty()) {
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(memberAccess->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    foundClass = className;
                    break;
                }
            }
        }
        if (foundClass.empty()) {
            addError("Undefined member: " + memberAccess->getMember());
            return objReg;
        }
        if (!canReadField(memberAccess->getMember(), foundClass)) {
            addError("Cannot access private field '" + memberAccess->getMember() + "' of class '" + foundClass + "'");
            return objReg;
        }
        uint8_t offsetReg = emitConstant(static_cast<int64_t>(offset));
        emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, offsetReg, 0, 0});
        emitCall(Opcode::CALL, "_F_object_get_field_p_i8");
        freeRegister(offsetReg);
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        freeRegister(objReg);
        return dest;
    }

    if (auto funcCall = dynamic_cast<const ast::FunctionCall*>(&expr)) {
        const ast::Expression* targetPtr = &funcCall->getFunction();
        
        // Unwrap PrimaryExpression/ParenthesizedExpression if needed
        while (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(targetPtr)) {
            if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primaryExpr->getPrimary())) {
                targetPtr = &paren->getExpression();
            } else {
                break;
            }
        }

        // Helper to extract a bare identifier name from a MemberAccess object expression.
        // The parser wraps the object in PrimaryExpression(Identifier(...)).
        auto resolveObjectName = [](const ast::MemberAccess& ma) -> std::string {
            const ast::Expression& obj = ma.getObject();
            if (auto primExpr = dynamic_cast<const ast::PrimaryExpression*>(&obj)) {
                if (auto id = dynamic_cast<const ast::Identifier*>(&primExpr->getPrimary())) {
                    return id->getName();
                }
            }
            return {};
        };

        if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(targetPtr)) {
            std::string methodName = memberAccess->getMember();
            std::string resolvedClass;
            bool isStaticCall = false;

            // Detect static calls on built-in class names (DateTime.now())
            std::string objName = resolveObjectName(*memberAccess);
            if (methodName == "release" && !objName.empty()) {
                for (auto scopeIt = scopeStack_.rbegin(); scopeIt != scopeStack_.rend(); ++scopeIt) {
                    auto localIt = scopeIt->find(objName);
                    if (localIt != scopeIt->end() && localIt->second.typeId == 110) {
                        localIt->second.explicitlyReleased = true;
                        break;
                    }
                }
            }
            if (!objName.empty() && isBuiltinClassName(objName) && getLocalTypeId(objName) == 0 && !classes_.count(objName)) {
                resolvedClass = objName;
                isStaticCall = true;
            }
            // DateTime is in classes_ map but still supports static calls (now(), parse(), etc.)
            if (resolvedClass.empty() && objName == "DateTime" && isBuiltinClassName(objName)) {
                resolvedClass = objName;
                isStaticCall = true;
            }

            // Resolve the receiver recursively before consulting the legacy
            // method-name index. This preserves the class of expressions such
            // as `obj.factory().length()` and avoids dispatching by method name
            // when the receiver already carries precise type information.
            if (resolvedClass.empty()) {
                const auto receiverInfo = inferExpressionTypeInfo(memberAccess->getObject());
                resolvedClass = receiverInfo.className;
                if (resolvedClass.empty()) {
                    resolvedClass = builtinClassNameFromTypeId(receiverInfo.typeId);
                }
            }

            // Keep the fallback candidates until all receiver-specific paths
            // have been tried. Selecting the last class seen is unsafe when
            // unrelated classes share a method name.
            std::vector<std::string> fallbackCandidates;
            if (resolvedClass.empty()) {
                auto it = methodNameToClasses_.find(methodName);
                if (it != methodNameToClasses_.end()) {
                    fallbackCandidates = it->second;
                    if (fallbackCandidates.size() == 1) {
                        resolvedClass = fallbackCandidates.front();
                    }
                }
            }

            // Generated serializable deserialization is class-qualified and
            // static even though it has no source-level method declaration.
            // Resolve it before the generic unresolved-method diagnostic.
            if (!isStaticCall && methodName == "deserialize" &&
                !objName.empty() && classes_.count(objName) &&
                classes_.at(objName).isSerializable) {
                isStaticCall = true;
                resolvedClass = objName;
            }

            // Enforce visibility after receiver inference as well as after
            // legacy name-based resolution. Recursive inference can identify
            // a class before the fallback index is consulted.
            if (!isStaticCall && !resolvedClass.empty()) {
                auto classIt = classes_.find(resolvedClass);
                if (classIt != classes_.end()) {
                    auto privIt = classIt->second.privateMethods.find(methodName);
                    if (privIt != classIt->second.privateMethods.end() && privIt->second) {
                        bool canAccess = currentClass_ &&
                            (currentClass_->name == resolvedClass ||
                             isDerivedFrom(currentClass_->name, resolvedClass));
                        if (!canAccess) {
                            addError("Cannot access private method '" + methodName + "' of class '" + resolvedClass + "'");
                        }
                    }
                }
            }

            // Detect instance calls on built-in types by the object's typeId or literal type
            if (resolvedClass.empty()) {
                // Check if object is a local variable with known type
                std::string objName2 = resolveObjectName(*memberAccess);
                uint32_t typeId = 0;
                if (!objName2.empty()) {
                    typeId = getLocalTypeId(objName2);
                } else {
                    // Check for string literals
                    auto* objExpr = &memberAccess->getObject();
                    while (auto primExpr = dynamic_cast<const ast::PrimaryExpression*>(objExpr)) {
                        auto& primary = primExpr->getPrimary();
                        if (dynamic_cast<const ast::StringLiteral*>(&primary)) {
                            typeId = 101;
                        } else if (dynamic_cast<const ast::IntegerLiteral*>(&primary)) {
                            // Int64 has no recognized object methods by default
                        }
                        break;
                    }
                }
                switch (typeId) {
                    case 101: resolvedClass = "String"; break;
                    case 102: resolvedClass = "Array"; break;
                    case 103: resolvedClass = "Map"; break;
                    case 109: resolvedClass = "Character"; break;
                    case 110: resolvedClass = "Args"; break;
                    case 111: resolvedClass = "Compression"; break;
                    case 114: resolvedClass = "Csv"; break;
                    case 113: resolvedClass = "Buffer"; break;
                    case 106: resolvedClass = "URL"; break;
                    case 108: resolvedClass = "HttpClient"; break;
                    case 107: resolvedClass = "HttpResponse"; break;
                    case 105: resolvedClass = "Random"; break;
                    case 117: resolvedClass = "Dict"; break;
                    case 118: resolvedClass = "List"; break;
                    case 119: resolvedClass = "DateTime"; break;
                    case 120: resolvedClass = "Regex"; break;
                    case 121: resolvedClass = "Mutex"; break;
                    case 122: resolvedClass = "Uuid"; break;
                    case 125: resolvedClass = "Decimal"; break;
                    default: break;
                }
            }

            if (resolvedClass.empty()) {
                if (fallbackCandidates.size() > 1) {
                    std::string candidates;
                    for (size_t i = 0; i < fallbackCandidates.size(); ++i) {
                        if (i) candidates += ", ";
                        candidates += fallbackCandidates[i];
                    }
                    addError("Cannot safely resolve method '" + methodName +
                             "' for an unknown receiver; candidates: " + candidates);
                } else {
                    addError("Cannot resolve method '" + methodName + "'");
                }
            } else if (isBuiltinClassName(resolvedClass)) {
                std::string requiredModule = getRequiredModule(resolvedClass);
                if (!isSymbolImported(resolvedClass, requiredModule)) {
                    addError("Use of '" + resolvedClass + "' requires 'import " + requiredModule + ";'");
                    return 0;
                }
            }

            if (isStaticCall && resolvedClass == "Buffer" && methodName == "fromBytes") {
                addError("Buffer.fromBytes is not supported; use free function buffer_fromBytes(data, len)");
                return 0;
            }

            if (isStaticCall && resolvedClass == "Array") {
                addError("Array." + methodName + " is not supported as a static method; use an Array instance");
                return 0;
            }

            if (isStaticCall && resolvedClass == "DateTime") {
                std::string suggest;
                if (methodName == "now") suggest = "datetime_now()";
                else if (methodName == "nowSeconds") suggest = "datetime_now_seconds()";
                else if (methodName == "nowPrecise") suggest = "datetime_now_precise()";
                else if (methodName == "new") suggest = "datetime_new(ts)";
                else if (methodName == "parse") suggest = "datetime_parse(str, fmt)";
                else if (methodName == "fromIso8601") suggest = "datetime_from_iso8601(str)";
                else if (methodName == "format") suggest = "datetime_format(dt, fmt)";
                else if (methodName == "iso8601") suggest = "datetime_iso8601(dt)";
                else if (methodName == "addDays") suggest = "datetime_add_days(dt, days)";
                else if (methodName == "addHours") suggest = "datetime_add_hours(dt, hours)";
                else if (methodName == "addMinutes") suggest = "datetime_add_minutes(dt, mins)";
                else if (methodName == "addSeconds") suggest = "datetime_add_seconds(dt, secs)";
                else if (methodName == "addMilliseconds") suggest = "datetime_add_milliseconds(dt, ms)";
                else if (methodName == "diffDays") suggest = "datetime_diff_days(a, b)";
                else if (methodName == "diffHours") suggest = "datetime_diff_hours(a, b)";
                else if (methodName == "diffSeconds") suggest = "datetime_diff_seconds(a, b)";
                else if (methodName == "compare") suggest = "datetime_compare(a, b)";
                
                if (!suggest.empty()) {
                    addError("DateTime." + methodName + " is not supported as a static method; use free function " + suggest);
                } else {
                    addError("DateTime." + methodName + " is not supported as a static method");
                }
                return 0;
            }

            if (isStaticCall) {
                // Static call: no 'this' in r1, args start from r1
                if (funcCall->getArguments()) {
                    auto& args = funcCall->getArguments()->getArguments();
                    std::vector<uint8_t> argRegs;
                    for (size_t i = 0; i < args.size() && i < 7; ++i) {
                        argRegs.push_back(visitExpression(*args[i]));
                    }
                    for (size_t i = 0; i < argRegs.size(); ++i) {
                        emit(Opcode::MOV, OperandsR{argReg(1, i), argRegs[i], 0, 0});
                        freeRegister(argRegs[i]);
                    }
                }
            } else {
                // Instance call: visit args first, then load 'this' into r1
                // (visitExpression may clobber r1 for string literal construction)
                std::vector<uint8_t> argRegs;
                if (funcCall->getArguments()) {
                    auto& args = funcCall->getArguments()->getArguments();
                    for (size_t i = 0; i < args.size() && i < 6; ++i) {
                        argRegs.push_back(visitExpression(*args[i]));
                    }
                }
                uint8_t objReg = visitExpression(memberAccess->getObject());
                if (inferExpressionTypeInfo(memberAccess->getObject()).isNullable) {
                    emitNullCheck(objReg);
                }
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                freeRegister(objReg);
                for (size_t i = 0; i < argRegs.size(); ++i) {
                    emit(Opcode::MOV, OperandsR{argReg(2, i), argRegs[i], 0, 0});
                    freeRegister(argRegs[i]);
                }
            }

            if (resolvedClass == "List") {
                if (methodName == "push") {
                    uint32_t argType = 100;
                    if (funcCall->getArguments() && !funcCall->getArguments()->getArguments().empty()) {
                        argType = getTypeId(nullptr, funcCall->getArguments()->getArguments()[0].get());
                    }
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(argType));
                    emit(Opcode::MOV, OperandsR{3, 2, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_list_push_i8_p_i8_i8");
                    freeRegister(typeReg);
                } else if (methodName == "length") {
                    emitCall(Opcode::CALL, "_F_hoo_list_length_i8_p");
                } else if (methodName == "clear") {
                    emitCall(Opcode::CALL, "_F_hoo_list_clear_v_p");
                } else if (methodName == "pop") {
                    emitCall(Opcode::CALL, "_F_hoo_list_pop_data_i8_p");
                } else if (methodName == "release") {
                    emitCall(Opcode::CALL, "_F_M_hoo_E_anyarray_release_v");
                } else {
                    addError("Unsupported List method '" + methodName + "'");
                }
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }

            if (resolvedClass == "Dict") {
                if (methodName == "count") {
                    emitCall(Opcode::CALL, "_F_hoo_dict_count_i8_p");
                } else if (methodName == "clear") {
                    emitCall(Opcode::CALL, "_F_hoo_dict_clear_v_p");
                } else if (methodName == "remove") {
                    emitCall(Opcode::CALL, "_F_hoo_dict_remove_i8_p_i8");
                } else if (methodName == "release") {
                    emitCall(Opcode::CALL, "_F_M_hoo_E_hashmap_release_v");
                } else {
                    addError("Unsupported Dict method '" + methodName + "'");
                }
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }

            // Array instance methods use the plain runtime bridge names. The
            // class-qualified naming convention is reserved for String and
            // user-defined methods; using it here makes chained array results
            // fail symbol resolution even though their type is known.
            if (resolvedClass == "Array") {
                std::string symbol;
                if (methodName == "length") symbol = "_F_M_hoo_E_array_length_v_p";
                else if (methodName == "empty") symbol = "_F_M_hoo_E_array_empty_v_p";
                else if (methodName == "clear") symbol = "_F_M_hoo_E_array_clear_v_p";
                else if (methodName == "sort") symbol = "_F_M_hoo_E_array_sort_v_p";
                else if (methodName == "reverse") symbol = "_F_M_hoo_E_array_reverse_v_p";
                else if (methodName == "shuffle") symbol = "_F_M_hoo_E_array_shuffle_v_p";
                else if (methodName == "sortRange") symbol = "_F_M_hoo_E_array_sortRange_v_p_p_p";
                else if (methodName == "binarySearch") symbol = "_F_M_hoo_E_array_binarySearch_v_p_p";
                else if (methodName == "pushInt64") symbol = "_F_M_hoo_E_array_pushInt64_v_p_p";
                else if (methodName == "getInt64") symbol = "_F_M_hoo_E_array_getInt64_v_p_p";
                else if (methodName == "pushDouble") symbol = "_F_M_hoo_E_array_pushDouble_v_p_p";
                else if (methodName == "getDouble") symbol = "_F_M_hoo_E_array_getDouble_v_p_p";
                else if (methodName == "pushString") symbol = "_F_M_hoo_E_array_pushString_v_p_p";
                else if (methodName == "getString") symbol = "_F_M_hoo_E_array_getString_v_p_p";
                else if (methodName == "pushBool") symbol = "_F_M_hoo_E_array_pushBool_v_p_p";
                else if (methodName == "getBool") symbol = "_F_M_hoo_E_array_getBool_v_p_p";
                else if (methodName == "pushObject") symbol = "_F_M_hoo_E_array_pushObject_v_p_p";
                else if (methodName == "getObject") symbol = "_F_M_hoo_E_array_getObject_v_p_p";
                if (!symbol.empty()) {
                    emitCall(Opcode::CALL, symbol);
                    uint8_t dest = allocateRegister();
                    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                    return dest;
                }
            }

            if (resolvedClass == "Buffer") {
                std::string symbol;
                if (methodName == "length") symbol = "_F_M_hoo_E_buffer_length_v";
                else if (methodName == "capacity") symbol = "_F_M_hoo_E_buffer_capacity_v";
                else if (methodName == "copy") symbol = "_F_M_hoo_E_buffer_copy_v";
                else if (methodName == "clear") symbol = "_F_M_hoo_E_buffer_clear_v";
                else if (methodName == "byteAt") symbol = "_F_M_hoo_E_buffer_byteAt_v_p";
                else if (methodName == "setByte") symbol = "_F_M_hoo_E_buffer_setByte_v_p_p";
                else if (methodName == "append") symbol = "_F_M_hoo_E_buffer_append_v_p_p";
                else if (methodName == "appendBuffer") symbol = "_F_M_hoo_E_buffer_appendBuffer_v_p";
                else if (methodName == "slice") symbol = "_F_M_hoo_E_buffer_slice_v_p_p";
                else if (methodName == "data") symbol = "_F_M_hoo_E_buffer_data_v";
                if (!symbol.empty()) {
                    emitCall(Opcode::CALL, symbol);
                    uint8_t dest = allocateRegister();
                    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                    return dest;
                }
            }

            // DateTime instance methods
            if (resolvedClass == "DateTime" && !isStaticCall) {
                MangledFunctionParams mp;
                mp.modulePath = (resolvedClass.empty() || !isBuiltinClassName(resolvedClass))
                                ? modulePath_ : std::vector<std::string>{"hoo"};
                mp.functionName = "datetime_inst_" + methodName;
                mp.returnType = "ptr";
                if (methodName == "compare" || methodName == "diffDays" || methodName == "diffHours" || methodName == "getTimestamp") {
                    mp.returnType = "int64";
                } else if (methodName == "diffSeconds") {
                    mp.returnType = "double";
                }
                if (funcCall->getArguments()) {
                    auto& args = funcCall->getArguments()->getArguments();
                    for (size_t i = 0; i < args.size(); ++i) {
                        mp.parameterTypes.push_back("ptr");
                    }
                }
                std::string mangledName = SymbolMangler::mangleFunctionName(mp);
                emitCall(Opcode::CALL, mangledName);
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }

            MangledFunctionParams mp;
            mp.modulePath = (resolvedClass.empty() || !isBuiltinClassName(resolvedClass))
                            ? modulePath_ : std::vector<std::string>{"hoo"};
            mp.functionName = methodName;
            mp.isOverload = isOverloadedMethod_[resolvedClass][methodName];

            if (!resolvedClass.empty() && isBuiltinClassName(resolvedClass)) {
                if (isClassMethodJitClass(resolvedClass)) {
                    mp.className = resolvedClass;
                    mp.isStatic = isStaticCall;

                    bool isInt64Ret = (methodName == "length" || methodName == "isEmpty" ||
                                       methodName == "isSuccess" || methodName == "equals" ||
                                       methodName == "contains" || methodName == "startsWith" ||
                                       methodName == "indexOf" || methodName == "count" ||
                                       methodName == "size" || methodName == "statusCode" ||
                                       methodName == "port" || methodName == "selfPid" ||
                                       methodName == "kill" || methodName == "readchar" ||
                                       methodName == "compare" || methodName == "keyType");
                    bool isVoidRet = (methodName == "release" || methodName == "setTimeout" ||
                                      methodName == "print" || methodName == "println" ||
                                      methodName == "lock" || methodName == "unlock" ||
                                      methodName == "destroy" || methodName == "close" ||
                                      methodName == "delete" || methodName == "clear" ||
                                      methodName == "pop" || methodName == "remove" ||
                                      methodName == "push" || methodName == "pushInt64" ||
                                      methodName == "pushDouble" || methodName == "pushString" ||
                                      methodName == "pushObject" || methodName == "set" ||
                                      methodName == "setHeader" || methodName == "writeText" ||
                                      methodName == "appendText" || methodName == "mkdir" ||
                                      methodName == "mkdirs" || methodName == "rmdir" ||
                                      methodName == "copy" || methodName == "rename" ||
                                      methodName == "setEnv" || methodName == "unsetEnv" ||
                                      methodName == "setCurrentDir");
                    if (isInt64Ret) mp.returnType = "int64";
                    else if (isVoidRet) mp.returnType = "void";
                    else mp.returnType = "ptr";

                    if (funcCall->getArguments()) {
                        auto& args = funcCall->getArguments()->getArguments();
                        for (size_t i = 0; i < args.size(); ++i) {
                            mp.parameterTypes.push_back("ptr");
                        }
                    }
                } else if (isSingletonBuiltinClass(resolvedClass)) {
                    mp.className = resolvedClass;
                    mp.classModifiers = {"SINGLETON"};
                    mp.functionName = methodName;
                    std::vector<uint32_t> argTypeIds;
                    if (funcCall->getArguments()) {
                        auto& args = funcCall->getArguments()->getArguments();
                        for (const auto& arg : args) {
                            argTypeIds.push_back(inferExpressionTypeId(*arg));
                        }
                    }
                    mp.returnType = singletonMethodReturnType(resolvedClass, methodName, argTypeIds);
                    if (funcCall->getArguments()) {
                        auto& args = funcCall->getArguments()->getArguments();
                        for (size_t i = 0; i < args.size(); ++i) {
                            mp.parameterTypes.push_back("ptr");
                        }
                    }
                } else {
                    std::string prefix = classToPrefix(resolvedClass);
                    mp.functionName = prefix + "_" + methodName;
                    mp.returnType = "void";
                    if (funcCall->getArguments()) {
                        auto& args = funcCall->getArguments()->getArguments();
                        for (size_t i = 0; i < args.size(); ++i) {
                            mp.parameterTypes.push_back("ptr");
                        }
                    }
                }
            } else {
                mp.className = resolvedClass;
                mp.functionName = methodName;
                mp.returnType = "ptr";
                auto classIt = classes_.find(resolvedClass);
                if (classIt != classes_.end() && classIt->second.isSerializable) {
                    mp.classModifiers = {"SERIALIZABLE"};
                    mp.isStatic = isStaticCall;
                }
                if (funcCall->getArguments()) {
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                        if (mp.isOverload) {
                            const auto argInfo = inferExpressionTypeInfo(*arg);
                            mp.parameterTypes.push_back(mangleTypeId(argInfo.typeId, argInfo.isNullable));
                        } else {
                            mp.parameterTypes.push_back("ptr");
                        }
                    }
                }
            }

            std::string mangledName = SymbolMangler::mangleFunctionName(mp);
            if (mp.isOverload) {
                emitCall(Opcode::CALL_OVERLOADED, mangledName);
            } else {
                emitCall(Opcode::CALL, mangledName);
            }

            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        } else {
            const ast::Identifier* id = nullptr;
            if (auto idTarget = dynamic_cast<const ast::Identifier*>(targetPtr)) {
                id = idTarget;
            } else if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(targetPtr)) {
                id = dynamic_cast<const ast::Identifier*>(&primaryExpr->getPrimary());
            }

            if (id) {
                std::string functionName = id->getName();
                std::string requiredModule = getRequiredModule(functionName);
                if (!requiredModule.empty() && !isSymbolImported(functionName, requiredModule)) {
                    addError("Use of '" + functionName + "' requires 'import " + requiredModule + ";'");
                    return 0;
                }
                
                MangledFunctionParams mp;
                mp.functionName = functionName;
                mp.isOverload = isOverloadedFunction_[functionName];
                auto externalIt = externalFunctionImports_.find(functionName);
                auto externalMetaIt = externalFunctionMetadata_.find(functionName);
                if (isHooModuleFreeFunction(functionName)) {
                    mp.modulePath = std::vector<std::string>{"hoo"};
                } else if (externalMetaIt != externalFunctionMetadata_.end() &&
                           !externalMetaIt->second.modulePath.empty()) {
                    std::stringstream ss(externalMetaIt->second.modulePath);
                    std::string part;
                    while (std::getline(ss, part, '.')) {
                        if (!part.empty()) mp.modulePath.push_back(part);
                    }
                } else if (externalIt != externalFunctionImports_.end() &&
                           functionReturnTypes_.find(functionName) == functionReturnTypes_.end()) {
                    std::stringstream ss(externalIt->second.first);
                    std::string part;
                    while (std::getline(ss, part, '.')) {
                        if (!part.empty()) {
                            mp.modulePath.push_back(part);
                        }
                    }
                } else {
                    mp.modulePath = modulePath_;
                }

                if (funcCall->getArguments()) {
                    auto& args = funcCall->getArguments()->getArguments();
                    std::vector<uint8_t> argRegs;
                    for (size_t i = 0; i < args.size() && i < 7; ++i) {
                        argRegs.push_back(visitExpression(*args[i]));
                    }
                    for (size_t i = 0; i < argRegs.size(); ++i) {
                        emit(Opcode::MOV, OperandsR{argReg(1, i), argRegs[i], 0, 0});
                        freeRegister(argRegs[i]);
                    }
                }
                
                auto retIt = functionReturnTypes_.find(functionName);
                if (isHooModuleFreeFunction(functionName)) {
                    if (isMathFreeFunction(functionName)) {
                        std::vector<uint32_t> argTypeIds;
                        if (funcCall->getArguments()) {
                            for (const auto& arg : funcCall->getArguments()->getArguments()) {
                                argTypeIds.push_back(inferExpressionTypeId(*arg));
                            }
                        }
                        uint32_t typeId = hooModuleFreeFunctionReturnTypeId(functionName, argTypeIds);
                        mp.returnType = typeIdToMangleType(typeId);
                    } else if (functionName == "byte_slice_release" || functionName == "net_socket_release") {
                        mp.returnType = "void";
                    } else {
                        mp.returnType = "ptr";
                    }
                } else {
                    if (retIt != functionReturnTypes_.end()) {
                        mp.returnType = typeIdToMangleType(retIt->second);
                    } else if (externalMetaIt != externalFunctionMetadata_.end()) {
                        mp.returnType = externalMetaIt->second.returnType.empty()
                            ? "void" : externalMetaIt->second.returnType;
                    } else if (externalIt != externalFunctionImports_.end()) {
                        mp.returnType = externalIt->second.second.empty() ? "void" : externalIt->second.second;
                    } else {
                        mp.returnType = "void";
                    }
                }
                if (funcCall->getArguments()) {
                    size_t argIndex = 0;
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                        if (mp.isOverload) {
                            const auto argInfo = inferExpressionTypeInfo(*arg);
                            mp.parameterTypes.push_back(mangleTypeId(argInfo.typeId, argInfo.isNullable));
                        } else if (externalMetaIt != externalFunctionMetadata_.end() &&
                                   argIndex < externalMetaIt->second.parameterTypes.size()) {
                            mp.parameterTypes.push_back(externalMetaIt->second.parameterTypes[argIndex]);
                        } else {
                            mp.parameterTypes.push_back("ptr");
                        }
                        ++argIndex;
                    }
                }
                
                std::string mangledName = SymbolMangler::mangleFunctionName(mp);
                if (mp.isOverload) {
                    emitCall(Opcode::CALL_OVERLOADED, mangledName);
                } else {
                    emitCall(Opcode::CALL, mangledName); 
                }
                
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }
        }
        addError("Unsupported function call target: " + std::string(typeid(*targetPtr).name()));
        return 0;
    }

    if (auto binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
        const uint32_t leftExprType = inferExpressionTypeId(binary->getLeft());
        const uint32_t rightExprType = inferExpressionTypeId(binary->getRight());
      

        if (leftExprType == 125 || rightExprType == 125) {
            if (leftExprType != 125 || rightExprType != 125) {
                addError("Decimal operands must both be Decimal types");
                return 0;
            }
            return emitDecimalBinaryOp(*binary);
        }
        if (leftExprType == 104 || rightExprType == 104) {
            const bool leftIsTensor = leftExprType == 104;
            const bool rightIsTensor = rightExprType == 104;
            if (leftIsTensor != rightIsTensor) {
                const bool scalarIsLeft = !leftIsTensor;
                switch (binary->getOperator()) {
                    case ast::BinaryOperator::PLUS:
                        return emitTensorScalarCall(*binary, "_F_hoo_Tensor_add_scalar_p_p_i8_i8");
                    case ast::BinaryOperator::MINUS:
                        return emitTensorScalarCall(*binary, scalarIsLeft
                            ? "_F_hoo_Tensor_sub_scalar_left_p_p_i8_i8"
                            : "_F_hoo_Tensor_sub_scalar_p_p_i8_i8");
                    case ast::BinaryOperator::MULTIPLY:
                    case ast::BinaryOperator::ELEMENT_MULTIPLY:
                        return emitTensorScalarCall(*binary, "_F_hoo_Tensor_scale_p_p_i8_i8");
                    case ast::BinaryOperator::DIVIDE:
                    case ast::BinaryOperator::ELEMENT_DIVIDE:
                        return emitTensorScalarCall(*binary, scalarIsLeft
                            ? "_F_hoo_Tensor_div_scalar_left_p_p_i8_i8"
                            : "_F_hoo_Tensor_div_scalar_p_p_i8_i8");
                    default:
                        addError("Unsupported tensor-scalar binary operator");
                        return 0;
                }
            }
            const auto leftTensorInfo = inferExpressionTypeInfo(binary->getLeft());
            const auto rightTensorInfo = inferExpressionTypeInfo(binary->getRight());
            const auto isLowPrecisionTensorElement = [](uint32_t elementType) {
                return elementType == 5 || elementType == 6 || elementType == 8 || elementType == 9;
            };
            const bool lowPrecisionTensor =
                isLowPrecisionTensorElement(leftTensorInfo.elementTypeId) ||
                isLowPrecisionTensorElement(rightTensorInfo.elementTypeId);
            if (lowPrecisionTensor) {
                switch (binary->getOperator()) {
                    case ast::BinaryOperator::PLUS:
                        return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_add_p_p_p");
                    case ast::BinaryOperator::MINUS:
                        return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_sub_p_p_p");
                    case ast::BinaryOperator::ELEMENT_MULTIPLY:
                        return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_elementMul_p_p_p");
                    case ast::BinaryOperator::ELEMENT_DIVIDE:
                        return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_elementDiv_p_p_p");
                    case ast::BinaryOperator::MULTIPLY:
                        return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_matmul_p_p_p");
                    default:
                        break;
                }
            }
            switch (binary->getOperator()) {
                case ast::BinaryOperator::PLUS: return emitTensorVectorArith(*binary, Opcode::VECTOR_ARITH, 0);
                case ast::BinaryOperator::MINUS: return emitTensorVectorArith(*binary, Opcode::VECTOR_ARITH, 2);
                case ast::BinaryOperator::MULTIPLY: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_matmul_p_p_p");
                case ast::BinaryOperator::ELEMENT_MULTIPLY: return emitTensorVectorArith(*binary, Opcode::VECTOR_ARITH, 4);
                case ast::BinaryOperator::ELEMENT_DIVIDE: return emitTensorVectorArith(*binary, Opcode::VECTOR_ARITH, 6);
                case ast::BinaryOperator::EQUALS: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_eq_p_p_p");
                case ast::BinaryOperator::NOT_EQUALS: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_ne_p_p_p");
                case ast::BinaryOperator::LESS: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_lt_p_p_p");
                case ast::BinaryOperator::LESS_EQUALS: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_le_p_p_p");
                case ast::BinaryOperator::GREATER: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_gt_p_p_p");
                case ast::BinaryOperator::GREATER_EQUALS: return emitTensorBinaryCall(*binary, "_F_hoo_Tensor_ge_p_p_p");
                default:
                    addError("Unsupported tensor binary operator");
                    return 0;
            }
        }
        if (leftExprType == 0 || rightExprType == 0) {
            addError("Expression of type 'any' cannot be used in a binary expression");
            return 0;
        }
        const uint32_t leftType = leftExprType;
        const uint32_t rightType = rightExprType;
        const bool isFloatExpr = leftType == 2 || leftType == 9 || rightType == 2 || rightType == 9;
        const bool isUnsigned = leftType == 6 || rightType == 6;
        const bool isNativeByteCompare = leftType == 6 && rightType == 6;
        const bool isSubwordInt = ((leftType == 5 || leftType == 6) &&
                                   (rightType == 5 || rightType == 6)) ||
                                  (leftType == 8 && rightType == 8);
        const bool isNativeF8Arithmetic = leftType == 9 && rightType == 9 &&
            (binary->getOperator() == ast::BinaryOperator::PLUS ||
             binary->getOperator() == ast::BinaryOperator::MINUS ||
             binary->getOperator() == ast::BinaryOperator::MULTIPLY ||
             binary->getOperator() == ast::BinaryOperator::DIVIDE);
        const bool isStringConcat = binary->getOperator() == ast::BinaryOperator::PLUS
            && (leftType == 101 || rightType == 101);
        const bool isAnd = binary->getOperator() == ast::BinaryOperator::AND;
        const bool isOr = binary->getOperator() == ast::BinaryOperator::OR;

        if (isStringConcat) {
            uint8_t left = visitExpression(binary->getLeft());
            uint8_t right = visitExpression(binary->getRight());
            emit(Opcode::MOV, OperandsR{1, left, 0, 0});
            emit(Opcode::MOV, OperandsR{2, right, 0, 0});
            emitCall(Opcode::CALL, "_F_M_hoo_E_String_concat_p_p");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            if (isManagedTemporary(binary->getLeft())) {
                emit(Opcode::MOV, OperandsR{1, left, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                freeRegister(left);
            } else {
                freeRegister(left);
            }
            if (isManagedTemporary(binary->getRight())) {
                emit(Opcode::MOV, OperandsR{1, right, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                freeRegister(right);
            } else {
                freeRegister(right);
            }
            return dest;
        }

        if (isAnd || isOr) {
            uint8_t left = visitExpression(binary->getLeft());
            Label* skipLabel = createLabel();
            Label* endLabel = createLabel();
            uint8_t dest = allocateRegister();
            if (isAnd) {
                emitBranch(Opcode::BEQ, left, 0, skipLabel);
            } else {
                emitBranch(Opcode::BNE, left, 0, skipLabel);
            }
            uint8_t right = visitExpression(binary->getRight());
            emit(Opcode::MOV, OperandsR{dest, right, 0, 0});
            freeRegister(right);
            emitJump(Opcode::JMP, 0, endLabel);
            bindLabel(skipLabel);
            emit(Opcode::MOV, OperandsR{dest, left, 0, 0});
            bindLabel(endLabel);
            freeRegister(left);
            return dest;
        }

        uint8_t left = visitExpression(binary->getLeft());
        uint8_t right = visitExpression(binary->getRight());
        if (isNativeF8Arithmetic) {
            emit(Opcode::MOV, OperandsR{1, left, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
            emit(Opcode::MOV, OperandsR{left, 1, 0, 0});
            emit(Opcode::MOV, OperandsR{1, right, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
            emit(Opcode::MOV, OperandsR{right, 1, 0, 0});
        }
        uint8_t dest = allocateRegister();
        Opcode op = Opcode::ARITH;
        uint16_t func = 0;
        switch (binary->getOperator()) {
            case ast::BinaryOperator::PLUS:
                op = isNativeF8Arithmetic ? Opcode::FLOAT_ARITH_B : (isFloatExpr ? Opcode::FLOAT_ARITH : (isSubwordInt ? Opcode::ARITH_B : Opcode::ARITH)), func = 0;
                break;
            case ast::BinaryOperator::MINUS: op = isNativeF8Arithmetic ? Opcode::FLOAT_ARITH_B : (isFloatExpr ? Opcode::FLOAT_ARITH : (isSubwordInt ? Opcode::ARITH_B : Opcode::ARITH)); func = 1; break;
            case ast::BinaryOperator::MULTIPLY:
                op = isNativeF8Arithmetic ? Opcode::FLOAT_ARITH_B : (isFloatExpr ? Opcode::FLOAT_ARITH : (isSubwordInt ? Opcode::ARITH_B : Opcode::ARITH)), func = 2;
                break;
            case ast::BinaryOperator::DIVIDE:
                op = isNativeF8Arithmetic ? Opcode::FLOAT_ARITH_B : (isFloatExpr ? Opcode::FLOAT_ARITH : (isSubwordInt ? Opcode::ARITH_B : Opcode::ARITH));
                func = isFloatExpr ? 3 : (isSubwordInt && isUnsigned ? 6 : (isSubwordInt ? 5 : 5));
                break;
            case ast::BinaryOperator::MODULO:
                if (isFloatExpr) {
                    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, right, 0, 0});
                    emitCall(Opcode::CALL, "_F_M_hoo_E_math_fmod_d_p_p");
                    uint8_t fmodDest = allocateRegister();
                    emit(Opcode::MOV, OperandsR{fmodDest, 1, 0, 0});
                    freeRegister(left);
                    freeRegister(right);
                    return fmodDest;
                }
                op = isSubwordInt ? Opcode::ARITH_B : Opcode::ARITH;
                func = isSubwordInt && isUnsigned ? 8 : 7; break;
            case ast::BinaryOperator::SHIFT_LEFT:
                op = isSubwordInt ? Opcode::SHIFT_B : Opcode::SHIFT;
                func = 0; break;
            case ast::BinaryOperator::SHIFT_RIGHT:
                op = isSubwordInt ? Opcode::SHIFT_B : Opcode::SHIFT;
                func = 1; break;
            case ast::BinaryOperator::EQUALS: op = isFloatExpr ? Opcode::FCMP : (isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP); func = 0; break;
            case ast::BinaryOperator::NOT_EQUALS: op = isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP; func = 1; break;
            case ast::BinaryOperator::LESS: op = isFloatExpr ? Opcode::FCMP : (isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP); func = isFloatExpr ? 1 : (isUnsigned ? 4 : 2); break;
            case ast::BinaryOperator::LESS_EQUALS: op = isFloatExpr ? Opcode::FCMP : (isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP); func = isFloatExpr ? 2 : (isUnsigned ? 5 : 3); break;
            case ast::BinaryOperator::GREATER: {
                op = isFloatExpr ? Opcode::FCMP : (isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP);
                func = isFloatExpr ? 1 : (isUnsigned ? 4 : 2);
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::GREATER_EQUALS: {
                op = isFloatExpr ? Opcode::FCMP : (isNativeByteCompare ? Opcode::CMP_B : Opcode::CMP);
                func = isFloatExpr ? 2 : (isUnsigned ? 5 : 3);
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::AND: op = Opcode::LOGIC; func = 0; break;
            case ast::BinaryOperator::OR:  op = Opcode::LOGIC; func = 1; break;
            default: addError("Unsupported binary operator");
        }
        emit(op, OperandsR{dest, left, right, func});
        if (isNativeF8Arithmetic) {
            emit(Opcode::MOV, OperandsR{1, dest, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_decode_d_i1");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        } else if (isSubwordInt && (binary->getOperator() == ast::BinaryOperator::PLUS ||
                                    binary->getOperator() == ast::BinaryOperator::MINUS ||
                                    binary->getOperator() == ast::BinaryOperator::MULTIPLY ||
                                    binary->getOperator() == ast::BinaryOperator::DIVIDE ||
                                    binary->getOperator() == ast::BinaryOperator::MODULO ||
                                    binary->getOperator() == ast::BinaryOperator::SHIFT_LEFT ||
                                    binary->getOperator() == ast::BinaryOperator::SHIFT_RIGHT)) {
            if (leftType == 5 && rightType == 5) {
                uint8_t shift = emitConstant(56);
                emit(Opcode::SHIFT, OperandsR{dest, dest, shift, 0});
                emit(Opcode::SHIFT, OperandsR{dest, dest, shift, 2});
                freeRegister(shift);
            } else {
                uint8_t mask = emitConstant(0xFF);
                emit(Opcode::LOGIC, OperandsR{dest, dest, mask, 0});
                freeRegister(mask);
            }
        }
        freeRegister(left);
        freeRegister(right);
        return dest;
    }

    if (auto logicAnd = dynamic_cast<const ast::LogicalAnd*>(&expr)) {
        const uint32_t leftType = inferExpressionTypeId(logicAnd->getLeft());
        const uint32_t rightType = inferExpressionTypeId(logicAnd->getRight());
        if (leftType == 0 || rightType == 0) {
            addError("Expression of type 'any' cannot be used in a logical expression");
            return 0;
        }
        uint8_t left = visitExpression(logicAnd->getLeft());
        uint8_t dest = allocateRegister();
        if (leftType == 104 || rightType == 104) {
            uint8_t right = visitExpression(logicAnd->getRight());
            emit(Opcode::MOV, OperandsR{1, left, 0, 0});
            emit(Opcode::MOV, OperandsR{2, right, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Tensor_and_p_p_p");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(right);
        } else {
            Label* skipLabel = createLabel();
            Label* endLabel = createLabel();
            emitBranch(Opcode::BEQ, left, 0, skipLabel);
            uint8_t right = visitExpression(logicAnd->getRight());
            emit(Opcode::MOV, OperandsR{dest, right, 0, 0});
            freeRegister(right);
            emitJump(Opcode::JMP, 0, endLabel);
            bindLabel(skipLabel);
            emit(Opcode::MOVZ, OperandsI{dest, 0, 0});
            bindLabel(endLabel);
        }
        freeRegister(left);
        return dest;
    }
    if (auto logicOr = dynamic_cast<const ast::LogicalOr*>(&expr)) {
        const uint32_t leftType = inferExpressionTypeId(logicOr->getLeft());
        const uint32_t rightType = inferExpressionTypeId(logicOr->getRight());
        if (leftType == 0 || rightType == 0) {
            addError("Expression of type 'any' cannot be used in a logical expression");
            return 0;
        }
        uint8_t left = visitExpression(logicOr->getLeft());
        uint8_t dest = allocateRegister();
        if (leftType == 104 || rightType == 104) {
            uint8_t right = visitExpression(logicOr->getRight());
            emit(Opcode::MOV, OperandsR{1, left, 0, 0});
            emit(Opcode::MOV, OperandsR{2, right, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Tensor_or_p_p_p");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(right);
        } else {
            Label* skipLabel = createLabel();
            Label* endLabel = createLabel();
            emitBranch(Opcode::BNE, left, 0, skipLabel);
            uint8_t right = visitExpression(logicOr->getRight());
            emit(Opcode::MOV, OperandsR{dest, right, 0, 0});
            freeRegister(right);
            emitJump(Opcode::JMP, 0, endLabel);
            bindLabel(skipLabel);
            emit(Opcode::MOV, OperandsR{dest, left, 0, 0});
            bindLabel(endLabel);
        }
        freeRegister(left);
        return dest;
    }

    if (auto logicalNot = dynamic_cast<const ast::LogicalNot*>(&expr)) {
        uint8_t src = visitExpression(logicalNot->getOperand());
        uint8_t dest = allocateRegister();
        if (inferExpressionTypeId(logicalNot->getOperand()) == 8) {
            emit(Opcode::LOGIC_B, OperandsR{dest, src, 0, 2});
        } else if (inferExpressionTypeId(logicalNot->getOperand()) == 104) {
            emit(Opcode::MOV, OperandsR{1, src, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Tensor_not_p_p");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        } else {
            emit(Opcode::CMP, OperandsR{dest, src, 0, 0});
        }
        freeRegister(src);
        return dest;
    }

    if (auto unaryMinus = dynamic_cast<const ast::UnaryMinus*>(&expr)) {
        uint8_t src = visitExpression(unaryMinus->getOperand());
        uint8_t dest = allocateRegister();
        const uint32_t operandType = inferExpressionTypeId(unaryMinus->getOperand());
        if (operandType == 125) { // Decimal
            emit(Opcode::MOV, OperandsR{1, src, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Decimal_neg_p_p");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        } else if (operandType == 2) {
            uint8_t zero = emitConstant(0);
            emit(Opcode::FLOAT_ARITH, OperandsR{dest, zero, src, 1});
            freeRegister(zero);
        } else if (operandType == 9) {
            uint8_t zero = emitConstant(0.0);
            emit(Opcode::MOV, OperandsR{1, zero, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
            uint8_t zeroF8 = allocateRegister();
            emit(Opcode::MOV, OperandsR{zeroF8, 1, 0, 0});
            emit(Opcode::MOV, OperandsR{1, src, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
            uint8_t srcF8 = allocateRegister();
            emit(Opcode::MOV, OperandsR{srcF8, 1, 0, 0});
            emit(Opcode::FLOAT_ARITH_B, OperandsR{dest, zeroF8, srcF8, 1});
            emit(Opcode::MOV, OperandsR{1, dest, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_f8_decode_d_i1");
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(srcF8);
            freeRegister(zeroF8);
            freeRegister(zero);
        } else if (operandType == 5) {
            uint8_t zero = emitConstant(0);
            emit(Opcode::ARITH_B, OperandsR{dest, zero, src, 1});
            uint8_t shift = emitConstant(56);
            emit(Opcode::SHIFT, OperandsR{dest, dest, shift, 0});
            emit(Opcode::SHIFT, OperandsR{dest, dest, shift, 2});
            freeRegister(shift);
            freeRegister(zero);
        } else {
            emit(Opcode::ARITH, OperandsR{dest, 0, src, 1});
        }
        freeRegister(src);
        return dest;
    }

    if (auto compoundAssign = dynamic_cast<const ast::CompoundAssignmentExpression*>(&expr)) {
        uint8_t rhsReg = visitExpression(compoundAssign->getRight());
        uint8_t lhsReg = 0;
        int32_t offset = 0;
        uint8_t objReg = 0;
        bool isMember = false;

        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&compoundAssign->getLeft())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                offset = getLocalOffset(id->getName());
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, 30, static_cast<int16_t>(offset)});
            }
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&compoundAssign->getLeft())) {
            objReg = visitExpression(leftMember->getObject());
            isMember = true;
            std::string foundClass;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    foundClass = className;
                    break;
                }
            }
            if (!foundClass.empty()) {
                auto classIt = classes_.find(foundClass);
                if (classIt != classes_.end() && classIt->second.isImmutable && !inConstructor_) {
                    addError("Cannot modify field '" + leftMember->getMember() + "' of immutable class '" + foundClass + "'");
                }
                if (!canWriteField(leftMember->getMember(), foundClass)) {
                    addError("Cannot write to field '" + leftMember->getMember() + "' of class '" + foundClass + "'");
                }
                lhsReg = allocateRegister();
                uint8_t offsetReg = emitConstant(static_cast<int64_t>(offset));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, offsetReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_get_field_p_i8");
                freeRegister(offsetReg);
                emit(Opcode::MOV, OperandsR{lhsReg, 1, 0, 0});
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
        }

        if (lhsReg != 0) {
            uint8_t resultReg = allocateRegister();
            uint16_t func = 0;
            Opcode op = Opcode::ARITH;
            switch (compoundAssign->getOperator()) {
                case ast::CompoundAssignmentOperator::PLUS_ASSIGN: func = 0; break;
                case ast::CompoundAssignmentOperator::MINUS_ASSIGN: func = 1; break;
                case ast::CompoundAssignmentOperator::MULTIPLY_ASSIGN: func = 2; break;
                case ast::CompoundAssignmentOperator::DIVIDE_ASSIGN: func = 5; break;
                case ast::CompoundAssignmentOperator::MODULO_ASSIGN:
                    {
                        const uint32_t lhsType = inferExpressionTypeId(compoundAssign->getLeft());
                        if (lhsType == 2 || lhsType == 9) {
                            emit(Opcode::MOV, OperandsR{1, lhsReg, 0, 0});
                            emit(Opcode::MOV, OperandsR{2, rhsReg, 0, 0});
                            emitCall(Opcode::CALL, "_F_M_hoo_E_math_fmod_d_p_p");
                            emit(Opcode::MOV, OperandsR{resultReg, 1, 0, 0});
                            freeRegister(lhsReg);
                            freeRegister(rhsReg);
                            if (isMember) {
                                uint8_t setOffsetReg = emitConstant(static_cast<int64_t>(offset));
                                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                                emit(Opcode::MOV, OperandsR{2, setOffsetReg, 0, 0});
                                emit(Opcode::MOV, OperandsR{3, resultReg, 0, 0});
                                emitCall(Opcode::CALL, "_F_object_set_field_v_p_i8_p");
                                freeRegister(setOffsetReg);
                                freeRegister(objReg);
                            } else {
                                emit(Opcode::ST_D, OperandsI{resultReg, 30, static_cast<int16_t>(offset)});
                            }
                            return resultReg;
                        }
                        func = 7;
                    }
                    break;
                case ast::CompoundAssignmentOperator::LEFT_SHIFT_ASSIGN: op = Opcode::SHIFT; func = 0; break;
                case ast::CompoundAssignmentOperator::RIGHT_SHIFT_ASSIGN: op = Opcode::SHIFT; func = 1; break;
                default: addError("Unsupported compound assignment");
            }

            const uint32_t lhsType = inferExpressionTypeId(compoundAssign->getLeft());
            const uint32_t rhsType = inferExpressionTypeId(compoundAssign->getRight());
            const bool isSubwordInt =
                ((lhsType == 5 || lhsType == 6) && (rhsType == 5 || rhsType == 6)) ||
                (lhsType == 8 && rhsType == 8);
            const bool isNativeF8 = lhsType == 9 && rhsType == 9 &&
                compoundAssign->getOperator() != ast::CompoundAssignmentOperator::MODULO_ASSIGN &&
                compoundAssign->getOperator() != ast::CompoundAssignmentOperator::LEFT_SHIFT_ASSIGN &&
                compoundAssign->getOperator() != ast::CompoundAssignmentOperator::RIGHT_SHIFT_ASSIGN;
            bool nativeF8Lowered = false;
            if (isSubwordInt && op == Opcode::ARITH) {
                op = Opcode::ARITH_B;
                if (lhsType == 6 && compoundAssign->getOperator() == ast::CompoundAssignmentOperator::DIVIDE_ASSIGN) func = 6;
                if (lhsType == 6 && compoundAssign->getOperator() == ast::CompoundAssignmentOperator::MODULO_ASSIGN) func = 8;
            }
            if (isSubwordInt && op == Opcode::SHIFT) {
                op = Opcode::SHIFT_B;
            }
            if (isNativeF8 && op == Opcode::ARITH) {
                if (compoundAssign->getOperator() == ast::CompoundAssignmentOperator::DIVIDE_ASSIGN) func = 3;
                emit(Opcode::MOV, OperandsR{1, lhsReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
                uint8_t lhsF8 = allocateRegister();
                emit(Opcode::MOV, OperandsR{lhsF8, 1, 0, 0});
                emit(Opcode::MOV, OperandsR{1, rhsReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_f8_encode_i1_d");
                uint8_t rhsF8 = allocateRegister();
                emit(Opcode::MOV, OperandsR{rhsF8, 1, 0, 0});
                emit(Opcode::FLOAT_ARITH_B, OperandsR{resultReg, lhsF8, rhsF8, func});
                emit(Opcode::MOV, OperandsR{1, resultReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_f8_decode_d_i1");
                emit(Opcode::MOV, OperandsR{resultReg, 1, 0, 0});
                freeRegister(rhsF8);
                freeRegister(lhsF8);
                nativeF8Lowered = true;
            }
            if (!nativeF8Lowered) {
                emit(op, OperandsR{resultReg, lhsReg, rhsReg, func});
            }

            if (op == Opcode::ARITH_B || op == Opcode::SHIFT_B) {
                if (lhsType == 5) {
                    uint8_t shift = emitConstant(56);
                    emit(Opcode::SHIFT, OperandsR{resultReg, resultReg, shift, 0});
                    emit(Opcode::SHIFT, OperandsR{resultReg, resultReg, shift, 2});
                    freeRegister(shift);
                } else {
                    uint8_t mask = emitConstant(op == Opcode::SHIFT_B && lhsType == 8 ? 1 : 0xFF);
                    emit(Opcode::LOGIC, OperandsR{resultReg, resultReg, mask, 0});
                    freeRegister(mask);
                }
            }
            
            if (isMember) {
                uint8_t setOffsetReg = emitConstant(static_cast<int64_t>(offset));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, setOffsetReg, 0, 0});
                emit(Opcode::MOV, OperandsR{3, resultReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_set_field_v_p_i8_p");
                freeRegister(setOffsetReg);
                freeRegister(objReg);
            } else {
                emit(Opcode::ST_D, OperandsI{resultReg, 30, static_cast<int16_t>(offset)});
            }
            
            freeRegister(lhsReg);
            freeRegister(rhsReg);
            return resultReg;
        }
        freeRegister(rhsReg);
        return 0;
    }

    if (auto incDec = dynamic_cast<const ast::IncrementDecrementExpression*>(&expr)) {
        uint8_t lhsReg = 0;
        int32_t offset = 0;
        uint8_t objReg = 0;
        bool isMember = false;
        std::string varName;

        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&incDec->getOperand())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                varName = id->getName();
                offset = getLocalOffset(varName);
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, 30, static_cast<int16_t>(offset)});
            }
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&incDec->getOperand())) {
            objReg = visitExpression(leftMember->getObject());
            isMember = true;
            std::string foundClass;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    foundClass = className;
                    break;
                }
            }
            if (!foundClass.empty()) {
                auto classIt = classes_.find(foundClass);
                if (classIt != classes_.end() && classIt->second.isImmutable && !inConstructor_) {
                    addError("Cannot modify field '" + leftMember->getMember() + "' of immutable class '" + foundClass + "'");
                }
                if (!canWriteField(leftMember->getMember(), foundClass)) {
                    addError("Cannot write to field '" + leftMember->getMember() + "' of class '" + foundClass + "'");
                }
                lhsReg = allocateRegister();
                uint8_t offsetReg = emitConstant(static_cast<int64_t>(offset));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, offsetReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_get_field_p_i8");
                freeRegister(offsetReg);
                emit(Opcode::MOV, OperandsR{lhsReg, 1, 0, 0});
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
        }

        if (lhsReg != 0) {
            uint8_t oneReg = emitConstant(1);
            uint8_t resultReg = allocateRegister();
            uint16_t func = (incDec->getOperator() == ast::IncrementDecrementOperator::INCREMENT) ? 0 : 1;
            emit(Opcode::ARITH, OperandsR{resultReg, lhsReg, oneReg, func});
            
            if (isMember) {
                uint8_t setOffsetReg = emitConstant(static_cast<int64_t>(offset));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, setOffsetReg, 0, 0});
                emit(Opcode::MOV, OperandsR{3, resultReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_set_field_v_p_i8_p");
                freeRegister(setOffsetReg);
                freeRegister(objReg);
            } else {
                emit(Opcode::ST_D, OperandsI{resultReg, 30, static_cast<int16_t>(offset)});
            }
            
            freeRegister(oneReg);
            if (incDec->isPrefix()) {
                freeRegister(lhsReg);
                return resultReg;
            } else {
                freeRegister(resultReg);
                return lhsReg;
            }
        }
        return 0;
    }

    if (auto assign = dynamic_cast<const ast::AssignmentExpression*>(&expr)) {
        uint8_t valueReg = visitExpression(assign->getRight());
        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&assign->getLeft())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                int32_t offset = getLocalOffset(id->getName());
                uint32_t oldTypeId = getLocalTypeId(id->getName());
                validateAssignmentNullSafety(getLocalIsNullable(id->getName()), oldTypeId,
                                             &assign->getRight(), id->getName());
                if (oldTypeId >= 100) {
                    emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(offset)});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                }
                emit(Opcode::ST_D, OperandsI{valueReg, 30, static_cast<int16_t>(offset)});
                return valueReg;
            }
        } else if (auto leftArray = dynamic_cast<const ast::ArrayAccess*>(&assign->getLeft())) {
            const auto leftArrayInfo = inferExpressionTypeInfo(leftArray->getArray());
            uint8_t objReg = visitExpression(leftArray->getArray());
            if (leftArrayInfo.isNullable) {
                emitNullCheck(objReg);
            }
            uint8_t idxReg = visitExpression(leftArray->getIndex());
            uint32_t sourceTypeId = 0;
            uint32_t valueTypeId = getTypeId(nullptr, &assign->getRight());
            uint32_t mapValueTypeId = 0;
            if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&leftArray->getArray())) {
                if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                    sourceTypeId = getLocalTypeId(id->getName());
                    mapValueTypeId = getLocalElementTypeId(id->getName());
                }
            }

            if (sourceTypeId == 118) {
                uint8_t typeReg = emitConstant(static_cast<int64_t>(valueTypeId));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
                emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                emit(Opcode::MOV, OperandsR{5, valueReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_list_set_i8_p_i8_i8_i8");
                freeRegister(typeReg);
            } else if (sourceTypeId == 117) {
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
                if (mapValueTypeId == 0) {
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(valueTypeId));
                    emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{5, valueReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_dict_set_any_i8_p_i8_i8_i8");
                    freeRegister(typeReg);
                } else {
                    emit(Opcode::MOV, OperandsR{3, valueReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_dict_set_fixed_i8_p_i8_i8");
                }
            } else {
                addError("Unsupported indexed assignment target");
            }
            freeRegister(objReg);
            freeRegister(idxReg);
            return valueReg;
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&assign->getLeft())) {
            const auto leftMemberInfo = inferExpressionTypeInfo(leftMember->getObject());
            uint8_t objReg = visitExpression(leftMember->getObject());
            if (leftMemberInfo.isNullable) {
                emitNullCheck(objReg);
            }
            int32_t offset = 0;
            std::string foundClass;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    foundClass = className;
                    break;
                }
            }
            if (!foundClass.empty()) {
                auto classIt = classes_.find(foundClass);
                if (classIt != classes_.end() && classIt->second.isImmutable && !inConstructor_) {
                    addError("Cannot modify field '" + leftMember->getMember() + "' of immutable class '" + foundClass + "'");
                }
                if (!canWriteField(leftMember->getMember(), foundClass)) {
                    addError("Cannot write to field '" + leftMember->getMember() + "' of class '" + foundClass + "'");
                }
                if (classIt != classes_.end()) {
                    bool fieldNullable = false;
                    auto fIt = classIt->second.fieldIsNullable.find(leftMember->getMember());
                    if (fIt != classIt->second.fieldIsNullable.end()) fieldNullable = fIt->second;
                    uint32_t fieldTypeId = 100;
                    auto tfIt = classIt->second.fieldTypeIds.find(leftMember->getMember());
                    if (tfIt != classIt->second.fieldTypeIds.end()) fieldTypeId = tfIt->second;
                    validateAssignmentNullSafety(fieldNullable, fieldTypeId, &assign->getRight(), leftMember->getMember());
                }
                uint8_t setOffsetReg = emitConstant(static_cast<int64_t>(offset));
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, setOffsetReg, 0, 0});
                emit(Opcode::MOV, OperandsR{3, valueReg, 0, 0});
                emitCall(Opcode::CALL, "_F_object_set_field_v_p_i8_p");
                freeRegister(setOffsetReg);
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
            freeRegister(objReg);
            return valueReg;
        }
        addError("Unsupported assignment target");
        return valueReg;
    }

    if (auto arrayAccess = dynamic_cast<const ast::ArrayAccess*>(&expr)) {
        const auto arrayInfo = inferExpressionTypeInfo(arrayAccess->getArray());
        uint8_t arrReg = visitExpression(arrayAccess->getArray());
        if (arrayInfo.isNullable) {
            emitNullCheck(arrReg);
        }
        uint8_t idxReg = visitExpression(arrayAccess->getIndex());
        uint32_t sourceTypeId = 0;
        uint32_t elementTypeId = 0;
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&arrayAccess->getArray())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                sourceTypeId = getLocalTypeId(id->getName());
                elementTypeId = getLocalElementTypeId(id->getName());
            }
        }

        if (sourceTypeId == 118) {
            emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
            emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_list_get_data_i8_p_i8");
            freeRegister(arrReg);
            freeRegister(idxReg);
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        }

        if (sourceTypeId == 117) {
            emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
            emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
            if (elementTypeId == 0) {
                emitCall(Opcode::CALL, "_F_hoo_dict_get_any_data_i8_p_i8");
            } else {
                emitCall(Opcode::CALL, "_F_hoo_dict_get_fixed_data_i8_p_i8");
            }
            freeRegister(arrReg);
            freeRegister(idxReg);
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            return dest;
        }
        
        // Bounds check: compare idx against length via runtime call
        uint8_t lenReg = allocateRegister();
        emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
        emitCall(Opcode::CALL, sourceTypeId == 104 ? "_F_hoo_Tensor_length_i8_p" : "_F_array_length_v_p");
        emit(Opcode::MOV, OperandsR{lenReg, 1, 0, 0});
        uint8_t cmpReg = allocateRegister();
        emit(Opcode::CMP, OperandsR{cmpReg, idxReg, lenReg, 1}); // 1 = BLT (idx < len)
        freeRegister(lenReg);
        
        Label* trapLabel = createLabel();
        Label* afterAccess = createLabel();
        emitBranch(Opcode::BEQ, cmpReg, 0, trapLabel); // if idx >= len, trap
        freeRegister(cmpReg);
        
        // Access element via runtime call
        emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
        if (sourceTypeId == 104 && (elementTypeId == 2 || elementTypeId == 9)) {
            emitCall(Opcode::CALL, "_F_hoo_Tensor_getDouble_d_p_i8");
        } else if (sourceTypeId == 104) {
            emitCall(Opcode::CALL, "_F_hoo_Tensor_getInt64_i8_p_i8");
        } else if (elementTypeId == 2 || elementTypeId == 9) {
            emitCall(Opcode::CALL, "_F_array_get_double_v_p_p");
        } else if (elementTypeId == 3 || elementTypeId == 8) {
            emitCall(Opcode::CALL, "_F_array_get_bool_v_p_p");
        } else {
            emitCall(Opcode::CALL, "_F_array_get_int64_v_p_p");
        }
        freeRegister(arrReg);
        freeRegister(idxReg);
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        
        emitJump(Opcode::JMP, 0, afterAccess);
        
        // Out-of-bounds trap
        bindLabel(trapLabel);
        emitCall(Opcode::CALL, "_F_hoo_exception_runtime_p");
        emit(Opcode::MOV, OperandsR{2, 1, 0, 0});
        emit(Opcode::SYSCALL, OperandsI{0, 0, 9});
        
        bindLabel(afterAccess);
        return dest;
    }

    addError("Unsupported expression type: " + std::string(typeid(expr).name()));
    return 0;
}

void HVMCodeGenerator::addError(const std::string& message) {
    errors_.push_back(message);
}

uint8_t HVMCodeGenerator::allocateRegister() {
    for (uint8_t i = 9; i <= 20; ++i) {
        if (!usedRegs_[i]) {
            usedRegs_[i] = true;
            return i;
        }
    }
    // Register exhaustion is a compiler bug - fail hard rather than silently
    // using r0 (hardwired zero).
    addError("Register pressure: out of temporary registers");
    return 0;
}

void HVMCodeGenerator::freeRegister(uint8_t reg) {
    if (reg >= 9 && reg <= 20) usedRegs_[reg] = false;
}

HVMCodeGenerator::RegisterMask HVMCodeGenerator::captureRegisterMask() const {
    RegisterMask mask;
    for (uint8_t i = 0; i < 32; ++i) {
        mask.set(i, usedRegs_[i]);
    }
    return mask;
}

void HVMCodeGenerator::restoreRegisterMask(const RegisterMask& mask) {
    for (uint8_t i = 9; i <= 20; ++i) {
        usedRegs_[i] = mask.test(i);
    }
}

void HVMCodeGenerator::emitCompressed(uint8_t opcode4, uint8_t rd, uint8_t rs1, uint8_t imm4) {
    // Layout: byte0 = (imm4 << 4) | (opcode4 & 0x0F)
    //          byte1 = (rd << 4) | (rs1 & 0x0F)
    uint8_t byte0 = static_cast<uint8_t>((imm4 << 4) | (opcode4 & 0x0F));
    uint8_t byte1 = static_cast<uint8_t>((rd << 4) | (rs1 & 0x0F));
    compressedInstructions_.push_back(byte0);
    compressedInstructions_.push_back(byte1);
    // Update offset tracking (compressed instructions are 2 bytes each)
    currentByteOffset_ += 2;
}

int32_t HVMCodeGenerator::reserveLocal(const std::string& name, uint32_t typeId, const std::string& className, uint32_t elementTypeId, uint32_t keyTypeId, bool isNullable) {
    currentStackOffset_ -= 8;
    if (scopeStack_.empty()) scopeStack_.push_back({});
    // A nullable local declared with a named reference type keeps its class
    // name even when the type ID resolves to the generic-object slot (100).
    // Such a slot still holds a hoo_alloc-managed object and must be released
    // by scope cleanup, otherwise non-null values leak on scope exit.
    bool arcManaged = isArcManagedTypeId(typeId);
    if (!arcManaged && isNullable) {
        arcManaged = isHooReleaseManagedClassName(className);
    }
    scopeStack_.back()[name] = {currentStackOffset_, typeId, className, elementTypeId, keyTypeId,
                                isNullable, arcManaged, "", false};
    return currentStackOffset_;
}

static uint32_t dictKeyTypeId(const ast::DictType& type) {
    switch (type.getKeyType()) {
        case ast::DictKeyType::INT64: return 1;
        case ast::DictKeyType::INT8: return 5;
        case ast::DictKeyType::BYTE: return 6;
    }
    return 1;
}

static uint32_t mapKeyTypeId(const ast::MapType& type) {
    switch (type.getKeyType()) {
        case ast::MapKeyType::INT64: return 1;
        case ast::MapKeyType::INT8: return 5;
        case ast::MapKeyType::BYTE: return 6;
        case ast::MapKeyType::CHAR: return 7;
        case ast::MapKeyType::STRING: return 101;
    }
    return 0;
}

static int64_t integerLiteralValue(const ast::Expression& expr, int64_t fallback) {
    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        if (auto il = dynamic_cast<const ast::IntegerLiteral*>(&pe->getPrimary())) {
            return il->getValue();
        }
    }
    return fallback;
}

static uint32_t mapConstructorKeyTypeId(const ast::NewObjectExpression& expr) {
    const auto* args = expr.getArguments();
    if (!args || args->getArguments().empty()) return 0;
    switch (integerLiteralValue(*args->getArguments()[0], -1)) {
        case 0: return 6;   // HOO_MAP_KEY_BYTE
        case 1: return 5;   // HOO_MAP_KEY_INT8
        case 2: return 1;   // HOO_MAP_KEY_INT64
        case 3: return 7;   // HOO_MAP_KEY_CHAR
        case 4: return 101; // HOO_MAP_KEY_STRING
        default: return 0;
    }
}

static uint32_t mapConstructorValueTypeId(const ast::NewObjectExpression& expr) {
    const auto* args = expr.getArguments();
    if (!args || args->getArguments().size() < 2) return 0;
    switch (integerLiteralValue(*args->getArguments()[1], -1)) {
        case 0: return 100; // HOO_MAP_VAL_ANY
        case 1: return 1;   // HOO_MAP_VAL_INT64
        case 2: return 2;   // HOO_MAP_VAL_DOUBLE
        case 3: return 3;   // HOO_MAP_VAL_BOOL
        case 4: return 101; // HOO_MAP_VAL_STRING
        case 5: return 100; // HOO_MAP_VAL_OBJECT
        case 6: return 5;   // HOO_MAP_VAL_INT8
        case 7: return 7;   // HOO_MAP_VAL_CHAR
        default: return 0;
    }
}

int32_t HVMCodeGenerator::getLocalOffset(const std::string& name) {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.offset;
    }
    addError("Undefined variable: " + name);
    return 0;
}

uint32_t HVMCodeGenerator::getLocalTypeId(const std::string& name) const {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.typeId;
    }
    return 0;
}

std::string HVMCodeGenerator::getLocalClassName(const std::string& name) const {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.className;
    }
    return "";
}

uint32_t HVMCodeGenerator::getLocalElementTypeId(const std::string& name) const {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.elementTypeId;
    }
    return 0;
}

uint32_t HVMCodeGenerator::getLocalKeyTypeId(const std::string& name) const {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.keyTypeId;
    }
    return 0;
}

bool HVMCodeGenerator::getLocalIsNullable(const std::string& name) const {
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.isNullable;
    }
    return false;
}

bool HVMCodeGenerator::isNullableDeclaredType(const ast::Type* type) {
    if (!type) return false;
    auto opt = dynamic_cast<const ast::OptionalType*>(type);
    return opt && opt->isOptional();
}

bool HVMCodeGenerator::isNullLiteralExpression(const ast::Expression* expr) {
    if (!expr) return false;
    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(expr)) {
        return dynamic_cast<const ast::NullLiteral*>(&pe->getPrimary()) != nullptr;
    }
    return false;
}

void HVMCodeGenerator::validateAssignmentNullSafety(bool targetNullable, uint32_t targetTypeId,
                                                    const ast::Expression* value, const std::string& targetName) {
    if (targetNullable) return;
    if (targetTypeId == 0) return; // untyped / any target accepts anything
    if (!value) return;
    const auto valueInfo = inferExpressionTypeInfo(*value);
    if (!valueInfo.isNullable) return;
    const bool isNullLiteral = isNullLiteralExpression(value);
    if (targetTypeId >= 100) {
        // Reference-typed target: the literal null is a valid reference value,
        // but a possibly-null value must not flow into a non-nullable slot
        // because a later dereference would be unsafe.
        if (!isNullLiteral) {
            addError("Cannot assign a nullable value to non-nullable '" + targetName +
                     "'; declare it as '" + targetName + "?' or check for null first");
        }
        return;
    }
    // Value-typed (primitive) target: null and nullable values are rejected.
    addError("Cannot assign " + std::string(isNullLiteral ? "null" : "a nullable value") +
             " to non-nullable '" + targetName + "'; declare it as a nullable type (T?)");
}

bool HVMCodeGenerator::isBuiltinClassName(const std::string& name) const {
    static const std::unordered_set<std::string> builtinClasses = {
        "String", "Array", "Map", "Exception", "Character",
        "DateTime", "Fs", "Thread", "Regex",
        "Net", "URL", "HttpClient", "HttpResponse",
        "Path", "Uuid", "Compression",
        "Args", "Csv", "Console", "StringBuilder",
        "Buffer", "Random", "Dict", "List",
        "Mutex", "Decimal"
        , "Condition", "Semaphore"
    };
    return builtinClasses.count(name) > 0;
}

void HVMCodeGenerator::emit(Opcode op, const Operands& operands) {
    // Regular 32‑bit/escape emission
    HVMInstruction inst(op, operands);
    instructions_.push_back(inst);
    currentByteOffset_ += inst.getSize();
}





uint8_t HVMCodeGenerator::emitConstant(int64_t value) {
    uint8_t reg = allocateRegister();
    
    if (value >= 0 && value <= 32767) {
        emit(Opcode::MOVZ, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else if (value >= -16384 && value <= 16383) {
        emit(Opcode::ADDI, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else {
        Section* rodata = module_->getSection(".rodata");
        if (!rodata) {
            Section s;
            s.name = ".rodata";
            s.type = SectionType::SHT_RODATA;
            s.flags = SectionFlags::ALLOC;
            module_->addSection(std::move(s));
            rodata = module_->getSection(".rodata");
        }
        
        uint32_t offset = static_cast<uint32_t>(rodata->data.size());
        for (int i = 0; i < 8; ++i) {
            rodata->data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
        rodata->virtual_size = rodata->data.size();
        
        uint8_t addrReg = emitRoDataAddress(offset);
        emit(Opcode::LD_D, OperandsI{reg, addrReg, 0});
        freeRegister(addrReg);
    }
    return reg;
}

uint8_t HVMCodeGenerator::emitRoDataAddress(uint32_t offset) {
    // LDA with rs=r0 is interpreted as .rodata-base addressing by the HVM runtime.
    // For large offsets, build the full address with bounded ADDI steps.
    uint8_t addrReg = allocateRegister();
    const int64_t kLdaMax = 16383;
    const int64_t kAddiMax = 16383;

    int64_t remaining = static_cast<int64_t>(offset);
    int16_t baseImm = static_cast<int16_t>(std::min<int64_t>(remaining, kLdaMax));
    emit(Opcode::LDA, OperandsI{addrReg, 0, baseImm});
    remaining -= baseImm;

    while (remaining > 0) {
        int16_t step = static_cast<int16_t>(std::min<int64_t>(remaining, kAddiMax));
        emit(Opcode::ADDI, OperandsI{addrReg, addrReg, step});
        remaining -= step;
    }

    return addrReg;
}

HVMCodeGenerator::Label* HVMCodeGenerator::createLabel() {
    allLabels_.push_back(std::make_unique<Label>());
    return allLabels_.back().get();
}

void HVMCodeGenerator::bindLabel(Label* label) {
    label->targetByteOffset = static_cast<int32_t>(currentByteOffset_);
    for (const auto& fixup : label->fixups) {
        auto& inst = instructions_[fixup.instructionIndex];
        int32_t byteOffset = label->targetByteOffset - static_cast<int32_t>(fixup.instructionByteOffset);
        int32_t wordOffset = byteOffset / 4;
        auto operands = inst.getOperands();
        if (std::holds_alternative<OperandsB>(operands)) {
            auto& ops = std::get<OperandsB>(operands);
            ops.imm15 = static_cast<int16_t>(wordOffset);
            inst.setOperands(ops);
        } else if (std::holds_alternative<OperandsJ>(operands)) {
            auto& ops = std::get<OperandsJ>(operands);
            ops.offset = wordOffset;
            inst.setOperands(ops);
        }
    }
    label->fixups.clear();
}

void HVMCodeGenerator::emitJump(Opcode op, uint8_t rd, Label* target) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;
    if (target->targetByteOffset != -1) {
        int32_t wordOffset = (target->targetByteOffset - static_cast<int32_t>(instrOff)) / 4;
        emit(op, OperandsJ{rd, wordOffset});
    } else {
        target->fixups.push_back({instrIdx, instrOff});
        emit(op, OperandsJ{rd, 0});
    }
}

void HVMCodeGenerator::emitCall(Opcode op, const std::string& symbol) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;
    
    auto* sym = module_->getSymbol(symbol);
    
    // If it's an external symbol (not found or already marked as undefined)
    // we add a new symbol record for this specific call site so the JIT can resolve it.
    if (!sym || sym->section_index == -1) {
        Symbol undefSym;
        undefSym.name = symbol;
        undefSym.type = Symbol::STT_FUNC;
        undefSym.binding = Symbol::STB_GLOBAL;
        undefSym.section_index = -1; // Undefined
        undefSym.value = instrOff;   // Target this specific instruction for JIT resolution
        module_->addSymbol(undefSym);
        sym = &module_->getSymbols().back();
    }

    int32_t wordOffset = 0;
    if (sym && sym->section_index != -1) {
        wordOffset = (static_cast<int32_t>(sym->value) - static_cast<int32_t>(instrOff)) / 4;
    } else {
        symbolFixups_.push_back({symbol, instrIdx, instrOff});
    }

    emit(op, OperandsJ{29, wordOffset}); 
}

void HVMCodeGenerator::emitBranch(Opcode op, uint8_t rs1, uint8_t rs2, Label* target) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;
    if (target->targetByteOffset != -1) {
        int32_t wordOffset = (target->targetByteOffset - static_cast<int32_t>(instrOff)) / 4;
        emit(op, OperandsB{rs1, rs2, static_cast<int16_t>(wordOffset)});
    } else {
        target->fixups.push_back({instrIdx, instrOff});
        emit(op, OperandsB{rs1, rs2, 0});
    }
}

void HVMCodeGenerator::emitNullCheck(uint8_t valueReg) {
    Label* ok = createLabel();
    emitBranch(Opcode::BNE, valueReg, 0, ok);
    moduleUsesNullChecks_ = true;
    emitCall(Opcode::CALL, "_F_hoo_exception_null_pointer_p");
    emit(Opcode::MOV, OperandsR{2, 1, 0, 0});
    emit(Opcode::SYSCALL, OperandsI{0, 0, 9});
    bindLabel(ok);
}


uint32_t HVMCodeGenerator::typeIdFromDeclaredType(const ast::Type* type, std::string* outClassName) const {
    if (dynamic_cast<const ast::AnyType*>(type)) return 0;
    if (dynamic_cast<const ast::ListType*>(type)) {
        if (outClassName) *outClassName = "List";
        return 118;
    }
    if (dynamic_cast<const ast::ByteSliceType*>(type)) return 130;
    if (dynamic_cast<const ast::DictType*>(type)) {
        if (outClassName) *outClassName = "Dict";
        return 117;
    }
    if (dynamic_cast<const ast::DecimalType*>(type)) {
    if (outClassName) *outClassName = "Decimal";
    return 125;
}
    if (auto bt = dynamic_cast<const ast::BaseType*>(type)) {
        if (bt->isPrimitive()) {
            switch (bt->getPrimitiveType()->getKind()) {
                case ast::PrimitiveTypeKind::INT64: return 1;
                case ast::PrimitiveTypeKind::FLOAT:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:   return 2;
                case ast::PrimitiveTypeKind::BIT:    return 8;
                case ast::PrimitiveTypeKind::F8:     return 9;
                case ast::PrimitiveTypeKind::BOOL:    return 3;
                case ast::PrimitiveTypeKind::VOID:    return 4;
                case ast::PrimitiveTypeKind::INT8:    return 5;
                case ast::PrimitiveTypeKind::BYTE:    return 6;
                case ast::PrimitiveTypeKind::CHAR:    return 7;
                case ast::PrimitiveTypeKind::STRING:  return 101;
                case ast::PrimitiveTypeKind::BUFFER:  return 113;
                default: return 1;
            }
        } else {
            std::string name = bt->getIdentifier();
            if (outClassName) *outClassName = name;
            uint32_t tid = builtinConstructedTypeId(name);
            if (tid != 100) return tid;
            // Case-insensitive fallback for buffer
            if (name == "buffer") return 113;
            return 100;
        }
    }
    if (dynamic_cast<const ast::ArrayType*>(type)) return 102;
    if (dynamic_cast<const ast::FutureType*>(type)) {
        if (outClassName) *outClassName = "Future";
        return 123;
    }
    if (dynamic_cast<const ast::MapType*>(type)) return 103;
    if (dynamic_cast<const ast::TensorType*>(type)) return 104;
    if (auto opt = dynamic_cast<const ast::OptionalType*>(type)) {
        // Preserve the underlying type of T? instead of collapsing to generic
        // object. A nullable array (int64[]?) stays an array; a nullable
        // scalar/object keeps its base type ID and class name. Nullability is
        // tracked separately by the codegen (Local/ExpressionTypeInfo).
        const ast::ArrayType& inner = opt->getArrayType();
        if (inner.getDimensionCount() > 0) {
            if (outClassName) *outClassName = "";
            return 102;
        }
        return typeIdFromDeclaredType(&inner.getBaseType(), outClassName);
    }
    return 100;
}

std::string HVMCodeGenerator::typeIdToMangleType(uint32_t typeId) const {
    switch (typeId) {
        case 1: return "int64";
        case 2: return "double";
        case 3: return "bool";
        case 4: return "void";
        case 5: return "int8";
        case 6: return "byte";
        case 7: return "char";
        case 8: return "bit";
        case 9: return "f8";
        case 130: return "ptr";
        case 0: return "any";
        case 101: return "string";
        case 104: return "tensor";
        /* Future values use the stable pointer ABI in function symbols. */
        case 123: return "ptr";
        default: return "ptr";
    }
}

std::string HVMCodeGenerator::mangleTypeId(uint32_t typeId, bool isNullable) const {
    std::string base = typeIdToMangleType(typeId);
    if (isNullable) base += "?";
    return base;
}

uint32_t HVMCodeGenerator::tensorElementTypeIdFromType(const ast::TensorType& type) const {
    return typeIdFromDeclaredType(&type.getElementType());
}

uint32_t HVMCodeGenerator::tensorElementTypeIdFromLiteral(const ast::TensorLiteral& literal) {
    uint32_t commonType = 100;
    if (!literal.getElements()) return commonType;
    std::function<uint32_t(const ast::Expression&)> leafType = [&](const ast::Expression& expression) -> uint32_t {
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&expression)) {
            if (auto nested = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
                uint32_t nestedCommon = 100;
                if (!nested->getElements()) return nestedCommon;
                for (const auto& child : nested->getElements()->getExpressions()) {
                    uint32_t childType = leafType(*child);
                    if (childType == 100) continue;
                    if (nestedCommon == 100) nestedCommon = childType;
                    else if (nestedCommon != childType) return 100;
                }
                return nestedCommon;
            }
        }
        return getTypeId(nullptr, &expression);
    };
    for (const auto& elem : literal.getElements()->getExpressions()) {
        uint32_t type = leafType(*elem);

        if (type != 100) {
            if (commonType == 100) commonType = type;
            else if (commonType != type) {
                if ((commonType == 2 || commonType == 9) && (type == 2 || type == 9)) commonType = 2;
                else return 100;
            }
        }
    }
    return commonType == 100 ? 1 : commonType;
}

std::vector<int64_t> HVMCodeGenerator::tensorShapeFromLiteral(const ast::TensorLiteral& literal) {
    std::vector<int64_t> shape;
    auto* elements = literal.getElements();
    int64_t firstDim = elements ? static_cast<int64_t>(elements->getExpressions().size()) : 0;
    shape.push_back(firstDim);
    if (firstDim == 0) return shape;

    const ast::Expression* first = elements->getExpressions()[0].get();
    const ast::ArrayLiteral* currentArray = nullptr;
    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(first)) {
        currentArray = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary());
    }
    while (currentArray) {
        auto* nestedElements = currentArray->getElements();
        int64_t dim = nestedElements ? static_cast<int64_t>(nestedElements->getExpressions().size()) : 0;
        shape.push_back(dim);
        if (dim == 0) break;
        currentArray = nullptr;
        const ast::Expression* next = nestedElements->getExpressions()[0].get();
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(next)) {
            currentArray = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary());
        }
    }
    return shape;
}

void HVMCodeGenerator::emitFlattenTensorLiteralElements(const ast::Expression& expr, uint8_t tensorReg) {
    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
            for (const auto& elem : arr->getElements()->getExpressions()) {
                emitFlattenTensorLiteralElements(*elem, tensorReg);
            }
            return;
        }
    }

    uint8_t valueReg = visitExpression(expr);
    emit(Opcode::MOV, OperandsR{1, tensorReg, 0, 0});
    emit(Opcode::MOV, OperandsR{2, valueReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_pushValue_i8_p_i8");
    freeRegister(valueReg);
}

uint8_t HVMCodeGenerator::emitTensorNewEx(uint32_t elemTypeId, size_t rank,
                                          const std::function<uint8_t(size_t)>& emitDim) {
    emitCall(Opcode::CALL, "_F_hoo_Array_new_p");
    uint8_t arrReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});
    for (size_t i = 0; i < rank; ++i) {
        uint8_t dimReg = emitDim(i);
        emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, dimReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_Array_pushInt64_p_i8");
        freeRegister(dimReg);
    }

    uint8_t elemReg = emitConstant(static_cast<int64_t>(elemTypeId));
    uint8_t rankReg = emitConstant(static_cast<int64_t>(rank));
    emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
    emit(Opcode::MOV, OperandsR{2, rankReg, 0, 0});
    emit(Opcode::MOV, OperandsR{3, arrReg, 0, 0});
    freeRegister(elemReg);
    freeRegister(rankReg);
    emitCall(Opcode::CALL, "_F_hoo_Tensor_new_ex_p_i8_i8_p");

    uint8_t result = allocateRegister();
    emit(Opcode::MOV, OperandsR{result, 1, 0, 0});

    emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
    freeRegister(arrReg);
    return result;
}

uint8_t HVMCodeGenerator::emitTensorNewCall(uint32_t elemTypeId, size_t rank,
                                            const std::function<uint8_t(size_t)>& emitDim) {
    if (rank == 1 || rank == 2 || rank == 3) {
        uint8_t elemReg = emitConstant(static_cast<int64_t>(elemTypeId));
        emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
        freeRegister(elemReg);
        for (size_t i = 0; i < rank; ++i) {
            uint8_t dimReg = emitDim(i);
            emit(Opcode::MOV, OperandsR{argReg(2, i), dimReg, 0, 0});
            freeRegister(dimReg);
        }
        if (rank == 1) emitCall(Opcode::CALL, "_F_hoo_Tensor_new1_p_i8_i8");
        else if (rank == 2) emitCall(Opcode::CALL, "_F_hoo_Tensor_new2_p_i8_i8_i8");
        else emitCall(Opcode::CALL, "_F_hoo_Tensor_new3_p_i8_i8_i8_i8");
        uint8_t result = allocateRegister();
        emit(Opcode::MOV, OperandsR{result, 1, 0, 0});
        return result;
    }
    return emitTensorNewEx(elemTypeId, rank, emitDim);
}

uint8_t HVMCodeGenerator::emitTensorLiteral(const ast::TensorLiteral& literal) {
    uint32_t elemType = tensorElementTypeIdFromLiteral(literal);
    std::vector<int64_t> shape = tensorShapeFromLiteral(literal);
    if (shape.empty()) shape.push_back(0);

    uint8_t tensorReg = emitTensorNewCall(elemType, shape.size(),
        [this, &shape](size_t i) -> uint8_t { return emitConstant(shape[i]); });

    if (literal.getElements()) {
        for (const auto& elem : literal.getElements()->getExpressions()) {
            emitFlattenTensorLiteralElements(*elem, tensorReg);
        }
    }
    return tensorReg;
}

uint8_t HVMCodeGenerator::emitTensorBinaryCall(const ast::BinaryExpression& binary, const std::string& symbolName) {
    uint8_t left = visitExpression(binary.getLeft());
    uint8_t right = visitExpression(binary.getRight());
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    emit(Opcode::MOV, OperandsR{2, right, 0, 0});
    emitCall(Opcode::CALL, symbolName);
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    freeRegister(left);
    freeRegister(right);
    return dest;
}

uint8_t HVMCodeGenerator::emitTensorScalarCall(const ast::BinaryExpression& binary, const std::string& symbolName) {
    const uint32_t leftType = inferExpressionTypeId(binary.getLeft());
    const bool leftIsTensor = leftType == 104;
    const ast::Expression& tensorExpr = leftIsTensor ? binary.getLeft() : binary.getRight();
    const ast::Expression& scalarExpr = leftIsTensor ? binary.getRight() : binary.getLeft();
    const uint32_t scalarType = inferExpressionTypeId(scalarExpr);
    uint8_t tensor = visitExpression(tensorExpr);
    uint8_t scalar = visitExpression(scalarExpr);
    emit(Opcode::MOV, OperandsR{1, tensor, 0, 0});
    emit(Opcode::MOV, OperandsR{2, scalar, 0, 0});
    uint8_t scalarTypeReg = emitConstant(static_cast<int64_t>(scalarType));
    emit(Opcode::MOV, OperandsR{3, scalarTypeReg, 0, 0});
    emitCall(Opcode::CALL, symbolName);
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    freeRegister(scalarTypeReg);
    freeRegister(tensor);
    freeRegister(scalar);
    return dest;
}

uint8_t HVMCodeGenerator::emitTensorVectorArith(const ast::BinaryExpression& binary, hvm::Opcode vecOp, uint16_t func) {
    // Tensor handles are opaque ABI objects; the old vector lowering assumed
    // an inline payload at a fixed offset and is not valid for dynamic-rank
    // tensors. Keep the HVM instruction available for native vector code, but
    // route tensor expressions through the checked runtime kernels.
    (void)vecOp;
    switch (func) {
        case 0: return emitTensorBinaryCall(binary, "_F_hoo_Tensor_add_p_p_p");
        case 2: return emitTensorBinaryCall(binary, "_F_hoo_Tensor_sub_p_p_p");
        case 4: return emitTensorBinaryCall(binary, "_F_hoo_Tensor_elementMul_p_p_p");
        case 6: return emitTensorBinaryCall(binary, "_F_hoo_Tensor_elementDiv_p_p_p");
        default: return emitTensorBinaryCall(binary, "_F_hoo_Tensor_add_p_p_p");
    }

#if 0
    uint8_t left = visitExpression(binary.getLeft());
    uint8_t right = visitExpression(binary.getRight());
    // elemTypeReg = _F_hoo_Tensor_elementType_i8_p(left)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_elementType_i8_p");
    uint8_t elemTypeReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{elemTypeReg, 1, 0, 0});

    // rankReg = _F_hoo_Tensor_rank_i8_p(left)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_rank_i8_p");
    uint8_t rankReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{rankReg, 1, 0, 0});

    // d0Reg = _F_hoo_Tensor_dim_i8_p_i8(left, 0)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    uint8_t zeroReg = emitConstant(0);
    emit(Opcode::MOV, OperandsR{2, zeroReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_dim_i8_p_i8");
    uint8_t d0Reg = allocateRegister();
    emit(Opcode::MOV, OperandsR{d0Reg, 1, 0, 0});
    freeRegister(zeroReg);

    // d1Reg = _F_hoo_Tensor_dim_i8_p_i8(left, 1)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    uint8_t oneReg = emitConstant(1);
    emit(Opcode::MOV, OperandsR{2, oneReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_dim_i8_p_i8");
    uint8_t d1Reg = allocateRegister();
    emit(Opcode::MOV, OperandsR{d1Reg, 1, 0, 0});
    freeRegister(oneReg);

    // d2Reg = _F_hoo_Tensor_dim_i8_p_i8(left, 2)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    uint8_t twoReg = emitConstant(2);
    emit(Opcode::MOV, OperandsR{2, twoReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_dim_i8_p_i8");
    uint8_t d2Reg = allocateRegister();
    emit(Opcode::MOV, OperandsR{d2Reg, 1, 0, 0});
    freeRegister(twoReg);
    
    emit(Opcode::MOV, OperandsR{argReg(1, 0), elemTypeReg, 0, 0});
    emit(Opcode::MOV, OperandsR{argReg(1, 1), rankReg, 0, 0});
    emit(Opcode::MOV, OperandsR{argReg(1, 2), d0Reg, 0, 0});
    emit(Opcode::MOV, OperandsR{argReg(1, 3), d1Reg, 0, 0});
    emit(Opcode::MOV, OperandsR{argReg(1, 4), d2Reg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_new_p_i8_i8_i8_i8_i8");
    
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    
    freeRegister(rankReg);
    freeRegister(d0Reg);
    freeRegister(d1Reg);
    freeRegister(d2Reg);
    
    // lenReg = _F_hoo_Tensor_length_i8_p(left)
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Tensor_length_i8_p");
    uint8_t lenReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{lenReg, 1, 0, 0});
    
    uint8_t baseLeft = allocateRegister();
    emit(Opcode::ADDI, OperandsI{baseLeft, left, 64});
    uint8_t baseRight = allocateRegister();
    emit(Opcode::ADDI, OperandsI{baseRight, right, 64});
    uint8_t baseDest = allocateRegister();
    emit(Opcode::ADDI, OperandsI{baseDest, dest, 64});
    
    Label* loopStart = createLabel();
    bindLabel(loopStart);
    
    uint8_t vlReg = allocateRegister();
    emit(Opcode::VSETVL, OperandsR{vlReg, lenReg, elemTypeReg, 0});
    freeRegister(elemTypeReg);
    
    emit(Opcode::VECTOR_MEM, OperandsR{0, baseLeft, 0, 0}); 
    emit(Opcode::VECTOR_MEM, OperandsR{1, baseRight, 0, 0}); 
    
    emit(vecOp, OperandsR{2, 0, 1, func}); 
    
    emit(Opcode::VECTOR_MEM, OperandsR{2, baseDest, 0, 1}); 
    
    uint8_t shiftReg = allocateRegister();
    uint8_t threeReg = emitConstant(3);
    emit(Opcode::SHIFT, OperandsR{shiftReg, vlReg, threeReg, 0}); // shl
    freeRegister(threeReg);
    
    uint8_t tempLeft = allocateRegister();
    emit(Opcode::ARITH, OperandsR{tempLeft, baseLeft, shiftReg, 0}); 
    emit(Opcode::MOV, OperandsR{baseLeft, tempLeft, 0, 0});
    freeRegister(tempLeft);
    
    uint8_t tempRight = allocateRegister();
    emit(Opcode::ARITH, OperandsR{tempRight, baseRight, shiftReg, 0}); 
    emit(Opcode::MOV, OperandsR{baseRight, tempRight, 0, 0});
    freeRegister(tempRight);
    
    uint8_t tempDest = allocateRegister();
    emit(Opcode::ARITH, OperandsR{tempDest, baseDest, shiftReg, 0}); 
    emit(Opcode::MOV, OperandsR{baseDest, tempDest, 0, 0});
    freeRegister(tempDest);
    
    uint8_t nextLenReg = allocateRegister();
    emit(Opcode::ARITH, OperandsR{nextLenReg, lenReg, vlReg, 1}); 
    emit(Opcode::MOV, OperandsR{lenReg, nextLenReg, 0, 0});
    freeRegister(nextLenReg);
    
    freeRegister(shiftReg);
    freeRegister(vlReg);
    
    uint8_t condReg = allocateRegister();
    uint8_t zeroReg2 = emitConstant(0);
    emit(Opcode::CMP, OperandsR{condReg, lenReg, zeroReg2, 1}); // cmpne
    freeRegister(zeroReg2);
    emitBranch(Opcode::BNE, condReg, 0, loopStart); 
    freeRegister(condReg);
    
    freeRegister(lenReg);
    freeRegister(baseLeft);
    freeRegister(baseRight);
    freeRegister(baseDest);
    freeRegister(left);
    freeRegister(right);
    
    return dest;
#endif
}
uint8_t HVMCodeGenerator::emitDecimalBinaryOp(const ast::BinaryExpression& binary) {
    uint8_t lhs = visitExpression(binary.getLeft());
    uint8_t rhs = visitExpression(binary.getRight());

    const char* symbol = nullptr;

    switch (binary.getOperator()) {
        // Arithmetic
        case ast::BinaryOperator::PLUS:       symbol = "_F_hoo_Decimal_add_p_p_p"; break;
        case ast::BinaryOperator::MINUS:      symbol = "_F_hoo_Decimal_sub_p_p_p"; break;
        case ast::BinaryOperator::MULTIPLY:   symbol = "_F_hoo_Decimal_mul_p_p_p"; break;
        case ast::BinaryOperator::DIVIDE:     symbol = "_F_hoo_Decimal_div_p_p_p"; break;
        case ast::BinaryOperator::MODULO:     symbol = "_F_hoo_Decimal_mod_p_p_p"; break;
        // Comparison (return int64 1/0, will be boxed as bool by caller)
        case ast::BinaryOperator::EQUALS:             symbol = "_F_hoo_Decimal_eq_p_p_p"; break;
        case ast::BinaryOperator::NOT_EQUALS:        symbol = "_F_hoo_Decimal_ne_p_p_p"; break;
        case ast::BinaryOperator::LESS:              symbol = "_F_hoo_Decimal_lt_p_p_p"; break;
        case ast::BinaryOperator::LESS_EQUALS:       symbol = "_F_hoo_Decimal_le_p_p_p"; break;
        case ast::BinaryOperator::GREATER:           symbol = "_F_hoo_Decimal_gt_p_p_p"; break;
        case ast::BinaryOperator::GREATER_EQUALS:    symbol = "_F_hoo_Decimal_ge_p_p_p"; break;
        default:
            addError("Unsupported decimal operator");
            freeRegister(lhs);
            freeRegister(rhs);
            return 0;
    }

    emit(Opcode::MOV, OperandsR{1, lhs, 0, 0});
    emit(Opcode::MOV, OperandsR{2, rhs, 0, 0});

    emitCall(Opcode::CALL, symbol);

    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});

    freeRegister(lhs);
    freeRegister(rhs);

    return dest;
}
HVMCodeGenerator::ExpressionTypeInfo HVMCodeGenerator::inferExpressionTypeInfo(const ast::Expression& expr) {
    ExpressionTypeInfo result;

    if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        const ast::ASTNode& primary = primaryExpr->getPrimary();
        if (auto id = dynamic_cast<const ast::Identifier*>(&primary)) {
            for (auto scope = scopeStack_.rbegin(); scope != scopeStack_.rend(); ++scope) {
                auto it = scope->find(id->getName());
                if (it != scope->end()) {
                    result.typeId = it->second.typeId;
                    result.className = it->second.className;
                    result.elementTypeId = it->second.elementTypeId;
                    result.keyTypeId = it->second.keyTypeId;
                    result.isNullable = it->second.isNullable;
                    return result;
                }
            }
            if (isBuiltinClassName(id->getName())) {
                result.className = id->getName();
                result.typeId = builtinConstructedTypeId(id->getName());
            }
            return result;
        }
        if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primary)) {
            return inferExpressionTypeInfo(paren->getExpression());
        }
        if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&primary)) {
            result.typeId = arr->isList() ? 118 : 102;
            if (arr->getElements()) {
                uint32_t common = 0;
                for (const auto& element : arr->getElements()->getExpressions()) {
                    const auto elementInfo = inferExpressionTypeInfo(*element);
                    if (elementInfo.typeId == 100) { common = 100; break; }
                    if (common == 0) common = elementInfo.typeId;
                    else if (common != elementInfo.typeId) { common = 100; break; }
                }
                if (arr->isList()) {
                    if (common != 0 && common != 100) result.elementTypeId = common;
                } else {
                    result.elementTypeId = common == 0 ? 100 : common;
                }
            }
            return result;
        }
        if (dynamic_cast<const ast::NullLiteral*>(&primary)) {
            result.typeId = 0;
            result.isNullable = true;
        }
        else if (dynamic_cast<const ast::IntegerLiteral*>(&primary)) result.typeId = 1;
        else if (dynamic_cast<const ast::FloatingLiteral*>(&primary)) result.typeId = 2;
        else if (dynamic_cast<const ast::BooleanLiteral*>(&primary)) result.typeId = 3;
        else if (dynamic_cast<const ast::BitLiteral*>(&primary)) result.typeId = 8;
        else if (dynamic_cast<const ast::F8Literal*>(&primary)) result.typeId = 9;
        else if (dynamic_cast<const ast::StringLiteral*>(&primary) ||
                 dynamic_cast<const ast::InterpolatedString*>(&primary)) result.typeId = 101;
        else if (dynamic_cast<const ast::CharacterLiteral*>(&primary)) result.typeId = 109;
        else if (dynamic_cast<const ast::DecimalLiteral*>(&primary)) result.typeId = 125;
        else if (auto tensorLiteral = dynamic_cast<const ast::TensorLiteral*>(&primary)) {
            result.typeId = 104;
            result.elementTypeId = tensorElementTypeIdFromLiteral(*tensorLiteral);
        }
        else if (auto nested = dynamic_cast<const ast::Expression*>(&primary)) {
            return inferExpressionTypeInfo(*nested);
        }
        return result;
    }

    if (auto awaitExpr = dynamic_cast<const ast::AwaitExpression*>(&expr)) {
        const auto future = inferExpressionTypeInfo(awaitExpr->getFuture());
        if (future.typeId == 123) {
            result.typeId = future.elementTypeId != 0 ? future.elementTypeId : 100;
            result.className = builtinClassNameFromTypeId(result.typeId);
        }
        result.isNullable = future.isNullable;
        return result;
    }

    if (auto arrayAccess = dynamic_cast<const ast::ArrayAccess*>(&expr)) {
        const auto array = inferExpressionTypeInfo(arrayAccess->getArray());
        if (array.elementTypeId != 0) result.typeId = array.elementTypeId;
        else if (array.typeId == 104) result.typeId = 1;
        result.isNullable = array.isNullable;
        return result;
    }

    if (auto newExpr = dynamic_cast<const ast::NewObjectExpression*>(&expr)) {
        result.typeId = builtinConstructedTypeId(newExpr->getClassName());
        result.className = newExpr->getClassName();
        if (newExpr->getClassName() == "Map") {
            result.keyTypeId = mapConstructorKeyTypeId(*newExpr);
            result.elementTypeId = mapConstructorValueTypeId(*newExpr);
        }
        return result;
    }
    if (auto newHash = dynamic_cast<const ast::NewDictExpression*>(&expr)) {
        result.typeId = 117;
        result.className = "Dict";
        result.keyTypeId = dictKeyTypeId(newHash->getDictType());
        result.elementTypeId = typeIdFromDeclaredType(&newHash->getDictType().getValueType());
        return result;
    }

    if (auto unaryMinus = dynamic_cast<const ast::UnaryMinus*>(&expr)) {
        return inferExpressionTypeInfo(unaryMinus->getOperand());
    }

    if (auto logicalNot = dynamic_cast<const ast::LogicalNot*>(&expr)) {
        const auto operand = inferExpressionTypeInfo(logicalNot->getOperand());
        result.typeId = operand.typeId == 104 ? 104 : 3;
        result.elementTypeId = operand.elementTypeId;
        return result;
    }

    if (auto logicalAnd = dynamic_cast<const ast::LogicalAnd*>(&expr)) {
        const auto left = inferExpressionTypeInfo(logicalAnd->getLeft());
        const auto right = inferExpressionTypeInfo(logicalAnd->getRight());
        result.typeId = (left.typeId == 104 || right.typeId == 104) ? 104 : 3;
        return result;
    }

    if (auto logicalOr = dynamic_cast<const ast::LogicalOr*>(&expr)) {
        const auto left = inferExpressionTypeInfo(logicalOr->getLeft());
        const auto right = inferExpressionTypeInfo(logicalOr->getRight());
        result.typeId = (left.typeId == 104 || right.typeId == 104) ? 104 : 3;
        return result;
    }

    if (auto binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
        const auto left = inferExpressionTypeInfo(binary->getLeft());
        const auto right = inferExpressionTypeInfo(binary->getRight());
        // Tensor comparisons and logical operations return tensor masks, not
        // scalar booleans. Preserve that shape so the code generator selects
        // the tensor runtime path for chained indexing and composition.
        if (left.typeId == 104 || right.typeId == 104) {
            result.typeId = 104;
            result.elementTypeId = 8;
            return result;
        }
        switch (binary->getOperator()) {
            case ast::BinaryOperator::EQUALS:
            case ast::BinaryOperator::NOT_EQUALS:
            case ast::BinaryOperator::LESS:
            case ast::BinaryOperator::LESS_EQUALS:
            case ast::BinaryOperator::GREATER:
            case ast::BinaryOperator::GREATER_EQUALS:
                result.typeId = 3;
                return result;
            case ast::BinaryOperator::PLUS:
                if (left.typeId == 101 || right.typeId == 101) {
                    result.typeId = 101;
                    result.className = "String";
                    return result;
                }
                break;
            default:
                break;
        }
        if (left.typeId == 125 || right.typeId == 125) {
            result.typeId = 125;
        } else if (left.typeId == 9 || right.typeId == 9) {
            result.typeId = 9;
        } else if (left.typeId == 2 || right.typeId == 2) {
            result.typeId = 2;
        } else if (left.typeId == right.typeId) {
            result = left;
        } else if (left.typeId != 100) {
            result = left;
        } else {
            result = right;
        }
        return result;
    }

    if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(&expr)) {
        const auto receiver = inferExpressionTypeInfo(memberAccess->getObject());
        std::string className = receiver.className;
        if (className.empty()) className = builtinClassNameFromTypeId(receiver.typeId);
        if (!className.empty()) {
            auto classIt = classes_.find(className);
            if (classIt != classes_.end()) {
                const auto fieldIt = classIt->second.fieldTypeIds.find(memberAccess->getMember());
                if (fieldIt != classIt->second.fieldTypeIds.end()) {
                    result.typeId = fieldIt->second;
                    auto fieldClassIt = classIt->second.fieldClassNames.find(memberAccess->getMember());
                    if (fieldClassIt != classIt->second.fieldClassNames.end()) result.className = fieldClassIt->second;
                    auto elemIt = classIt->second.fieldElementTypeIds.find(memberAccess->getMember());
                    if (elemIt != classIt->second.fieldElementTypeIds.end()) result.elementTypeId = elemIt->second;
                    auto nullableIt = classIt->second.fieldIsNullable.find(memberAccess->getMember());
                    if (nullableIt != classIt->second.fieldIsNullable.end()) result.isNullable = nullableIt->second;
                    else result.isNullable = receiver.isNullable;
                    return result;
                }
            }
        }
        // Preserve the established built-in method tables for simple receivers.
        result.typeId = inferExpressionTypeIdLegacy(expr);
        return result;
    }

    if (auto funcCall = dynamic_cast<const ast::FunctionCall*>(&expr)) {
        std::vector<uint32_t> argumentTypeIds;
        std::vector<bool> argumentNullable;
        if (funcCall->getArguments()) {
            for (const auto& arg : funcCall->getArguments()->getArguments()) {
                const auto argInfo = inferExpressionTypeInfo(*arg);
                argumentTypeIds.push_back(argInfo.typeId);
                argumentNullable.push_back(argInfo.isNullable);
            }
        }
        auto selectOverload = [&](const std::vector<OverloadReturnInfo>& overloads) -> const OverloadReturnInfo* {
            const OverloadReturnInfo* best = nullptr;
            int bestScore = INT_MAX;
            for (const auto& overload : overloads) {
                if (overload.parameterTypes.size() != argumentTypeIds.size()) continue;
                int score = 0;
                bool viable = true;
                for (size_t i = 0; i < argumentTypeIds.size(); ++i) {
                    const bool argNullable = argumentNullable[i];
                    const bool paramNullable = i < overload.parameterIsNullable.size() ? overload.parameterIsNullable[i] : false;
                    const uint32_t actual = argumentTypeIds[i];
                    const uint32_t expected = overload.parameterTypes[i];
                    if (argNullable && !paramNullable) { viable = false; break; }
                    if (actual == expected) continue;
                    if (expected == 1 && (actual == 5 || actual == 6)) score += 1;
                    else if (expected == 2 && actual == 9) score += 1;
                    else if (expected == 2 && (actual == 1 || actual == 5 || actual == 6)) score += 2;
                    else if (expected == 3 && actual == 8) score += 1;
                    else if (expected == 100) score += 3;
                    else if (expected == 0) score += 20;
                    else { viable = false; break; }
                }
                if (viable && score < bestScore) {
                    best = &overload;
                    bestScore = score;
                }
            }
            return best;
        };
        const ast::Identifier* functionId = dynamic_cast<const ast::Identifier*>(&funcCall->getFunction());
        if (!functionId) {
            if (auto functionPrimary = dynamic_cast<const ast::PrimaryExpression*>(&funcCall->getFunction())) {
                functionId = dynamic_cast<const ast::Identifier*>(&functionPrimary->getPrimary());
            }
        }
        if (auto id = functionId) {
            if (isHooModuleFreeFunction(id->getName())) {
                std::vector<uint32_t> freeFunctionArgs;
                if (funcCall->getArguments()) {
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                        freeFunctionArgs.push_back(inferExpressionTypeInfo(*arg).typeId);
                    }
                }
                result.typeId = hooModuleFreeFunctionReturnTypeId(id->getName(), freeFunctionArgs);
                result.className = builtinClassNameFromTypeId(result.typeId);
                return result;
            }
            auto overloadIt = functionOverloadReturns_.find(id->getName());
            if (overloadIt != functionOverloadReturns_.end()) {
                if (const auto* selected = selectOverload(overloadIt->second)) return selected->result;
            }
            auto it = functionReturnTypes_.find(id->getName());
            if (it != functionReturnTypes_.end()) {
                result.typeId = it->second;
                auto classIt = functionReturnClass_.find(id->getName());
                if (classIt != functionReturnClass_.end()) result.className = classIt->second;
                auto externalMeta = externalFunctionMetadata_.find(id->getName());
                if (externalMeta != externalFunctionMetadata_.end() && result.className.empty()) {
                    result.className = externalMeta->second.returnClass;
                }
                if (result.className.empty()) result.className = builtinClassNameFromTypeId(result.typeId);
                if (result.typeId == 123) {
                    auto futureIt = functionFutureElementTypes_.find(id->getName());
                    if (futureIt != functionFutureElementTypes_.end()) result.elementTypeId = futureIt->second;
                }
                return result;
            }
        }
        if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(&funcCall->getFunction())) {
            const auto receiver = inferExpressionTypeInfo(memberAccess->getObject());
            std::string className = receiver.className;
            if (className.empty()) className = builtinClassNameFromTypeId(receiver.typeId);
            const ast::Identifier* objectId = dynamic_cast<const ast::Identifier*>(&memberAccess->getObject());
            if (!objectId) {
                if (auto objectPrimary = dynamic_cast<const ast::PrimaryExpression*>(&memberAccess->getObject())) {
                    objectId = dynamic_cast<const ast::Identifier*>(&objectPrimary->getPrimary());
                }
            }
            if (objectId) {
                if (isBuiltinClassName(objectId->getName()) && getLocalTypeId(objectId->getName()) == 0) {
                    className = objectId->getName();
                }
                // Serializable deserialization is a generated static method;
                // expose its class result to later instance dispatch and
                // chained expression inference.
                if (className.empty() && classes_.count(objectId->getName()) &&
                    classes_.at(objectId->getName()).isSerializable) {
                    className = objectId->getName();
                }
            }

            if (className.size() > 0 && classes_.count(className) &&
                classes_.at(className).isSerializable) {
                if (memberAccess->getMember() == "deserialize") {
                    result.typeId = 100;
                    result.className = className;
                    return result;
                }
                if (memberAccess->getMember() == "serialize") {
                    result.typeId = 101;
                    return result;
                }
            }
            auto classIt = classes_.find(className);
            if (classIt != classes_.end()) {
                auto overloadIt = classIt->second.methodOverloadReturns.find(memberAccess->getMember());
                if (overloadIt != classIt->second.methodOverloadReturns.end()) {
                    if (const auto* selected = selectOverload(overloadIt->second)) return selected->result;
                }
                auto retIt = classIt->second.methodReturnTypes.find(memberAccess->getMember());
                if (retIt != classIt->second.methodReturnTypes.end()) {
                    result.typeId = retIt->second;
                    result.className = classIt->second.methodReturnClasses[memberAccess->getMember()];
                    return result;
                }
            }
            // For chained built-ins, retain the common return shapes needed
            // for a subsequent receiver lookup.
            const auto& method = memberAccess->getMember();
            if (className == "Array" && (method == "sort" || method == "reverse" || method == "shuffle")) {
                result.typeId = 102; result.className = "Array"; return result;
            }
            if (className == "Map" && (method == "getInt64String" || method == "getStringString")) {
                result.typeId = 101; result.className = "String"; return result;
            }
            if (className == "Buffer" && (method == "copy" || method == "slice")) {
                result.typeId = 113; result.className = "Buffer"; return result;
            }
            result.typeId = inferExpressionTypeIdLegacy(expr);
            result.className = builtinClassNameFromTypeId(result.typeId);
            return result;
        }
    }

    result.typeId = inferExpressionTypeIdLegacy(expr);
    return result;
}

uint32_t HVMCodeGenerator::inferExpressionTypeId(const ast::Expression& expr) {
    return inferExpressionTypeInfo(expr).typeId;
}

uint32_t HVMCodeGenerator::inferExpressionTypeIdLegacy(const ast::Expression& expr) {
    if (auto awaitExpr = dynamic_cast<const ast::AwaitExpression*>(&expr)) {
        const ast::Expression* source = &awaitExpr->getFuture();
        while (auto primary = dynamic_cast<const ast::PrimaryExpression*>(source)) {
            const auto& node = primary->getPrimary();
            if (auto id = dynamic_cast<const ast::Identifier*>(&node)) {
                for (auto scope = scopeStack_.rbegin(); scope != scopeStack_.rend(); ++scope) {
                    auto local = scope->find(id->getName());
                    if (local != scope->end() && local->second.typeId == 123) {
                        return local->second.elementTypeId != 0 ? local->second.elementTypeId : 100;
                    }
                }
                return 100;
            }
            if (auto nested = dynamic_cast<const ast::Expression*>(&node)) {
                source = nested;
            } else {
                break;
            }
        }
        while (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(source)) {
            source = &paren->getExpression();
        }
        if (auto call = dynamic_cast<const ast::FunctionCall*>(source)) {
            if (auto functionPrimary = dynamic_cast<const ast::PrimaryExpression*>(&call->getFunction())) {
                if (auto id = dynamic_cast<const ast::Identifier*>(&functionPrimary->getPrimary())) {
                    auto it = functionFutureElementTypes_.find(id->getName());
                    if (it != functionFutureElementTypes_.end()) return it->second;
                }
            }
        }
        if (auto id = dynamic_cast<const ast::Identifier*>(source)) {
            for (auto scope = scopeStack_.rbegin(); scope != scopeStack_.rend(); ++scope) {
                auto local = scope->find(id->getName());
                if (local != scope->end() && local->second.typeId == 123) {
                    return local->second.elementTypeId != 0 ? local->second.elementTypeId : 100;
                }
            }
        }
        return 100;
    }
    if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        const ast::ASTNode& primary = primaryExpr->getPrimary();
        if (dynamic_cast<const ast::IntegerLiteral*>(&primary)) return 1;
        if (dynamic_cast<const ast::FloatingLiteral*>(&primary)) return 2;
        if (dynamic_cast<const ast::BooleanLiteral*>(&primary)) return 3;
        if (dynamic_cast<const ast::BitLiteral*>(&primary)) return 8;
        if (dynamic_cast<const ast::F8Literal*>(&primary)) return 9;
        if (dynamic_cast<const ast::StringLiteral*>(&primary)) return 101;
        if (dynamic_cast<const ast::CharacterLiteral*>(&primary)) return 109;
        if (dynamic_cast<const ast::InterpolatedString*>(&primary)) return 101;
        if (dynamic_cast<const ast::DecimalLiteral*>(&primary)) return 125;
        if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&primary)) return arr->isList() ? 118 : 102;
        if (dynamic_cast<const ast::TensorLiteral*>(&primary)) return 104;
        if (auto id = dynamic_cast<const ast::Identifier*>(&primary)) {
            return getLocalTypeId(id->getName());
        }
        if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primary)) {
            return inferExpressionTypeId(paren->getExpression());
        }
        return 100;
    }

    if (auto unaryMinus = dynamic_cast<const ast::UnaryMinus*>(&expr)) {
        return inferExpressionTypeId(unaryMinus->getOperand());
    }
    if (auto logicalNot = dynamic_cast<const ast::LogicalNot*>(&expr)) {
        return inferExpressionTypeId(logicalNot->getOperand()) == 104 ? 104 : 3;
    }
    if (auto logicAnd = dynamic_cast<const ast::LogicalAnd*>(&expr)) {
        uint32_t left = inferExpressionTypeId(logicAnd->getLeft());
        uint32_t right = inferExpressionTypeId(logicAnd->getRight());
        if (left == 104 || right == 104) return 104;
        return left == 8 && right == 8 ? 8 : 3;
    }
    if (auto logicOr = dynamic_cast<const ast::LogicalOr*>(&expr)) {
        uint32_t left = inferExpressionTypeId(logicOr->getLeft());
        uint32_t right = inferExpressionTypeId(logicOr->getRight());
        if (left == 104 || right == 104) return 104;
        return left == 8 && right == 8 ? 8 : 3;
    }
    if (auto binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
        uint32_t left = inferExpressionTypeId(binary->getLeft());
        uint32_t right = inferExpressionTypeId(binary->getRight());
        if (left == 104 || right == 104) return 104;
        if (left == 125 || right == 125) return 125;
        switch (binary->getOperator()) {
            case ast::BinaryOperator::LESS:
            case ast::BinaryOperator::LESS_EQUALS:
            case ast::BinaryOperator::GREATER:
            case ast::BinaryOperator::GREATER_EQUALS:
            case ast::BinaryOperator::EQUALS:
            case ast::BinaryOperator::NOT_EQUALS:
                return 3;
            case ast::BinaryOperator::AND:
            case ast::BinaryOperator::OR:
                return left == 8 && right == 8 ? 8 : 3;
            default:
                if (left == 9 || right == 9) return 9;
                if (left == 2 || right == 2) return 2;
                if (left == 8 && right == 8) return 8;
                return left != 100 ? left : right;
        }
    }
    if (auto arrayAccess = dynamic_cast<const ast::ArrayAccess*>(&expr)) {
        if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&arrayAccess->getArray())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&primaryExpr->getPrimary())) {
                uint32_t containerTypeId = getLocalTypeId(id->getName());
                uint32_t elementTypeId = getLocalElementTypeId(id->getName());
                if (containerTypeId == 104) {
                    return elementTypeId != 100 && elementTypeId != 0 ? elementTypeId : 1;
                }
                if (elementTypeId != 100) return elementTypeId;
            }
        }
        return 100;
    }
    if (auto newExpr = dynamic_cast<const ast::NewObjectExpression*>(&expr)) {
        uint32_t typeId = builtinConstructedTypeId(newExpr->getClassName());
        return typeId != 100 ? typeId : 100;
    }
    if (dynamic_cast<const ast::NewDictExpression*>(&expr)) {
        return 117;
    }
    if (auto funcCall = dynamic_cast<const ast::FunctionCall*>(&expr)) {
        if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&funcCall->getFunction())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&primaryExpr->getPrimary())) {
                if (isHooModuleFreeFunction(id->getName())) {
                    std::vector<uint32_t> argTypeIds;
                    if (funcCall->getArguments()) {
                        for (const auto& arg : funcCall->getArguments()->getArguments()) {
                            argTypeIds.push_back(inferExpressionTypeId(*arg));
                        }
                    }
                    return hooModuleFreeFunctionReturnTypeId(id->getName(), argTypeIds);
                }
                auto it = functionReturnTypes_.find(id->getName());
                if (it != functionReturnTypes_.end()) return it->second;
            }
        }
        if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(&funcCall->getFunction())) {
            if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&memberAccess->getObject())) {
                if (auto id = dynamic_cast<const ast::Identifier*>(&primaryExpr->getPrimary())) {
                    const std::string& className = id->getName();
                    if (isSingletonBuiltinClass(className)) {
                        std::vector<uint32_t> argTypeIds;
                        if (funcCall->getArguments()) {
                            for (const auto& arg : funcCall->getArguments()->getArguments()) {
                                argTypeIds.push_back(inferExpressionTypeId(*arg));
                            }
                        }

                        const std::string returnType =
                            singletonMethodReturnType(className, memberAccess->getMember(), argTypeIds);
                        if (returnType == "int64") return 1;
                        if (returnType == "double") return 2;
                        if (returnType == "int8") return 5;
                        if (returnType == "byte") return 6;
                        if (returnType == "bool") return 3;
                        if (returnType == "void") return 4;
                    }

                    uint32_t objectTypeId = getLocalTypeId(className);
                    if (objectTypeId == 0) {
                        static const std::unordered_map<std::string, uint32_t> builtinTypeIds = {
                            {"Array", 102}, {"Tensor", 104}, {"String", 101},
                            {"Map", 103}, {"Buffer", 113}, {"Character", 109},
                            {"Random", 105}, {"DateTime", 119}, {"Args", 110},
                            {"Compression", 111}, {"Csv", 114}, {"Path", 114},
                            {"URL", 106}, {"HttpClient", 108}, {"HttpResponse", 107},
                            {"Http", 108}, {"Response", 107}, {"Dict", 117},
                            {"List", 118}, {"Regex", 120}, {"Mutex", 121},
                            {"Uuid", 122}
                        };
                        auto it = builtinTypeIds.find(className);
                        if (it != builtinTypeIds.end()) {
                            objectTypeId = it->second;
                        }
                    }
                    const std::string& member = memberAccess->getMember();
                    if (objectTypeId == 102) {
                        if (member == "length" || member == "empty") return 1;
                        if (member == "sort" || member == "reverse" || member == "shuffle" || member == "sortRange") return 102;
                        if (member == "binarySearch") return 1;
                        return 100;
                    }
                    if (objectTypeId == 105) {
                        if (member == "nextInt" || member == "nextIntMax" || member == "nextBytes") return 1;
                        if (member == "nextBool") return 3;
                        if (member == "nextDouble") return 2;
                        return 100;
                    }
                    if (objectTypeId == 117) {
                        if (member == "count" || member == "remove") return 1;
                        if (member == "clear") return 4;
                        return 100;
                    }
                    if (objectTypeId == 118) {
                        if (member == "length" || member == "push") return 1;
                        if (member == "clear") return 4;
                        if (member == "pop") return 0;
                        return 100;
                    }
                    if (objectTypeId == 119) {
                        if (member == "format" || member == "iso8601") return 101;
                        if (member == "addDays" || member == "addHours" || member == "addMinutes" ||
                            member == "addSeconds" || member == "addMilliseconds" ||
                            member == "now" || member == "parse" || member == "fromIso8601") return 119;
                        if (member == "getTimestamp" || member == "compare" ||
                            member == "diffDays" || member == "diffHours") return 1;
                        if (member == "diffSeconds") return 2;
                        return 119;
                    }
                    if (objectTypeId == 110) {
                        if (member == "count" || member == "has" ||
                            member == "parse" || member == "getInt" ||
                            member == "getBool") return 1;
                        if (member == "get" || member == "value" ||
                            member == "programName" || member == "getString" ||
                            member == "helpText") return 101;
                        if (member == "getFloat") return 2;
                        if (member == "addString" || member == "addInt" ||
                            member == "addFlag" || member == "addFloat" ||
                            member == "addPositional" || member == "clear") return 4;
                        if (member == "new") return 110;
                        return 100;
                    }
                }
            }
        }
    }

    if (dynamic_cast<const ast::InterpolatedString*>(&expr)) return 101;

    return 100;
}

uint32_t HVMCodeGenerator::getTypeId(const ast::Type* type, const ast::Expression* initializer, std::string* outClassName) {
    if (!type) {
        if (initializer) {
            // Basic inference from literal
            if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(initializer)) {
                const ast::ASTNode& node = pe->getPrimary();
                if (dynamic_cast<const ast::IntegerLiteral*>(&node)) return 1;
                if (dynamic_cast<const ast::FloatingLiteral*>(&node)) return 2;
                if (dynamic_cast<const ast::BitLiteral*>(&node)) return 8;
                if (dynamic_cast<const ast::F8Literal*>(&node)) return 9;
                if (dynamic_cast<const ast::BooleanLiteral*>(&node)) return 3;
                if (dynamic_cast<const ast::StringLiteral*>(&node)) return 101;
                if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&node)) return arr->isList() ? 118 : 102;
                if (dynamic_cast<const ast::TensorLiteral*>(&node)) return 104;
                if (dynamic_cast<const ast::CharacterLiteral*>(&node)) return 109;
            }
            const auto inferredInfo = inferExpressionTypeInfo(*initializer);
            if (outClassName && !inferredInfo.className.empty()) {
                *outClassName = inferredInfo.className;
            }
            if (inferredInfo.typeId != 100) return inferredInfo.typeId;
            // Back-compat inference for older class-style factory calls.
            if (auto fc = dynamic_cast<const ast::FunctionCall*>(initializer)) {
                if (auto ma = dynamic_cast<const ast::MemberAccess*>(&fc->getFunction())) {
                    std::string clsName;
                    const ast::Expression& obj = ma->getObject();
                    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&obj)) {
                        if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                            clsName = id->getName();
                        }
                    }
                    if (clsName == "Array") {
                        if (ma->getMember() == "new") return 102;
                        return 101;
                    }
                    if (clsName == "Map") {
                        if (ma->getMember() == "new") return 103;
                        return 101;
                    }
                    if (clsName == "Args") {
                        if (ma->getMember() == "new") return 110;
                        return 101;
                    }
                    if (clsName == "Compression") {
                        if (ma->getMember() == "new") return 111;
                        return 101;
                    }
                    if (clsName == "Csv") {
                        if (ma->getMember() == "new" || ma->getMember() == "newWithOpts" ||
                             ma->getMember() == "retain") return 114;
                        return 101;
                    }
                    if (clsName == "Character") {
                        if (ma->getMember() == "new") return 109;
                        return 101;
                    }
                    if (clsName == "Buffer") {
                        if (ma->getMember() == "new") return 113;
                        return 101;
                    }
                    if (clsName == "Random") {
                        if (ma->getMember() == "new") return 105;
                        return 100;
                    }
                    if (clsName == "DateTime") {
                        const std::string& member = ma->getMember();
                        if (member == "iso8601" || member == "format") return 101;
                        if (member == "nowSeconds") return 1;
                        if (member == "nowPrecise") return 2;
                        return 119;
                    }
                    if (isBuiltinClassName(clsName)) {
                        uint32_t tid = builtinConstructedTypeId(clsName);
                        if (tid != 100) return tid;
                        return 101;
                    }
                    // Inference from instance method calls (e.g. args.get(0))
                    if (!clsName.empty()) {
                        const std::string& member = ma->getMember();
                        uint32_t objTypeId = getLocalTypeId(clsName);
                        if (objTypeId == 110) {
                            if (member == "count" || member == "has" ||
                                member == "parse" || member == "getInt" ||
                                member == "getBool") return 1;
                            if (member == "get" || member == "value" ||
                                member == "programName" || member == "getString" ||
                                member == "helpText") return 101;
                            if (member == "getFloat") return 2;
                            if (member == "addString" || member == "addInt" ||
                                member == "addFlag" || member == "addFloat" ||
                                member == "addPositional" || member == "clear") return 4;
                            return 100;
                        }
                        if (objTypeId == 114) {
                            if (member == "parse" || member == "readFile" ||
                                member == "parseAsMaps" || member == "readFileAsMaps" ||
                                member == "select" || member == "filter" || member == "sort")
                                return 102;
                            if (member == "generate" || member == "avg" ||
                                member == "min" || member == "max")
                                return 101;
                            if (member == "escape" || member == "writeFile" ||
                                member == "count" || member == "sum")
                                return 1;
                            if (member == "describe") return 103;
                            return 100;
                        }
                        if (objTypeId == 111) {
                            if (member == "gzipCompress" || member == "gzipDecompress" ||
                                member == "deflateCompress" || member == "deflateDecompress")
                                return 101;
                            return 100;
                        }
                        if (objTypeId == 109) {
                            if (member == "codepoint" || member == "length") return 1;
                            if (member == "data") return 101;
                            return 100;
                        }
                        if (objTypeId == 113) {
                            if (member == "length" || member == "capacity" ||
                                member == "byteAt" || member == "setByte" ||
                                member == "clear") return 1;
                            if (member == "copy" || member == "slice") return 113;
                            return 100;
                        }
                        if (objTypeId == 105) {
                            if (member == "nextInt" || member == "nextIntMax" ||
                                member == "nextBytes")
                                return 1;
                            if (member == "nextBool") return 3;
                            if (member == "nextDouble") return 2;
                            return 100;
                        }
                        if (objTypeId == 117) {
                            if (member == "count" || member == "remove") return 1;
                            if (member == "clear") return 4;
                            return 100;
                        }
                        if (objTypeId == 118) {
                            if (member == "length" || member == "push") return 1;
                            if (member == "clear") return 4;
                            if (member == "pop") return 0;
                            return 100;
                        }
                        if (objTypeId == 106) {
                            if (member == "getPort") return 1;
                            if (member == "getScheme" || member == "getHost" ||
                                member == "getPath" || member == "getQuery" ||
                                member == "getFragment" || member == "toString")
                                return 101;
                            return 100;
                        }
                        if (objTypeId == 108) {
                            if (member == "setHeader") return 1;
                            if (member == "get" || member == "post" ||
                                member == "put" || member == "delete")
                                return 107;
                            return 100;
                        }
                        if (objTypeId == 107) {
                            if (member == "statusCode" || member == "getStatusCode" ||
                                member == "isSuccess")
                                return 1;
                            if (member == "getBody" || member == "body" ||
                                member == "statusText" || member == "getStatusText")
                                return 101;
                            return 100;
                        }
                        if (objTypeId == 120) { // Regex
                            if (member == "match" || member == "search") return 1; // int64 (type ID 1)
                            if (member == "replace" || member == "find" || member == "group") return 101; // string (type ID 101)
                            if (member == "split") return 102; // array (type ID 102)
                            if (member == "release") return 4; // void (type ID 4)
                            return 100;
                        }
                        if (objTypeId == 121) { // Mutex
                            if (member == "lock" || member == "unlock" || member == "release") return 1; // int64 (type ID 1)
                            return 100;
                        }
                        if (objTypeId == 122) { // Uuid
                            if (member == "toString") return 101; // string (type ID 101)
                            if (member == "isNil" || member == "equals" || member == "compare") return 1; // int64 (type ID 1)
                            if (member == "release") return 4; // void (type ID 4)
                            if (member == "toBytes") return 113; // Buffer (type ID 113)
                            return 100;
                        }
                        if (objTypeId == 125) { // Decimal
                            if (member == "toString") return 101; // string (type ID 101)
                            return 100;
                        }
                        if (objTypeId == 103) {
                            if (member == "length" || member == "empty" ||
                                member == "keyType" || member == "valueType" ||
                                member == "containsInt64" || member == "containsString" ||
                                member == "containsInt8" ||
                                member == "getInt64Int64" || member == "getInt64Bool" ||
                                member == "getInt8Int64" || member == "getInt8Bool" ||
                                member == "getStringInt64" || member == "getStringBool")
                                return 1;
                            if (member == "getInt64Double" ||
                                member == "getStringDouble" || member == "getInt8Double")
                                return 2;
                            if (member == "getInt64String" ||
                                member == "getStringString" || member == "getInt8String")
                                return 101;
                            return 100;
                        }
                        // Inference for user-defined class methods
                        std::string objClassName = getLocalClassName(clsName);
                        if (objClassName.empty()) {
                            objClassName = clsName;
                        }
                        auto classIt = classes_.find(objClassName);
                        if (classIt != classes_.end()) {
                            auto retIt = classIt->second.methodReturnTypes.find(member);
                            if (retIt != classIt->second.methodReturnTypes.end()) {
                                return retIt->second;
                            }
                        }
                    }
                }
                // Direct function call: look up declared return type
                if (auto id = dynamic_cast<const ast::Identifier*>(&fc->getFunction())) {
                    const std::string& funcName = id->getName();
                    auto retIt = functionReturnTypes_.find(funcName);
                    if (retIt != functionReturnTypes_.end()) {
                        if (outClassName) {
                            auto clsIt = functionReturnClass_.find(funcName);
                            if (clsIt != functionReturnClass_.end()) {
                                *outClassName = clsIt->second;
                            }
                        }
                        return retIt->second;
                    }
                }
            }
            if (dynamic_cast<const ast::NewDictExpression*>(initializer)) {
                if (outClassName) *outClassName = "Dict";
                return 117;
            }
            // Inference from array subscript (arr[0])
            if (auto aa = dynamic_cast<const ast::ArrayAccess*>(initializer)) {
                std::string arrName;
                if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&aa->getArray())) {
                    if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                        arrName = id->getName();
                    }
                }
                if (!arrName.empty()) {
                    uint32_t elemTypeId = getLocalElementTypeId(arrName);
                    if (elemTypeId != 0) return elemTypeId;
                }
            }
        }
        return 100; // Default to Object
    }
    
    return typeIdFromDeclaredType(type);
}

void HVMCodeGenerator::emitModuleInit() {
    uint32_t funcStart = currentByteOffset_;
    size_t enterIdx = instructions_.size();

    emit(Opcode::ENTER, OperandsI{0, 0, 0});
    scopeStack_.push_back({});

    for (const auto& [className, dataOffset] : pendingSingletons_) {
        auto it = classes_.find(className);
        if (it == classes_.end()) continue;

        // Allocate: hoo_alloc(size, typeId)
        uint8_t sizeReg = emitConstant(static_cast<int64_t>(it->second.totalSize));
        emit(Opcode::MOV, OperandsR{1, sizeReg, 0, 0});
        uint8_t typeReg = emitConstant(100);
        emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_alloc_p_i8_i8");
        freeRegister(sizeReg);
        freeRegister(typeReg);

        uint8_t instanceReg = allocateRegister();
        emit(Opcode::MOV, OperandsR{instanceReg, 1, 0, 0});

        // Store singleton pointer in .data section via LDA with rs=1 (dataBase)
        uint8_t addrReg = allocateRegister();
        emit(Opcode::LDA, OperandsI{addrReg, 1, static_cast<int16_t>(dataOffset)});
        emit(Opcode::ST_D, OperandsI{instanceReg, addrReg, 0});
        freeRegister(addrReg);

        // Call the constructor to initialize the singleton instance
        emit(Opcode::MOV, OperandsR{1, instanceReg, 0, 0}); // 'this' in r1
        MangledFunctionParams mp;
        mp.modulePath = modulePath_;
        mp.className = className;
        mp.isConstructor = true;
        std::string ctorName = SymbolMangler::mangleFunctionName(mp);
        emitCall(Opcode::CALL, ctorName);

        freeRegister(instanceReg);
    }

    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});

    int32_t frameSize = alignFrameSize(-currentStackOffset_);
    instructions_[enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    Symbol sym;
    sym.name = "_F_module_init_v";
    sym.value = funcStart;
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
    sym.section_index = 0;
    module_->addSymbol(sym);

    scopeStack_.clear();
    currentStackOffset_ = 0;
}

bool HVMCodeGenerator::isDerivedFrom(const std::string& className, const std::string& potentialBase) const {
    auto it = classes_.find(className);
    if (it == classes_.end()) return false;
    if (it->second.baseClass == potentialBase) return true;
    if (!it->second.baseClass.empty()) {
        return isDerivedFrom(it->second.baseClass, potentialBase);
    }
    return false;
}

bool HVMCodeGenerator::canReadField(const std::string& fieldName, const std::string& owningClass) const {
    auto classIt = classes_.find(owningClass);
    if (classIt == classes_.end()) return true;
    auto accessIt = classIt->second.fieldAccess.find(fieldName);
    if (accessIt == classIt->second.fieldAccess.end()) return true;
    if (accessIt->second == FieldAccess::PUBLIC || accessIt->second == FieldAccess::DEFAULT_VAR) return true;
    // PRIVATE: accessible from same class or derived class
    if (currentClass_ && (currentClass_->name == owningClass || isDerivedFrom(currentClass_->name, owningClass))) {
        return true;
    }
    return false;
}

bool HVMCodeGenerator::canWriteField(const std::string& fieldName, const std::string& owningClass) const {
    auto classIt = classes_.find(owningClass);
    if (classIt == classes_.end()) return true;
    auto accessIt = classIt->second.fieldAccess.find(fieldName);
    if (accessIt == classIt->second.fieldAccess.end()) return true;
    if (accessIt->second == FieldAccess::PUBLIC) return true;
    // PRIVATE or DEFAULT_VAR: writable from same class or derived class
    if (currentClass_ && (currentClass_->name == owningClass || isDerivedFrom(currentClass_->name, owningClass))) {
        return true;
    }
    return false;
}

uint8_t HVMCodeGenerator::emitStringLiteral(const std::string& str) {
    Section* rodata = module_->getSection(".rodata");
    if (!rodata) {
        Section s;
        s.name = ".rodata";
        s.type = SectionType::SHT_RODATA;
        s.flags = SectionFlags::ALLOC;
        module_->addSection(std::move(s));
        rodata = module_->getSection(".rodata");
    }
    uint32_t offset = static_cast<uint32_t>(rodata->data.size());
    for (char c : str) rodata->data.push_back(c);
    rodata->data.push_back('\0');
    rodata->virtual_size = rodata->data.size();
    return emitRoDataAddress(offset);
}

uint32_t HVMCodeGenerator::serializeFieldTypeId(const ast::Type& type) const {
    if (auto bt = dynamic_cast<const ast::BaseType*>(&type)) {
        if (bt->isPrimitive()) {
            switch (bt->getPrimitiveType()->getKind()) {
                case ast::PrimitiveTypeKind::INT64:  return 1;   // HOO_TYPE_INT64
                case ast::PrimitiveTypeKind::INT8:   return 1;   // Promote to INT64
                case ast::PrimitiveTypeKind::BYTE:   return 1;   // Promote to INT64
                case ast::PrimitiveTypeKind::FLOAT:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:    return 2;   // HOO_TYPE_FLOAT64
                case ast::PrimitiveTypeKind::F8:     return 2;   // Promote to FLOAT64
                case ast::PrimitiveTypeKind::BOOL:   return 3;   // HOO_TYPE_BOOL
                case ast::PrimitiveTypeKind::BIT:    return 3;   // Promote to BOOL
                case ast::PrimitiveTypeKind::STRING: return 101; // HOO_TYPE_STRING
                case ast::PrimitiveTypeKind::BUFFER: return 113; // HOO_TYPE_BUFFER
                default: return 0;
            }
        }
        std::string name = bt->getIdentifier();
        if (name == "String" || name == "string") return 101;
        if (name == "Buffer" || name == "buffer") return 113; // HOO_TYPE_BUFFER
        return 0;
    }
    if (dynamic_cast<const ast::DictType*>(&type)) return 117;  // HOO_TYPE_HASHMAP
    if (dynamic_cast<const ast::ListType*>(&type)) return 118; // HOO_TYPE_ANYARRAY
    if (dynamic_cast<const ast::TensorType*>(&type)) return 126;  // HOO_TYPE_TENSOR_SERIALIZED
    return 0;
}

void HVMCodeGenerator::emitSerializeMethod(const ClassLayout& layout, const ast::ClassDeclaration& classDecl) {
    uint32_t funcStart = currentByteOffset_;
    size_t enterIdx = instructions_.size();
    emit(Opcode::ENTER, OperandsI{0, 0, 0});
    scopeStack_.push_back({});

    // 1. Create Dict<int64, any>: hoo_dict_new(HOO_TYPE_INT64=1, HOO_TYPE_ANY=0)
    uint8_t keyTypeReg = emitConstant(1);
    emit(Opcode::MOV, OperandsR{1, keyTypeReg, 0, 0});
    uint8_t valTypeReg = emitConstant(0);
    emit(Opcode::MOV, OperandsR{2, valTypeReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_dict_new_p_i8_i8");
    freeRegister(keyTypeReg);
    freeRegister(valTypeReg);
    uint8_t mapReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{mapReg, 1, 0, 0});

    // 2. For each public field, store in Dict with a stable positional key.
    // Walk the inheritance chain first so base fields are not silently lost.
    std::vector<std::pair<const ast::VariableDeclaration*, int32_t>> fields;
    std::function<void(const ast::ClassDeclaration&)> collectFields =
        [&](const ast::ClassDeclaration& decl) {
            if (decl.hasBaseClass()) {
                auto baseDecl = classDeclarations_.find(decl.getBaseClass());
                if (baseDecl != classDeclarations_.end()) collectFields(*baseDecl->second);
            }
            for (const auto& member : decl.getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                        auto offset = layout.fieldOffsets.find(var->getName());
                        if (offset != layout.fieldOffsets.end() && var->isPublic()) {
                            fields.emplace_back(var, offset->second);
                        }
                    }
                }
            }
        };
    collectFields(classDecl);
    int fieldIndex = 0;
    for (const auto& [var, fieldOffset] : fields) {

                // Load field value from this (r1)
                uint8_t fieldReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{fieldReg, 1, static_cast<int16_t>(fieldOffset)});

                // Determine HOO_TYPE for the field
                const ast::Type* fieldType = var->getType();
                uint32_t hooType = 0;
                if (fieldType) {
                    hooType = serializeFieldTypeId(*fieldType);
                }

                // Nested serializable objects are represented as ordinary
                // JSON objects in the enclosing Dict.  Convert the nested
                // object's generated JSON back to a managed Dict before
                // inserting it as an any value.
                if (auto nested = dynamic_cast<const ast::BaseType*>(fieldType)) {
                    if (!nested->isPrimitive()) {
                        auto nestedIt = classes_.find(nested->getIdentifier());
                        if (nestedIt != classes_.end() && nestedIt->second.isSerializable) {
                            MangledFunctionParams nestedMp;
                            nestedMp.modulePath = modulePath_;
                            nestedMp.className = nested->getIdentifier();
                            nestedMp.classModifiers = {"SERIALIZABLE"};
                            nestedMp.functionName = "serialize";
                            nestedMp.returnType = "ptr";
                            emit(Opcode::MOV, OperandsR{1, fieldReg, 0, 0});
                            emitCall(Opcode::CALL, SymbolMangler::mangleFunctionName(nestedMp));
                            emitCall(Opcode::CALL, "_F_M_hoo_E_String_data_p");
                            emitCall(Opcode::CALL, "_F_M_hoo_E_json_deserialize_hashmap_p_p");
                            emit(Opcode::MOV, OperandsR{fieldReg, 1, 0, 0});
                            hooType = 117;
                        }
                    }
                }

                // hashmap_set_any_i8(map, key, typeId, data)
                // Calling convention: r1=map, r2=key, r3=typeId, r5=data
                emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
                uint8_t keyReg = emitConstant(fieldIndex);
                emit(Opcode::MOV, OperandsR{2, keyReg, 0, 0});
                uint8_t typeReg = emitConstant(static_cast<int64_t>(hooType));
                emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                emit(Opcode::MOV, OperandsR{5, fieldReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_dict_set_any_i8_p_i8_i8_i8");
                freeRegister(keyReg);
                freeRegister(typeReg);
                freeRegister(fieldReg);
        ++fieldIndex;
    }

    // 3. Serialize Dict to JSON
    emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
    emitCall(Opcode::CALL, "_F_M_hoo_E_json_serialize_hashmap_p_p");
    freeRegister(mapReg);

    // 4. Return — result is already in r1
    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});

    // Fix up ENTER frame size
    int32_t frameSize = alignFrameSize(-currentStackOffset_);
    instructions_[enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    // Register symbol
    Symbol sym;
    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    mp.className = layout.name;
    for (auto mod : classDecl.getModifiers()) {
        std::string modStr = classModifierToString(mod);
        // Uppercase to match modifier code map keys in SymbolMangler
        for (char& c : modStr) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        mp.classModifiers.push_back(modStr);
    }
    mp.functionName = "serialize";
    mp.returnType = "ptr";
    sym.name = SymbolMangler::mangleFunctionName(mp);
    sym.value = funcStart;
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
    sym.section_index = 0;
    module_->addSymbol(sym);

    scopeStack_.clear();
    currentStackOffset_ = 0;
}

void HVMCodeGenerator::emitDeserializeMethod(const ClassLayout& layout, const ast::ClassDeclaration& classDecl) {
    uint32_t funcStart = currentByteOffset_;
    size_t enterIdx = instructions_.size();
    emit(Opcode::ENTER, OperandsI{0, 0, 0});
    scopeStack_.push_back({});

    // 1. Parse JSON into Dict<int64, any>: json_deserialize_hashmap(json)
    // json string pointer is in r1 (first parameter)
    emitCall(Opcode::CALL, "_F_M_hoo_E_json_deserialize_hashmap_p_p");
    uint8_t mapReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{mapReg, 1, 0, 0});

    // 2. Allocate new instance: hoo_alloc(size, typeId)
    uint8_t sizeReg = emitConstant(static_cast<int64_t>(layout.totalSize));
    emit(Opcode::MOV, OperandsR{1, sizeReg, 0, 0});
    uint8_t typeReg = emitConstant(100); // Generic Object typeId
    emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_alloc_p_i8_i8");
    freeRegister(sizeReg);
    freeRegister(typeReg);
    uint8_t instanceReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{instanceReg, 1, 0, 0});

    // Save instance on stack for later field assignments
    currentStackOffset_ -= 8;
    int32_t instanceTempOffset = currentStackOffset_;
    emit(Opcode::ST_D, OperandsI{instanceReg, 30, static_cast<int16_t>(instanceTempOffset)});

    // 3. Call constructor
    MangledFunctionParams ctorMp;
    ctorMp.modulePath = modulePath_;
    ctorMp.className = layout.name;
    ctorMp.isConstructor = true;
    std::string ctorName = SymbolMangler::mangleFunctionName(ctorMp);
    emit(Opcode::MOV, OperandsR{1, instanceReg, 0, 0});
    emitCall(Opcode::CALL, ctorName);

    // 4. For each public field, extract from Dict and assign.  The same
    // base-first traversal used by serialization keeps both sides aligned.
    std::vector<std::pair<const ast::VariableDeclaration*, int32_t>> fields;
    std::function<void(const ast::ClassDeclaration&)> collectFields =
        [&](const ast::ClassDeclaration& decl) {
            if (decl.hasBaseClass()) {
                auto baseDecl = classDeclarations_.find(decl.getBaseClass());
                if (baseDecl != classDeclarations_.end()) collectFields(*baseDecl->second);
            }
            for (const auto& member : decl.getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                        auto offset = layout.fieldOffsets.find(var->getName());
                        if (offset != layout.fieldOffsets.end() && var->isPublic()) {
                            fields.emplace_back(var, offset->second);
                        }
                    }
                }
            }
        };
    collectFields(classDecl);
    int fieldIndex = 0;
    for (const auto& [var, fieldOffset] : fields) {

                // Determine original field type for deserialization conversion
                uint32_t origFieldType = getTypeId(var->getType(), nullptr, nullptr);
                const ast::Type* fieldType = var->getType();
                uint32_t serializedType = fieldType ? serializeFieldTypeId(*fieldType) : 0;

                // Extract value from Dict by field index
                emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
                uint8_t keyReg = emitConstant(fieldIndex);
                emit(Opcode::MOV, OperandsR{2, keyReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_dict_get_any_data_i8_p_i8");
                freeRegister(keyReg);

                // Nested serializable fields arrive as a JSON object backed
                // by Dict<int64, any>. Re-encode that object and invoke the
                // nested class's generated static deserializer.
                if (auto nested = dynamic_cast<const ast::BaseType*>(fieldType)) {
                    if (!nested->isPrimitive()) {
                        auto nestedIt = classes_.find(nested->getIdentifier());
                        if (nestedIt != classes_.end() && nestedIt->second.isSerializable) {
                            MangledFunctionParams nestedMp;
                            nestedMp.modulePath = modulePath_;
                            nestedMp.className = nested->getIdentifier();
                            nestedMp.classModifiers = {"SERIALIZABLE"};
                            nestedMp.functionName = "deserialize";
                            nestedMp.returnType = "ptr";
                            nestedMp.isStatic = true;
                            nestedMp.parameterTypes = {"string"};
                            emitCall(Opcode::CALL, "_F_M_hoo_E_json_serialize_hashmap_p_p");
                            emitCall(Opcode::CALL, "_F_M_hoo_E_String_data_p");
                            emitCall(Opcode::CALL, SymbolMangler::mangleFunctionName(nestedMp));
                            serializedType = origFieldType;
                        }
                    }
                }

                // r1 now has the value.data — reverse type promotion if needed
                if (serializedType != origFieldType && serializedType != 0) {
                    if (origFieldType == 5 || origFieldType == 6) {
                        // INT8/BYTE promoted to INT64: truncate to 8 bits
                        uint8_t maskReg = emitConstant(0xFF);
                        emit(Opcode::MOV, OperandsR{2, maskReg, 0, 0});
                        emit(Opcode::LOGIC, OperandsR{1, 1, 2, 0}); // AND
                        freeRegister(maskReg);
                        if (origFieldType == 5) {
                            // INT8: sign-extend from 8 bits
                            uint8_t shiftReg = emitConstant(56);
                            emit(Opcode::MOV, OperandsR{2, shiftReg, 0, 0});
                            emit(Opcode::SHIFT, OperandsR{1, 1, 2, 0}); // SHL by 56
                            uint8_t shiftReg2 = emitConstant(56);
                            emit(Opcode::MOV, OperandsR{2, shiftReg2, 0, 0});
                            emit(Opcode::SHIFT, OperandsR{1, 1, 2, 2}); // SAR by 56
                            freeRegister(shiftReg2);
                            freeRegister(shiftReg);
                        }
                    } else if (origFieldType == 8) {
                        // BIT promoted to BOOL: mask down to 0/1
                        uint8_t maskReg = emitConstant(1);
                        emit(Opcode::MOV, OperandsR{2, maskReg, 0, 0});
                        emit(Opcode::LOGIC, OperandsR{1, 1, 2, 0}); // AND
                        freeRegister(maskReg);
                    } else if (origFieldType == 9) {
                        // F8 values are stored as float64 bits in registers and
                        // fields, so the deserialized double needs no conversion.
                    } else if (origFieldType == 113) {
                        // Buffer fields are JSON round-tripped as base64 and
                        // decoded back into a Buffer handle by the JSON layer,
                        // so the value is already in field-ready form.
                    }
                }

                // Store the extracted value into the field
                uint8_t instReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{instReg, 30, static_cast<int16_t>(instanceTempOffset)});
                emit(Opcode::ST_D, OperandsI{1, instReg, static_cast<int16_t>(fieldOffset)});
                freeRegister(instReg);

        ++fieldIndex;
    }

    // 5. Return the instance
    emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(instanceTempOffset)});
    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});

    // Fix up ENTER frame size
    int32_t frameSize = alignFrameSize(-currentStackOffset_);
    instructions_[enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    // Register symbol
    Symbol sym;
    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    mp.className = layout.name;
    for (auto mod : classDecl.getModifiers()) {
        std::string modStr = classModifierToString(mod);
        for (char& c : modStr) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        mp.classModifiers.push_back(modStr);
    }
    mp.functionName = "deserialize";
    mp.returnType = "ptr";
    mp.isStatic = true;
    mp.parameterTypes = {"string"};
    sym.name = SymbolMangler::mangleFunctionName(mp);
    sym.value = funcStart;
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
    sym.section_index = 0;
    module_->addSymbol(sym);

    scopeStack_.clear();
    currentStackOffset_ = 0;
}
} // namespace hooc
