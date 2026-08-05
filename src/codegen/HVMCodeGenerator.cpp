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





static uint32_t hashMapKeyTypeId(const ast::HashMapType& type);
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
        {"Decimal", "decimal"},
        {"Mutex", "thread_mutex"},
        {"Array", "array"},
        {"Map", "map"},
        {"Buffer", "buffer"},
        {"Random", "random"},
        {"AnyArray", "anyarray"},
        {"HashMap", "hashmap"},
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
        {"HashMap", 117},
        {"AnyArray", 118},
        {"DateTime", 119},
        {"Regex", 120},
        {"Mutex", 121},
        {"Uuid", 122},
        {"Decimal", 125},
    };
    auto it = typeIds.find(className);
    return it != typeIds.end() ? it->second : 100;
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
    return functionName == "buffer_fromBytes";
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

static bool isHooModuleFreeFunction(const std::string& functionName) {
    return isJsonFreeFunction(functionName) || isBufferFreeFunction(functionName) ||
           isCsvFreeFunction(functionName) || isFsFreeFunction(functionName) ||
           isDatetimeFreeFunction(functionName) || isEncodingFreeFunction(functionName) ||
           isMathFreeFunction(functionName) || isHashingFreeFunction(functionName) ||
           isSystemFreeFunction(functionName) || isProcessFreeFunction(functionName) ||
           isRegexFreeFunction(functionName) || isThreadFreeFunction(functionName) ||
           isUuidFreeFunction(functionName) || isCharacterFreeFunction(functionName) ||
           isPathFreeFunction(functionName) || isArgsFreeFunction(functionName) ||
           isStringFreeFunction(functionName);
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
    if (isBufferFreeFunction(functionName)) return 113;
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
    if (name == "HashMap" || name == "AnyArray") return "hoo.collections";
    if (name == "StringBuilder") return "hoo.string";

    // Free functions or other symbols with prefixes
    if (name.rfind("datetime_", 0) == 0) return "hoo.datetime";
    if (name.rfind("fs_", 0) == 0) return "hoo.io";
    if (name.rfind("json_", 0) == 0) return "hoo.json";
    if (name.rfind("buffer_", 0) == 0) return "hoo.buffer";
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
    instructions_.clear();
    currentByteOffset_ = 0;
    errors_.clear();
    scopeStack_.clear();
    currentStackOffset_ = 0;
    allLabels_.clear();
    symbolFixups_.clear();
    functionReturnTypes_.clear();
    functionReturnClass_.clear();
    functionFutureElementTypes_.clear();

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
                if (funcDecl->getReturnType()) {
                    std::string clsName;
                    functionReturnTypes_[funcDecl->getName()] = typeIdFromDeclaredType(funcDecl->getReturnType(), &clsName);
                    if (!clsName.empty()) {
                        functionReturnClass_[funcDecl->getName()] = clsName;
                    }
                } else {
                    functionReturnTypes_[funcDecl->getName()] = 4;
                }
            }
        }
    }

    // Register class modifier metadata before validation so forward references
    // between serializable/service classes can be resolved consistently.
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto classDecl = dynamic_cast<const ast::ClassDeclaration*>(decl.get())) {
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
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                        layout.fieldOffsets[var->getName()] = currentOffset;
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

            // Index methods for name-based mangling resolution
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto fn = dynamic_cast<const ast::FunctionDeclaration*>(declMember)) {
                        methodNameToClass_[fn->getName()] = layout.name;
                        layout.privateMethods[fn->getName()] = fn->isPrivate();
                        if (fn->getReturnType()) {
                            layout.methodReturnTypes[fn->getName()] = typeIdFromDeclaredType(fn->getReturnType());
                        } else {
                            layout.methodReturnTypes[fn->getName()] = 4; // void
                        }
                    } else if (auto overList = dynamic_cast<const ast::OverloadList*>(declMember)) {
                        for (const auto& fn : overList->getFunctions()) {
                            isOverloadedMethod_[layout.name][fn->getName()] = true;
                            if (fn->getReturnType()) {
                                layout.methodReturnTypes[fn->getName()] = typeIdFromDeclaredType(fn->getReturnType());
                            } else {
                                layout.methodReturnTypes[fn->getName()] = 4;
                            }
                        }
                    }
                }
            }
            classes_[layout.name].privateMethods = layout.privateMethods;

            // Serializable validation
            if (layout.isSerializable) {
                validateSerializableClass(*classDecl, layout, layout.name);
            }

            // Singleton validation: constructor must have no arguments
            if (layout.isSingleton) {
                for (const auto& member : classDecl->getBody().getMembers()) {
                    if (auto ctor = member->getConstructor()) {
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
    scopeStack_.push_back({});
    emit(Opcode::ENTER, OperandsI{0, 0, 0});

    uint8_t firstArgReg = isMethod ? 2 : 1;
    // Available arg regs: r1,r2,r3,r5,r6,r7,r8 (plain, 7 max) or r2,r3,r5,r6,r7,r8 (method, 6 max)
    uint8_t maxArgRegs = isMethod ? 6 : 7;

    auto mapParams = [&](const auto& params) {
        for (size_t i = 0; i < params.size() && i < maxArgRegs; ++i) {
            std::string paramClassName;
            uint32_t paramTypeId = getTypeId(&params[i]->getType(), nullptr, &paramClassName);
            int32_t offset = reserveLocal(params[i]->getName(), paramTypeId, paramClassName);
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

    if (decl) {
        mp.functionName = decl->getName();
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
            mp.returnType = typeIdToMangleType(typeIdFromDeclaredType(decl->getReturnType()));
        } else {
            mp.returnType = "void";
        }
    }

    auto addParamTypes = [&](const auto& params) {
        for (const auto& param : params) {
            if (mp.isOverload) {
                mp.parameterTypes.push_back(typeIdToMangleType(typeIdFromDeclaredType(&param->getType(), nullptr)));
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
    if (isConstructor) {
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
        case 110: // Args - uses calloc, no hoo_release
        case 111: // Compression - uses std::free
        case 120: // Regex - uses delete with custom refcounting
        case 121: // Mutex - uses delete
        case 122: // Uuid - uses std::free with custom refcounting
            return false;
        default:
            return typeId >= 100;
    }
}

void HVMCodeGenerator::emitScopeCleanup(size_t from, size_t to) {
    uint8_t saveReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{saveReg, 1, 0, 0});
    for (size_t i = from; i > to; --i) {
        auto& scope = scopeStack_[i - 1];
        for (const auto& [name, local] : scope) {
            if (isArcManagedTypeId(local.typeId)) {
                emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(local.offset)});
                emitCall(Opcode::CALL, "_F_hoo_release_v_p");
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
    if (dynamic_cast<const ast::NewHashMapExpression*>(current)) return true;
    if (dynamic_cast<const ast::ArrayLiteral*>(current)) return true;
    if (dynamic_cast<const ast::TensorLiteral*>(current)) return true;
    return false;
}

void HVMCodeGenerator::endFunction(const FunctionPrologueInfo& info) {
    int32_t frameSize = -currentStackOffset_;
    instructions_[info.enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    Symbol sym;
    sym.name = info.mangledName;
    sym.value = info.funcStartOffset;
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
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

    // Phase 3: Constructor validation — exactly one constructor with zero parameters
    int ctorCount = 0;
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto ctor = member->getConstructor()) {
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
    // Check for HashMapType
    if (auto hmType = dynamic_cast<const ast::HashMapType*>(&type)) {
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
                        addError("Serializable class '" + className + "' field '" + fieldName + "': float not allowed as HashMap value type");
                        return false;
                    case ast::PrimitiveTypeKind::CHAR:
                        addError("Serializable class '" + className + "' field '" + fieldName + "': char not allowed as HashMap value type");
                        return false;
                    default:
                        return false;
                }
            }
            // Check if it's a BaseType referencing a class name
            if (bt->getIdentifier() == "String" || bt->getIdentifier() == "string") return true;
            if (bt->getIdentifier() == "Buffer" || bt->getIdentifier() == "buffer") return true;
            // Serializable class as HashMap value is NOT allowed
            addError("Serializable class '" + className + "' field '" + fieldName + "': serializable class not allowed as HashMap value type");
            return false;
        }
        if (dynamic_cast<const ast::TensorType*>(&valueType)) {
            return true;
        }
        addError("Serializable class '" + className + "' field '" + fieldName + "': unsupported HashMap value type");
        return false;
    }

    // Check for AnyArrayType
    if (dynamic_cast<const ast::AnyArrayType*>(&type)) {
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
        if (typeName == "String" || typeName == "Buffer" || typeName == "AnyArray") {
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
        addError("Serializable class '" + className + "' field '" + fieldName + "': Map type not allowed for serialization (use HashMap)");
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
        if (decl.getType()) {
            if (auto arrType = dynamic_cast<const ast::ArrayType*>(decl.getType())) {
                elemTypeId = typeIdFromDeclaredType(&arrType->getBaseType());
            } else if (auto tensorType = dynamic_cast<const ast::TensorType*>(decl.getType())) {
                elemTypeId = tensorElementTypeIdFromType(*tensorType);
            } else if (auto hashMapType = dynamic_cast<const ast::HashMapType*>(decl.getType())) {
                keyTypeId = hashMapKeyTypeId(*hashMapType);
                elemTypeId = typeIdFromDeclaredType(&hashMapType->getValueType());
            } else if (auto mapType = dynamic_cast<const ast::MapType*>(decl.getType())) {
                keyTypeId = mapKeyTypeId(*mapType);
                elemTypeId = typeIdFromDeclaredType(&mapType->getValueType());
            } else if (dynamic_cast<const ast::AnyArrayType*>(decl.getType())) {
                elemTypeId = 0;
            } else if (auto futureType = dynamic_cast<const ast::FutureType*>(decl.getType())) {
                elemTypeId = typeIdFromDeclaredType(&futureType->getElementType());
            }
        } else if (decl.getInitializer()) {
            if (auto newHash = dynamic_cast<const ast::NewHashMapExpression*>(decl.getInitializer())) {
                keyTypeId = hashMapKeyTypeId(newHash->getHashMapType());
                elemTypeId = typeIdFromDeclaredType(&newHash->getHashMapType().getValueType());
            } else if (auto newObj = dynamic_cast<const ast::NewObjectExpression*>(decl.getInitializer())) {
                if (newObj->getClassName() == "Map") {
                    keyTypeId = mapConstructorKeyTypeId(*newObj);
                    elemTypeId = mapConstructorValueTypeId(*newObj);
                }
            } else if (auto arrLit = dynamic_cast<const ast::ArrayLiteral*>(decl.getInitializer())) {
                if (arrLit->isAnyArray()) {
                    elemTypeId = 0;
                } else {
                auto& elements = arrLit->getElements()->getExpressions();
                uint32_t commonType = 100;
                for (const auto& elem : elements) {
                    uint32_t t = getTypeId(nullptr, elem.get());
                    if (t != 100) {
                        if (commonType == 100) commonType = t;
                        else if (commonType != t) { commonType = 100; break; }
                    }
                }
                elemTypeId = commonType;
                }
            } else if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(decl.getInitializer())) {
                if (auto tensorLit = dynamic_cast<const ast::TensorLiteral*>(&pe->getPrimary())) {
                    elemTypeId = tensorElementTypeIdFromLiteral(*tensorLit);
                } else if (auto arrLit = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
                    if (arrLit->isAnyArray()) elemTypeId = 0;
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
        int32_t offset = reserveLocal(decl.getName(), typeId, varClassName, elemTypeId, keyTypeId);
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
            uint8_t elemReg = emitConstant(static_cast<int64_t>(elemType));
            emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
            freeRegister(elemReg);
            const auto& dims = tensorType->getDimensions();
            for (size_t i = 0; i < dims.size() && i < 3; ++i) {
                uint8_t dimReg = visitExpression(*dims[i]);
                emit(Opcode::MOV, OperandsR{argReg(2, i), dimReg, 0, 0});
                freeRegister(dimReg);
            }
            if (dims.size() == 1) emitCall(Opcode::CALL, "_F_hoo_Tensor_new1_p_i8_i8");
            else if (dims.size() == 2) emitCall(Opcode::CALL, "_F_hoo_Tensor_new2_p_i8_i8_i8");
            else emitCall(Opcode::CALL, "_F_hoo_Tensor_new3_p_i8_i8_i8_i8");
            uint8_t reg = allocateRegister();
            emit(Opcode::MOV, OperandsR{reg, 1, 0, 0});
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
            if (mapKeyTypeId == 101) {
                addError("for-in over maps currently supports only numeric and char keys");
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
        
        // Lowered: item = iter[i] via runtime call
        emit(Opcode::MOV, OperandsR{1, iterReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, iReg, 0, 0});
        emitCall(Opcode::CALL, "_F_array_get_int64_v_p_p");
        uint8_t itemReg = allocateRegister();
        emit(Opcode::MOV, OperandsR{itemReg, 1, 0, 0});
        
        uint32_t forInElemTypeId = 100;
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(&forIn->getIterable())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&pe->getPrimary())) {
                uint32_t et = forInTypeId == 103
                    ? getLocalKeyTypeId(id->getName())
                    : getLocalElementTypeId(id->getName());
                if (et != 0) forInElemTypeId = et;
            }
        }
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
        Label* endLabel = createLabel();
        
        // 1. Register handler: CALL hoo_push_handler(catchStartLabel)
        uint8_t handlerAddrReg = allocateRegister();
        emit(Opcode::LDA, OperandsI{handlerAddrReg, 0, 0}); 
        size_t ldaIdx = instructions_.size() - 1;
        uint32_t ldaOff = currentByteOffset_ - instructions_.back().getSize();
        
        emit(Opcode::MOV, OperandsR{1, handlerAddrReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_push_handler_v_p");
        freeRegister(handlerAddrReg);

        visitStatement(tryCatch->getTryBlock());
        
        // 2. Normal path: pop handler and go to finally
        emitCall(Opcode::CALL, "_F_hoo_pop_handler_v");
        emitJump(Opcode::JMP, 0, finallyLabel);

        bindLabel(catchStartLabel);
        // Fixup LDA to point to catch handler
        int32_t catchOffset = catchStartLabel->targetByteOffset - static_cast<int32_t>(ldaOff);
        auto ldaOps = instructions_[ldaIdx].getOperands();
        auto& ldaOpsI = std::get<OperandsI>(ldaOps);
        ldaOpsI.imm15 = static_cast<int16_t>(catchOffset);
        instructions_[ldaIdx].setOperands(ldaOpsI);

        // 3. Exception path: pop handler and run catch clauses
        emitCall(Opcode::CALL, "_F_hoo_pop_handler_v");

        for (const auto& clause : tryCatch->getCatchClauses()) {
            uint8_t excReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{excReg, 1, 0, 0});
            std::string catchClassName;
            uint32_t catchTypeId = getTypeId(clause.type.get(), nullptr, &catchClassName);
            int32_t itemOffset = reserveLocal(clause.variable, catchTypeId, catchClassName);
            emit(Opcode::ST_D, OperandsI{excReg, 30, static_cast<int16_t>(itemOffset)});
            freeRegister(excReg);
            visitStatement(*clause.block);
            emitJump(Opcode::JMP, 0, finallyLabel);
        }

        // 4. Finally block — executed on both normal and catch paths
        bindLabel(finallyLabel);
        if (tryCatch->getFinallyBlock()) {
            visitStatement(*tryCatch->getFinallyBlock());
        }
        bindLabel(endLabel);
    } else if (auto throwStmt = dynamic_cast<const ast::ThrowStatement*>(&stmt)) {
        if (throwStmt->isRethrow()) {
            emitCall(Opcode::CALL, "_F_hoo_rethrow_v");
        } else {
            uint8_t excReg = visitExpression(*throwStmt->getExpression());
            emit(Opcode::MOV, OperandsR{1, excReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_throw_v_p");
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

            if (arrayLit->isAnyArray()) {
                emitCall(Opcode::CALL, "_F_hoo_anyarray_new_p");
                uint8_t arrReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});

                for (const auto& elem : elements) {
                    uint8_t elemReg = visitExpression(*elem);
                    uint32_t elemType = getTypeId(nullptr, elem.get());
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(elemType));
                    emit(Opcode::MOV, OperandsR{1, arrReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{3, elemReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_anyarray_push_i8_p_i8_i8");
                    freeRegister(typeReg);
                    freeRegister(elemReg);
                }

                return arrReg;
            }
            
            emitCall(Opcode::CALL, "_F_hoo_Array_new_p");
            uint8_t arrReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{arrReg, 1, 0, 0});
            
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

    if (auto newHash = dynamic_cast<const ast::NewHashMapExpression*>(&expr)) {
        std::string requiredModule = getRequiredModule("HashMap");
        if (!isSymbolImported("HashMap", requiredModule)) {
            addError("Use of 'HashMap' requires 'import " + requiredModule + ";'");
            return 0;
        }
        const auto& type = newHash->getHashMapType();
        uint8_t keyTypeReg = emitConstant(static_cast<int64_t>(hashMapKeyTypeId(type)));
        uint8_t valueTypeReg = emitConstant(static_cast<int64_t>(typeIdFromDeclaredType(&type.getValueType())));
        emit(Opcode::MOV, OperandsR{1, keyTypeReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, valueTypeReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_hashmap_new_p_i8_i8");
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

            if (className == "AnyArray") {
                if (argCount != 0) {
                    addError("AnyArray constructor expects zero arguments");
                    return 0;
                }
                emitCall(Opcode::CALL, "_F_hoo_anyarray_new_p");
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
        uint8_t objReg = visitExpression(memberAccess->getObject());
        int32_t offset = 0; 
        std::string foundClass;
        for (const auto& [className, layout] : classes_) {
            auto it = layout.fieldOffsets.find(memberAccess->getMember());
            if (it != layout.fieldOffsets.end()) {
                offset = it->second;
                foundClass = className;
                break;
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
            if (!objName.empty() && isBuiltinClassName(objName) && getLocalTypeId(objName) == 0 && !classes_.count(objName)) {
                resolvedClass = objName;
                isStaticCall = true;
            }
            // DateTime is in classes_ map but still supports static calls (now(), parse(), etc.)
            if (resolvedClass.empty() && objName == "DateTime" && isBuiltinClassName(objName)) {
                resolvedClass = objName;
                isStaticCall = true;
            }

            // Detect instance calls on user-defined classes
            if (resolvedClass.empty()) {
                auto it = methodNameToClass_.find(methodName);
                if (it != methodNameToClass_.end()) {
                    resolvedClass = it->second;
                    // Private access check
                    auto classIt = classes_.find(resolvedClass);
                    if (classIt != classes_.end()) {
                        auto privIt = classIt->second.privateMethods.find(methodName);
                        if (privIt != classIt->second.privateMethods.end() && privIt->second) {
                            bool canAccess = false;
                            if (currentClass_ && currentClass_->name == resolvedClass) {
                                canAccess = true;
                            } else if (currentClass_ && isDerivedFrom(currentClass_->name, resolvedClass)) {
                                canAccess = true;
                            }
                            if (!canAccess) {
                                addError("Cannot access private method '" + methodName + "' of class '" + resolvedClass + "'");
                            }
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
                    case 117: resolvedClass = "HashMap"; break;
                    case 118: resolvedClass = "AnyArray"; break;
                    case 119: resolvedClass = "DateTime"; break;
                    case 120: resolvedClass = "Regex"; break;
                    case 121: resolvedClass = "Mutex"; break;
                    case 122: resolvedClass = "Uuid"; break;
                    case 125: resolvedClass = "Decimal"; break;
                    default: break;
                }
            }

            if (resolvedClass.empty()) {
                addError("Cannot resolve method '" + methodName + "'");
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
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                freeRegister(objReg);
                for (size_t i = 0; i < argRegs.size(); ++i) {
                    emit(Opcode::MOV, OperandsR{argReg(2, i), argRegs[i], 0, 0});
                    freeRegister(argRegs[i]);
                }
            }

            if (resolvedClass == "AnyArray") {
                if (methodName == "push") {
                    uint32_t argType = 100;
                    if (funcCall->getArguments() && !funcCall->getArguments()->getArguments().empty()) {
                        argType = getTypeId(nullptr, funcCall->getArguments()->getArguments()[0].get());
                    }
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(argType));
                    emit(Opcode::MOV, OperandsR{3, 2, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_anyarray_push_i8_p_i8_i8");
                    freeRegister(typeReg);
                } else if (methodName == "length") {
                    emitCall(Opcode::CALL, "_F_hoo_anyarray_length_i8_p");
                } else if (methodName == "clear") {
                    emitCall(Opcode::CALL, "_F_hoo_anyarray_clear_v_p");
                } else if (methodName == "pop") {
                    emitCall(Opcode::CALL, "_F_hoo_anyarray_pop_data_i8_p");
                } else if (methodName == "release") {
                    emitCall(Opcode::CALL, "_F_M_hoo_E_anyarray_release_v");
                } else {
                    addError("Unsupported AnyArray method '" + methodName + "'");
                }
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }

            if (resolvedClass == "HashMap") {
                if (methodName == "count") {
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_count_i8_p");
                } else if (methodName == "clear") {
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_clear_v_p");
                } else if (methodName == "remove") {
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_remove_i8_p_i8");
                } else if (methodName == "release") {
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_release_v");
                } else {
                    addError("Unsupported HashMap method '" + methodName + "'");
                }
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
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
                if (funcCall->getArguments()) {
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                        if (mp.isOverload) {
                            mp.parameterTypes.push_back(typeIdToMangleType(inferExpressionTypeId(*arg)));
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
                if (isHooModuleFreeFunction(functionName)) {
                    mp.modulePath = std::vector<std::string>{"hoo"};
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
                    } else {
                        mp.returnType = "ptr";
                    }
                } else {
                    if (retIt != functionReturnTypes_.end()) {
                        mp.returnType = typeIdToMangleType(retIt->second);
                    } else if (externalIt != externalFunctionImports_.end()) {
                        mp.returnType = externalIt->second.second.empty() ? "void" : externalIt->second.second;
                    } else {
                        mp.returnType = "void";
                    }
                }
                if (funcCall->getArguments()) {
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                        if (mp.isOverload) {
                            mp.parameterTypes.push_back(typeIdToMangleType(inferExpressionTypeId(*arg)));
                        } else {
                            mp.parameterTypes.push_back("ptr");
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
        uint8_t dest = allocateRegister();
        Opcode op = Opcode::ARITH;
        uint16_t func = 0;
        switch (binary->getOperator()) {
            case ast::BinaryOperator::PLUS:  op = isFloatExpr ? Opcode::FLOAT_ARITH : Opcode::ARITH; func = 0; break;
            case ast::BinaryOperator::MINUS: op = isFloatExpr ? Opcode::FLOAT_ARITH : Opcode::ARITH; func = 1; break;
            case ast::BinaryOperator::MULTIPLY: op = isFloatExpr ? Opcode::FLOAT_ARITH : Opcode::ARITH; func = 2; break;
            case ast::BinaryOperator::DIVIDE: op = isFloatExpr ? Opcode::FLOAT_ARITH : Opcode::ARITH; func = isFloatExpr ? 3 : 5; break;
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
                func = 7; break;
            case ast::BinaryOperator::EQUALS: op = isFloatExpr ? Opcode::FCMP : Opcode::CMP; func = 0; break;
            case ast::BinaryOperator::NOT_EQUALS: op = Opcode::CMP; func = 1; break;
            case ast::BinaryOperator::LESS: op = isFloatExpr ? Opcode::FCMP : Opcode::CMP; func = isFloatExpr ? 1 : (isUnsigned ? 4 : 2); break;
            case ast::BinaryOperator::LESS_EQUALS: op = isFloatExpr ? Opcode::FCMP : Opcode::CMP; func = isFloatExpr ? 2 : (isUnsigned ? 5 : 3); break;
            case ast::BinaryOperator::GREATER: {
                op = isFloatExpr ? Opcode::FCMP : Opcode::CMP;
                func = isFloatExpr ? 1 : (isUnsigned ? 4 : 2);
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::GREATER_EQUALS: {
                op = isFloatExpr ? Opcode::FCMP : Opcode::CMP;
                func = isFloatExpr ? 2 : (isUnsigned ? 5 : 3);
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::AND: op = Opcode::LOGIC; func = 0; break;
            case ast::BinaryOperator::OR:  op = Opcode::LOGIC; func = 1; break;
            default: addError("Unsupported binary operator");
        }
        emit(op, OperandsR{dest, left, right, func});
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
        if (inferExpressionTypeId(logicalNot->getOperand()) == 104) {
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
        } else if (operandType == 2 || operandType == 9) {
            uint8_t zero = emitConstant(0);
            emit(Opcode::FLOAT_ARITH, OperandsR{dest, zero, src, 1});
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
            emit(op, OperandsR{resultReg, lhsReg, rhsReg, func});
            
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
                if (oldTypeId >= 100) {
                    emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(offset)});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                }
                emit(Opcode::ST_D, OperandsI{valueReg, 30, static_cast<int16_t>(offset)});
                return valueReg;
            }
        } else if (auto leftArray = dynamic_cast<const ast::ArrayAccess*>(&assign->getLeft())) {
            uint8_t objReg = visitExpression(leftArray->getArray());
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
                emitCall(Opcode::CALL, "_F_hoo_anyarray_set_i8_p_i8_i8_i8");
                freeRegister(typeReg);
            } else if (sourceTypeId == 117) {
                emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
                emit(Opcode::MOV, OperandsR{2, idxReg, 0, 0});
                if (mapValueTypeId == 0) {
                    uint8_t typeReg = emitConstant(static_cast<int64_t>(valueTypeId));
                    emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{5, valueReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_set_any_i8_p_i8_i8_i8");
                    freeRegister(typeReg);
                } else {
                    emit(Opcode::MOV, OperandsR{3, valueReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_hashmap_set_fixed_i8_p_i8_i8");
                }
            } else {
                addError("Unsupported indexed assignment target");
            }
            freeRegister(objReg);
            freeRegister(idxReg);
            return valueReg;
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&assign->getLeft())) {
            uint8_t objReg = visitExpression(leftMember->getObject());
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
        uint8_t arrReg = visitExpression(arrayAccess->getArray());
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
            emitCall(Opcode::CALL, "_F_hoo_anyarray_get_data_i8_p_i8");
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
                emitCall(Opcode::CALL, "_F_hoo_hashmap_get_any_data_i8_p_i8");
            } else {
                emitCall(Opcode::CALL, "_F_hoo_hashmap_get_fixed_data_i8_p_i8");
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
        emitCall(Opcode::CALL, "_F_hoo_throw_v_p");
        
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

int32_t HVMCodeGenerator::reserveLocal(const std::string& name, uint32_t typeId, const std::string& className, uint32_t elementTypeId, uint32_t keyTypeId) {
    currentStackOffset_ -= 8;
    if (scopeStack_.empty()) scopeStack_.push_back({});
    scopeStack_.back()[name] = {currentStackOffset_, typeId, className, elementTypeId, keyTypeId};
    return currentStackOffset_;
}

static uint32_t hashMapKeyTypeId(const ast::HashMapType& type) {
    switch (type.getKeyType()) {
        case ast::HashMapKeyType::INT64: return 1;
        case ast::HashMapKeyType::INT8: return 5;
        case ast::HashMapKeyType::BYTE: return 6;
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

bool HVMCodeGenerator::isBuiltinClassName(const std::string& name) const {
    static const std::unordered_set<std::string> builtinClasses = {
        "String", "Array", "Map", "Exception", "Character",
        "DateTime", "Fs", "Thread", "Regex",
        "Net", "URL", "HttpClient", "HttpResponse",
        "Path", "Uuid", "Compression",
        "Args", "Csv", "Console", "StringBuilder",
        "Buffer", "Random", "HashMap", "AnyArray",
        "Mutex", "Decimal"
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


uint32_t HVMCodeGenerator::typeIdFromDeclaredType(const ast::Type* type, std::string* outClassName) const {
    if (dynamic_cast<const ast::AnyType*>(type)) return 0;
    if (dynamic_cast<const ast::AnyArrayType*>(type)) {
        if (outClassName) *outClassName = "AnyArray";
        return 118;
    }
    if (dynamic_cast<const ast::HashMapType*>(type)) {
        if (outClassName) *outClassName = "HashMap";
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
    if (dynamic_cast<const ast::OptionalType*>(type)) return 100;
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
        case 0: return "any";
        case 101: return "string";
        case 104: return "tensor";
        /* Future values use the stable pointer ABI in function symbols. */
        case 123: return "ptr";
        default: return "ptr";
    }
}

uint32_t HVMCodeGenerator::tensorElementTypeIdFromType(const ast::TensorType& type) const {
    return typeIdFromDeclaredType(&type.getElementType());
}

uint32_t HVMCodeGenerator::tensorElementTypeIdFromLiteral(const ast::TensorLiteral& literal) {
    uint32_t commonType = 100;
    if (!literal.getElements()) return commonType;
    for (const auto& elem : literal.getElements()->getExpressions()) {
        uint32_t type = 100;
        if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(elem.get())) {
            if (auto nested = dynamic_cast<const ast::ArrayLiteral*>(&pe->getPrimary())) {
                type = 100;
                for (const auto& nestedElem : nested->getElements()->getExpressions()) {
                    uint32_t nestedType = getTypeId(nullptr, nestedElem.get());
                    if (nestedType != 100) {
                        if (type == 100) type = nestedType;
                        else if (type != nestedType) { type = 100; break; }
                    }
                }
            } else {
                type = getTypeId(nullptr, elem.get());
            }
        } else {
            type = getTypeId(nullptr, elem.get());
        }

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
    while (currentArray && shape.size() < 3) {
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

uint8_t HVMCodeGenerator::emitTensorLiteral(const ast::TensorLiteral& literal) {
    uint32_t elemType = tensorElementTypeIdFromLiteral(literal);
    std::vector<int64_t> shape = tensorShapeFromLiteral(literal);
    if (shape.empty()) shape.push_back(0);

    uint8_t elemReg = emitConstant(static_cast<int64_t>(elemType));
    emit(Opcode::MOV, OperandsR{1, elemReg, 0, 0});
    freeRegister(elemReg);
    for (size_t i = 0; i < shape.size() && i < 3; ++i) {
        uint8_t dimReg = emitConstant(shape[i]);
        emit(Opcode::MOV, OperandsR{argReg(2, i), dimReg, 0, 0});
        freeRegister(dimReg);
    }

    if (shape.size() == 1) emitCall(Opcode::CALL, "_F_hoo_Tensor_new1_p_i8_i8");
    else if (shape.size() == 2) emitCall(Opcode::CALL, "_F_hoo_Tensor_new2_p_i8_i8_i8");
    else emitCall(Opcode::CALL, "_F_hoo_Tensor_new3_p_i8_i8_i8_i8");

    uint8_t tensorReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{tensorReg, 1, 0, 0});
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

uint8_t HVMCodeGenerator::emitTensorVectorArith(const ast::BinaryExpression& binary, hvm::Opcode vecOp, uint16_t func) {
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
uint32_t HVMCodeGenerator::inferExpressionTypeId(const ast::Expression& expr) {
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
        if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&primary)) return arr->isAnyArray() ? 118 : 102;
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
    if (dynamic_cast<const ast::NewHashMapExpression*>(&expr)) {
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
                            {"Http", 108}, {"Response", 107}, {"HashMap", 117},
                            {"AnyArray", 118}, {"Regex", 120}, {"Mutex", 121},
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
                if (auto arr = dynamic_cast<const ast::ArrayLiteral*>(&node)) return arr->isAnyArray() ? 118 : 102;
                if (dynamic_cast<const ast::TensorLiteral*>(&node)) return 104;
                if (dynamic_cast<const ast::CharacterLiteral*>(&node)) return 109;
            }
            uint32_t inferredExprType = inferExpressionTypeId(*initializer);
            if (inferredExprType != 100) return inferredExprType;
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
            if (dynamic_cast<const ast::NewHashMapExpression*>(initializer)) {
                if (outClassName) *outClassName = "HashMap";
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

    int32_t frameSize = -currentStackOffset_;
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
                case ast::PrimitiveTypeKind::BUFFER: return 101; // Promote to STRING (base64)
                default: return 0;
            }
        }
        std::string name = bt->getIdentifier();
        if (name == "String" || name == "string") return 101;
        if (name == "Buffer" || name == "buffer") return 101; // Promoted to STRING (base64)
        return 0;
    }
    if (dynamic_cast<const ast::HashMapType*>(&type)) return 117;  // HOO_TYPE_HASHMAP
    if (dynamic_cast<const ast::AnyArrayType*>(&type)) return 118; // HOO_TYPE_ANYARRAY
    return 0;
}

void HVMCodeGenerator::emitSerializeMethod(const ClassLayout& layout, const ast::ClassDeclaration& classDecl) {
    uint32_t funcStart = currentByteOffset_;
    size_t enterIdx = instructions_.size();
    emit(Opcode::ENTER, OperandsI{0, 0, 0});
    scopeStack_.push_back({});

    // 1. Create HashMap<int64, any>: hoo_hashmap_new(HOO_TYPE_INT64=1, HOO_TYPE_ANY=0)
    uint8_t keyTypeReg = emitConstant(1);
    emit(Opcode::MOV, OperandsR{1, keyTypeReg, 0, 0});
    uint8_t valTypeReg = emitConstant(0);
    emit(Opcode::MOV, OperandsR{2, valTypeReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_hashmap_new_p_i8_i8");
    freeRegister(keyTypeReg);
    freeRegister(valTypeReg);
    uint8_t mapReg = allocateRegister();
    emit(Opcode::MOV, OperandsR{mapReg, 1, 0, 0});

    // 2. For each public field, store in HashMap with field index as key
    int fieldIndex = 0;
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto declMember = member->getDeclaration()) {
            if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                if (!var->isPublic()) continue;
                auto fieldIt = layout.fieldOffsets.find(var->getName());
                if (fieldIt == layout.fieldOffsets.end()) continue;
                int32_t fieldOffset = fieldIt->second;

                // Load field value from this (r1)
                uint8_t fieldReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{fieldReg, 1, static_cast<int16_t>(fieldOffset)});

                // Determine HOO_TYPE for the field
                const ast::Type* fieldType = var->getType();
                uint32_t hooType = 0;
                if (fieldType) {
                    hooType = serializeFieldTypeId(*fieldType);
                }

                // hashmap_set_any_i8(map, key, typeId, data)
                // Calling convention: r1=map, r2=key, r3=typeId, r5=data
                emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
                uint8_t keyReg = emitConstant(fieldIndex);
                emit(Opcode::MOV, OperandsR{2, keyReg, 0, 0});
                uint8_t typeReg = emitConstant(static_cast<int64_t>(hooType));
                emit(Opcode::MOV, OperandsR{3, typeReg, 0, 0});
                emit(Opcode::MOV, OperandsR{5, fieldReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_hashmap_set_any_i8_p_i8_i8_i8");
                freeRegister(keyReg);
                freeRegister(typeReg);
                freeRegister(fieldReg);
                ++fieldIndex;
            }
        }
    }

    // 3. Serialize HashMap to JSON
    emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
    emitCall(Opcode::CALL, "_F_M_hoo_E_json_serialize_hashmap_p_p");
    freeRegister(mapReg);

    // 4. Return — result is already in r1
    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});

    // Fix up ENTER frame size
    int32_t frameSize = -currentStackOffset_;
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

    // 1. Parse JSON into HashMap<int64, any>: json_deserialize_hashmap(json)
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

    // 4. For each public field, extract from HashMap and assign
    int fieldIndex = 0;
    for (const auto& member : classDecl.getBody().getMembers()) {
        if (auto declMember = member->getDeclaration()) {
            if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                if (!var->isPublic()) continue;
                auto fieldIt = layout.fieldOffsets.find(var->getName());
                if (fieldIt == layout.fieldOffsets.end()) continue;
                int32_t fieldOffset = fieldIt->second;

                // Determine original field type for deserialization conversion
                uint32_t origFieldType = getTypeId(var->getType(), nullptr, nullptr);
                const ast::Type* fieldType = var->getType();
                uint32_t serializedType = fieldType ? serializeFieldTypeId(*fieldType) : 0;

                // Extract value from HashMap by field index
                emit(Opcode::MOV, OperandsR{1, mapReg, 0, 0});
                uint8_t keyReg = emitConstant(fieldIndex);
                emit(Opcode::MOV, OperandsR{2, keyReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_hashmap_get_any_data_i8_p_i8");
                freeRegister(keyReg);

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
                        // F8 promoted to FLOAT64: call f8 conversion
                        emitCall(Opcode::CALL, "_F_M_hoo_E_F8_fromDouble_d_d");
                    } else if (origFieldType == 113) {
                        // BUFFER promoted to STRING: call base64 decode
                        emitCall(Opcode::CALL, "_F_M_hoo_E_Buffer_fromBase64_p_p");
                    }
                }

                // Store the extracted value into the field
                uint8_t instReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{instReg, 30, static_cast<int16_t>(instanceTempOffset)});
                emit(Opcode::ST_D, OperandsI{1, instReg, static_cast<int16_t>(fieldOffset)});
                freeRegister(instReg);

                ++fieldIndex;
            }
        }
    }

    // 5. Return the instance
    emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(instanceTempOffset)});
    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});

    // Fix up ENTER frame size
    int32_t frameSize = -currentStackOffset_;
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
