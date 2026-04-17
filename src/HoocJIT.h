#pragma once

////////////////////////////////////////////////////////////////////////////////
/// @file HoocJIT.h
/// @brief JIT Compilation and Execution Engine for the Hooc Compiler
///
/// @mainpage HoocJIT - Usage Guide
///
/// @section overview Overview
///
/// HoocJIT provides just-in-time compilation and execution for Hooc source code.
/// It wraps LLVM's ORC (Object Retrieval Compilation) JIT infrastructure to
/// compile Hooc source into machine code and execute it directly.
///
/// @section workflow Basic Workflow
///
/// @code
/// 1. Create HoocJIT instance (initializes LLVM, runtime registry)
/// 2. Call compile() with Hooc source code
/// 3. Call execute() or executeFunction() to run compiled code
/// @endcode
///
/// @section example Examples
///
/// @subsection basic Basic Usage (void function)
///
/// @code
/// #include "HoocJIT.h"
///
/// HoocJIT jit;
/// auto result = jit.compile("myModule", R"(
///     func main() -> void {
///         print("Hello, World!");
///     }
/// )");
///
/// if (result.success) {
///     auto exec = jit.execute("main");
///     if (!exec) {
///         std::cerr << jit.getLastError() << std::endl;
///     }
/// }
/// @endcode
///
/// @subsection typed Typed Return Values
///
/// @code
/// // Execute function and get return value
/// auto result = jit.executeFunction<int64_t>("add", 40, 2);
/// if (result.success) {
///     std::cout << "Result: " << result.value << std::endl;  // 42
/// }
/// @endcode
///
/// @subsection string String Return Values
///
/// @code
/// // Functions returning string
/// auto result = jit.executeFunction<std::string>("greet");
/// if (result.success) {
///     std::string greeting = result.value;  // Use the returned string
/// }
/// @endcode
///
/// @subsection error Error Handling
///
/// @code
/// // Check for compilation errors
/// auto result = jit.compile("module", source);
/// if (!result.success) {
///     std::cerr << "Compilation failed: " << result.error << std::endl;
///     return;
/// }
///
/// // Check for execution errors
/// auto exec = jit.execute("main");
/// if (!exec) {
///     std::cerr << "Execution failed: " << jit.getLastError() << std::endl;
/// }
/// @endcode
///
/// @section functions Function Reference
///
/// | Function | Description |
/// |----------|-------------|
/// | `compile(moduleName, source)` | Compiles Hooc source, returns IR on success |
/// | `execute(functionName)` | Executes a void function |
/// | `executeFunction<T>(name)` | Executes function, returns `TypedExecutionResult<T>` |
/// | `executeFunction<T>(name, args...)` | Same as above, but passes arguments |
/// | `lookup(symbolName)` | Looks up a JIT symbol (function, global) |
/// | `getLastError()` | Returns the last error message |
/// | `hasError()` | Returns true if an error occurred |
/// | `clearError()` | Clears the error state |
///
/// @section result_types Result Types
///
/// @subsection compileresult CompileResult
/// Returned by `compile()`:
/// - `success`: true if compilation succeeded
/// - `error`: error message if failed
/// - `ir`: generated LLVM IR string (if success)
///
/// @subsection executionresult ExecutionResult
/// Returned by `execute()`:
/// - `success`: true if execution succeeded
/// - `error`: error message if failed
///
/// @subsection typedexecutionresult TypedExecutionResult<T>
/// Returned by `executeFunction<T>()`:
/// - `success`: true if execution succeeded
/// - `error`: error message if failed
/// - `value`: the return value of the function (type T)
///
/// @section notes Implementation Notes
///
/// - Uses LLVM ORC LLJIT for JIT compilation
/// - Runtime functions (strings, arrays, memory) registered automatically
/// - Exception handling: all exceptions during execution are caught and reported
/// - Move-only: HoocJIT cannot be copied, only moved
///
/// @section threading Thread Safety
///
/// HoocJIT is not thread-safe. Do not call compile() or execute() from
/// multiple threads concurrently on the same HoocJIT instance.
///
/// @section see_also See Also
///
/// - HooCompiler: The compilation pipeline
/// - RuntimeRegistry: Runtime function registration
/// - LLVM ORC Documentation: https://llvm.org/docs/ORC.html
///
////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

// ============================================================================
// Forward Declarations
// ============================================================================

class HooCompiler;

// ============================================================================
// Result Types
// ============================================================================

/// @brief Result of a compilation operation
/// @details Contains the outcome of compiling Hooc source code, including
///         success status, error message (if any), and generated IR
struct CompileResult {
    /// @brief Whether compilation succeeded
    bool success;

    /// @brief Error message if compilation failed, empty otherwise
    std::string error;

    /// @brief Generated LLVM IR string if compilation succeeded
    std::string ir;

    /// @brief Creates a successful compilation result
    /// @param ir The generated LLVM IR string (defaults to empty)
    /// @return A CompileResult with success=true and the provided IR
    static CompileResult ok(std::string ir = {}) {
        return {true, {}, std::move(ir)};
    }

    /// @brief Creates a failed compilation result
    /// @param error The error message describing the failure
    /// @return A CompileResult with success=false and the error message
    static CompileResult fail(std::string error) {
        return {false, std::move(error), {}};
    }
};

/// @brief Result of executing a void function
/// @details Contains the outcome of executing a function that returns void
struct ExecutionResult {
    /// @brief Whether execution succeeded
    bool success;

    /// @brief Error message if execution failed, empty otherwise
    std::string error;

    /// @brief Creates a successful execution result
    /// @return An ExecutionResult with success=true
    static ExecutionResult ok() {
        return {true, {}};
    }

    /// @brief Creates a failed execution result
    /// @param error The error message describing the failure
    /// @return An ExecutionResult with success=false and the error message
    static ExecutionResult fail(std::string error) {
        return {false, std::move(error)};
    }
};

// ============================================================================
// Symbol Handle
// ============================================================================

/// @brief Alias for LLVM ORC executor symbol definition
/// @details Represents a resolved symbol in the JIT execution environment
using Symbol = llvm::orc::ExecutorSymbolDef;

// ============================================================================
// Typed Execution Result
// ============================================================================

/// @brief Result of executing a typed function
/// @details Template struct containing the outcome of executing a function
///         with a known return type. Includes the return value on success.
/// @tparam ReturnType The return type of the executed function
template<typename ReturnType>
struct TypedExecutionResult {
    /// @brief Whether execution succeeded
    bool success;

    /// @brief Error message if execution failed, empty otherwise
    std::string error;

    /// @brief The return value of the function (valid if success=true)
    ReturnType value;

    /// @brief Creates a successful execution result with a value
    /// @param val The return value from the function
    /// @return A TypedExecutionResult with success=true and the value
    static TypedExecutionResult successResult(ReturnType val) {
        return {true, {}, std::move(val)};
    }

    /// @brief Creates a failed execution result
    /// @param err The error message describing the failure
    /// @return A TypedExecutionResult with success=false and the error
    static TypedExecutionResult failure(std::string err) {
        return {false, std::move(err), {}};
    }
};

/// @brief Specialization of TypedExecutionResult for void functions
/// @details Contains no value field since void functions return nothing
template<>
struct TypedExecutionResult<void> {
    /// @brief Whether execution succeeded
    bool success;

    /// @brief Error message if execution failed, empty otherwise
    std::string error;

    /// @brief Creates a successful execution result
    /// @return A TypedExecutionResult<void> with success=true
    static TypedExecutionResult successResult() {
        TypedExecutionResult<void> result;
        result.success = true;
        result.error.clear();
        return result;
    }

    /// @brief Creates a failed execution result
    /// @param err The error message describing the failure
    /// @return A TypedExecutionResult<void> with success=false and the error
    static TypedExecutionResult failure(std::string err) {
        TypedExecutionResult<void> result;
        result.success = false;
        result.error = std::move(err);
        return result;
    }
};

// ============================================================================
// HoocJIT - JIT Compilation and Execution Engine
// ============================================================================

/// @brief JIT Compilation and Execution Engine for the Hooc Compiler
/// @details Provides just-in-time compilation and execution of Hooc source code.
///         Wraps LLVM's ORC JIT infrastructure to compile Hooc source into
///         machine code and execute it directly.
/// @note This class is move-only and cannot be copied.
/// @note This class is not thread-safe.
class HoocJIT {

public:

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /// @brief Constructs a new HoocJIT instance
    /// @details Initializes LLVM ORC JIT, registers runtime functions (strings,
    ///         arrays), and creates an HooCompiler instance.
    /// @throws std::runtime_error if JIT initialization fails
    HoocJIT();

    /// @brief Destructor - cleans up JIT and compiler resources
    ~HoocJIT();

    /// @brief Deleted copy constructor - HoocJIT cannot be copied
    HoocJIT(const HoocJIT&)            = delete;

    /// @brief Deleted copy assignment - HoocJIT cannot be copied
    HoocJIT& operator=(const HoocJIT&) = delete;

    /// @brief Move constructor
    /// @param other The HoocJIT to move from
    /// @details Transfers ownership of JIT and compiler resources
    HoocJIT(HoocJIT&& other) noexcept;

    /// @brief Move assignment operator
    /// @param other The HoocJIT to move from
    /// @return Reference to this HoocJIT
    /// @details Transfers ownership of JIT and compiler resources
    HoocJIT& operator=(HoocJIT&& other) noexcept;

    // ========================================================================
    // Compilation
    // ========================================================================

    /// @brief Compiles Hooc source code to LLVM IR and adds it to the JIT
    /// @param moduleName Unique identifier for the module (used for lookup)
    /// @param sourceCode The Hooc source code to compile
    /// @return CompileResult containing success status, IR (if successful),
    ///         and error message (if failed)
    /// @details Parses, type-checks, and compiles the source code. On success,
    ///         the compiled code is immediately available for execution via
    ///         execute() or executeFunction().
    CompileResult compile(const std::string& moduleName,
                         const std::string& sourceCode);

    // ========================================================================
    // Execution
    // ========================================================================

    /// @brief Executes a void function by name
    /// @param functionName Name of the function to execute (must have been
    ///                     compiled and added to the JIT)
    /// @return std::optional<ExecutionResult> - contains ExecutionResult on success,
    ///         std::nullopt if the function was not found or an exception occurred
    /// @details Looks up the function in the JIT, casts it to a void function
    ///         pointer, and executes it. Any exceptions during execution are
    ///         caught and reported via getLastError().
    /// @see executeFunction() for executing functions with return values
    std::optional<ExecutionResult> execute(const std::string& functionName);

    /// @brief Executes a function and returns its value
    /// @tparam ReturnType The return type of the function
    /// @param functionName Name of the function to execute
    /// @return TypedExecutionResult<ReturnType> containing:
    ///         - success: true if execution succeeded
    ///         - value: the return value (valid if success=true)
    ///         - error: error message (valid if success=false)
    /// @details Template version for functions with non-void return types.
    ///         The function signature must match: ReturnType func()
    /// @par Example:
    /// @code
    /// auto result = jit.executeFunction<int64_t>("getValue");
    /// if (result.success) {
    ///     int64_t val = result.value;
    /// }
    /// @endcode
    template<typename ReturnType>
    TypedExecutionResult<ReturnType> executeFunction(const std::string& functionName);

    /// @brief Executes a function with arguments and returns its value
    /// @tparam ReturnType The return type of the function
    /// @tparam Args Parameter pack types for function arguments
    /// @param functionName Name of the function to execute
    /// @param args Arguments to pass to the function
    /// @return TypedExecutionResult<ReturnType> containing:
    ///         - success: true if execution succeeded
    ///         - value: the return value (valid if success=true)
    ///         - error: error message (valid if success=false)
    /// @details Template version that supports passing arguments to the function.
    ///         The function signature must match: ReturnType func(Args...)
    /// @par Example:
    /// @code
    /// auto result = jit.executeFunction<int64_t>("add", 40, 2);
    /// if (result.success) {
    ///     std::cout << result.value << std::endl;  // 42
    /// }
    /// @endcode
    template<typename ReturnType, typename... Args>
    TypedExecutionResult<ReturnType> executeFunction(
        const std::string& functionName, Args&&... args);

    /// @brief Looks up a symbol (function, global variable) in the JIT
    /// @param symbolName Name of the symbol to look up
    /// @return std::optional<Symbol> containing the symbol on success,
    ///         std::nullopt if not found
    /// @details Resolves a symbol name to its address in the JIT execution
    ///         environment. Can be used for dynamic function call or to verify
    ///         that a symbol exists.
    /// @note Error message available via getLastError() if lookup fails
    std::optional<Symbol> lookup(const std::string& symbolName);

    // ========================================================================
    // Error Handling
    // ========================================================================

    /// @brief Returns the last error message
    /// @return String containing the most recent error, empty if no error
    /// @details Returns the error from the last failed operation (compile,
    ///         execute, lookup). Cleared automatically on successful operations.
    std::string getLastError() const {
        return lastError_;
    }

    /// @brief Checks if an error occurred in the last operation
    /// @return true if an error occurred, false otherwise
    /// @details Convenience method to check for errors without retrieving
    ///         the message.
    bool hasError() const {
        return !lastError_.empty();
    }

    /// @brief Clears the current error state
    /// @details Resets lastError_ to empty. Call this before retrying
    ///         an operation after a failure.
    void clearError() {
        lastError_.clear();
    }

    // ========================================================================
    // Accessors
    // ========================================================================

    /// @brief Gets a reference to the underlying LLVM LLJIT instance
    /// @return Reference to the LLJIT engine
    /// @details Provides access to the underlying JIT for advanced operations
    ///         such as custom symbol resolution, linking, or inspection.
    /// @note Use with caution - direct manipulation may break HoocJIT invariants
    llvm::orc::LLJIT& getJIT() {
        return *jit_;
    }

    /// @brief Gets a const reference to the underlying LLVM LLJIT instance
    /// @return Const reference to the LLJIT engine
    llvm::orc::LLJIT& getJIT() const {
        return *jit_;
    }

private:

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    /// @brief Initializes the JIT and compiler
    /// @return true if initialization succeeded, false otherwise
    /// @details Called by constructor. Sets up LLVM native target, creates
    ///         LLJIT instance, and registers runtime functions.
    bool initialize();

    /// @brief Adds a compiled module to the JIT
    /// @param module The LLVM module to add (ownership transferred)
    /// @return true if module was added successfully
    bool addModuleToJIT(std::unique_ptr<llvm::Module> module);

    /// @brief Verifies and adds a module to the JIT
    /// @param module The module to verify and add
    /// @param outIR Output parameter for the module's IR string
    /// @return true if verification passed and module was added
    bool verifyAndAddModule(std::unique_ptr<llvm::Module> module,
                            std::string& outIR);

    /// @brief Extracts IR string from an LLVM module
    /// @param module The module to stringify
    /// @return The module's LLVM IR representation
    std::string getIRFromModule(const llvm::Module& module) const;

    /// @brief Sets the current error message
    /// @param error The error message to set
    void setError(std::string error) {
        lastError_ = std::move(error);
    }

    /// @brief Looks up a symbol and returns its address
    /// @param symbolName Name of the symbol to look up
    /// @return std::optional containing the address on success, nullopt on failure
    /// @details Internal helper that centralizes symbol lookup logic.
    ///         Sets lastError_ on failure.
    std::optional<llvm::JITTargetAddress> lookupAddress(const std::string& symbolName);

    // ========================================================================
    // State
    // ========================================================================

    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<HooCompiler>       compiler_;
    std::string                       lastError_;
};

// ============================================================================
// Template Implementation
// ============================================================================

namespace detail {

/// @brief Helper to call a JIT-compiled function with arguments
/// @tparam ReturnType Return type of the function
/// @tparam Args Parameter pack types
/// @param addr The function address to call
/// @param args Arguments to forward to the function
/// @return The return value of the called function
/// @details Casts the address to the appropriate function pointer type
///         and calls it with perfect forwarding of arguments.
template<typename ReturnType, typename... Args>
ReturnType executeWithArgs(uintptr_t addr, Args&&... args) {
    using FuncPtr = ReturnType(*)(Args...);
    auto funcPtr = reinterpret_cast<FuncPtr>(addr);
    return funcPtr(std::forward<Args>(args)...);
}

/// @brief Specialization for void return type (no return value)
/// @param addr The function address to call
/// @details Calls the void function without attempting to return a value.
template<>
inline void executeWithArgs<void>(uintptr_t addr) {
    using FuncPtr = void(*)();
    auto funcPtr = reinterpret_cast<FuncPtr>(addr);
    funcPtr();
}

}  // namespace detail

/// @brief Executes a function with no arguments and returns its value
/// @tparam ReturnType The return type of the function
/// @param functionName Name of the function to execute
/// @return TypedExecutionResult<ReturnType> with success status and value
/// @details Looks up the function address, calls it with no arguments,
///         and wraps the result. All exceptions are caught and reported.
template<typename ReturnType>
TypedExecutionResult<ReturnType> HoocJIT::executeFunction(
    const std::string& functionName) {

    auto addrOpt = lookupAddress(functionName);
    if (!addrOpt) {
        return TypedExecutionResult<ReturnType>::failure(lastError_);
    }

    try {
        ReturnType result = detail::executeWithArgs<ReturnType>(*addrOpt);
        return TypedExecutionResult<ReturnType>::successResult(std::move(result));
    }
    catch (const std::exception& e) {
        return TypedExecutionResult<ReturnType>::failure(
            std::string("Exception during execution: ") + e.what());
    }
    catch (...) {
        return TypedExecutionResult<ReturnType>::failure(
            "Unknown exception during execution");
    }
}

/// @brief Specialization for void return type with no arguments
/// @param functionName Name of the function to execute
/// @return TypedExecutionResult<void> with success status
template<>
inline TypedExecutionResult<void> HoocJIT::executeFunction<void>(
    const std::string& functionName) {

    auto addrOpt = lookupAddress(functionName);
    if (!addrOpt) {
        return TypedExecutionResult<void>::failure(lastError_);
    }

    try {
        detail::executeWithArgs<void>(*addrOpt);
        return TypedExecutionResult<void>::successResult();
    }
    catch (const std::exception& e) {
        return TypedExecutionResult<void>::failure(
            std::string("Exception during execution: ") + e.what());
    }
    catch (...) {
        return TypedExecutionResult<void>::failure(
            "Unknown exception during execution");
    }
}

/// @brief Executes a function with arguments and returns its value
/// @tparam ReturnType The return type of the function
/// @tparam Args Parameter pack types for function arguments
/// @param functionName Name of the function to execute
/// @param args Arguments to pass to the function
/// @return TypedExecutionResult<ReturnType> with success status and value
/// @details Looks up the function address, calls it with the provided
///         arguments, and wraps the result. Arguments are perfectly forwarded.
template<typename ReturnType, typename... Args>
TypedExecutionResult<ReturnType> HoocJIT::executeFunction(
    const std::string& functionName, Args&&... args) {

    auto addrOpt = lookupAddress(functionName);
    if (!addrOpt) {
        return TypedExecutionResult<ReturnType>::failure(lastError_);
    }

    try {
        ReturnType result = detail::executeWithArgs<ReturnType>(
            *addrOpt, std::forward<Args>(args)...);
        return TypedExecutionResult<ReturnType>::successResult(std::move(result));
    }
    catch (const std::exception& e) {
        return TypedExecutionResult<ReturnType>::failure(
            std::string("Exception during execution: ") + e.what());
    }
    catch (...) {
        return TypedExecutionResult<ReturnType>::failure(
            "Unknown exception during execution");
    }
}

}  // namespace hooc
