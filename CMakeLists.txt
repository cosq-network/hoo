cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(Hoo
    VERSION 1.4.0
    DESCRIPTION "Hoo – a high-performance statically-typed systems programming language"
    LANGUAGES C CXX
)

# Semantic version components also available as individual variables
set(HOO_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(HOO_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(HOO_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(HOO_VERSION       "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(HOO_BUILD_TESTS "Build Hoo unit tests" OFF)
option(HOO_BUILD_SHARED_RUNTIME "Build the Hoo runtime as a shared library" OFF)
option(ANTLR4_USE_STATIC_RUNTIME "Define ANTLR4CPP_STATIC when linking a static ANTLR4 C++ runtime" OFF)

set(ANTLR4_ROOT "" CACHE PATH "ANTLR4 C++ runtime installation prefix")
set(ANTLR4_JAR_PATH "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar" CACHE FILEPATH "Path to ANTLR4 JAR file")
set(ANTLR4_GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/parsing/Hooc.g4" CACHE FILEPATH "Path to the Hoo ANTLR4 grammar")
set(ANTLR4_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/antlr4" CACHE PATH "Directory for generated ANTLR4 C++ files")

include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
endif()

if(WIN32)
    if(NOT DEFINED LLVM_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/llvm/LLVMConfig.cmake")
        set(LLVM_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/llvm" CACHE PATH "Path to LLVM CMake package directory" FORCE)
    elseif(NOT DEFINED LLVM_DIR AND EXISTS "C:/Program Files/LLVM/lib/cmake/llvm")
        set(LLVM_DIR "C:/Program Files/LLVM/lib/cmake/llvm" CACHE PATH "Path to LLVM CMake package directory" FORCE)
    endif()
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    foreach(_hoo_lang C CXX)
        string(REGEX REPLACE "([/-])Ob1" "\\1Ob2"
            CMAKE_${_hoo_lang}_FLAGS_RELWITHDEBINFO
            "${CMAKE_${_hoo_lang}_FLAGS_RELWITHDEBINFO}")
    endforeach()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
    add_compile_options(/wd4624)
    add_compile_options(/EHa)

    function(hoo_use_static_msvc_runtime target_name)
        set_property(TARGET ${target_name} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endfunction()
endif()

# Dependencies
#
# ANTLR4 is built from source by default (FetchContent) so the runtime is
# compiled with the same toolchain/SDK as the rest of the project. Prebuilt
# runtime libraries (e.g. the Homebrew antlr4-cpp-runtime bottle) are compiled
# against an older libc++ and crash with `std::overflow_error: __next_prime
# overflow` when their std::unordered_map layout is read by newer SDK headers.
# Set HOO_ANTLR4_USE_SYSTEM=ON or -DANTLR4_ROOT=... to use a system install.
option(HOO_ANTLR4_USE_SYSTEM "Use a system-installed ANTLR4 C++ runtime instead of building from source" OFF)
if(NOT ANTLR4_ROOT AND NOT HOO_ANTLR4_USE_SYSTEM)
    include(FetchContent)
    FetchContent_Declare(
        antlr4_runtime
        URL https://github.com/antlr/antlr4/archive/refs/tags/4.13.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR runtime/Cpp
    )
    set(ANTLR4_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/antlr4_runtime-src/runtime/Cpp/runtime/src" CACHE PATH "ANTLR4 include dir" FORCE)
    set(ANTLR4_LIBRARY antlr4_static)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    set(WITH_LIBCXX OFF CACHE INTERNAL "" FORCE)
    set(WITH_STATIC_CRT ON CACHE BOOL "Build ANTLR4 with the static MSVC runtime" FORCE)
    set(ANTLR_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
    set(ANTLR_BUILD_STATIC ON CACHE INTERNAL "" FORCE)
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE INTERNAL "" FORCE)
    set(ANTLR4_USE_STATIC_RUNTIME ON)
    FetchContent_MakeAvailable(antlr4_runtime)
    if(MSVC AND TARGET antlr4_static)
        target_compile_options(antlr4_static PRIVATE
            /FI${CMAKE_SOURCE_DIR}/cmake/antlr4_msvc_patch.hpp
            /wd4100
            /wd4244
            /wd4996
        )
    endif()
    message(STATUS "ANTLR4 built from source (static)")
else()
    # Try finding ANTLR4 via CMake config first (vcpkg provides antlr4::antlr4)
    find_package(antlr4 CONFIG QUIET)

    if(antlr4_FOUND)
        message(STATUS "Found ANTLR4 via CMake config (vcpkg)")
        set(ANTLR4_INCLUDE_DIR "${antlr4_INCLUDE_DIRS}")
        if(NOT ANTLR4_INCLUDE_DIR)
            set(ANTLR4_INCLUDE_DIR "${antlr4_INCLUDE_DIR}")
        endif()
        if(NOT ANTLR4_INCLUDE_DIR)
            find_path(ANTLR4_INCLUDE_DIR antlr4-runtime.h PATH_SUFFIXES antlr4-runtime)
        endif()
        if(TARGET antlr4::antlr4)
            set(ANTLR4_LIBRARY antlr4::antlr4)
        elseif(TARGET antlr4_static)
            set(ANTLR4_LIBRARY antlr4_static)
        elseif(TARGET antlr4_runtime)
            set(ANTLR4_LIBRARY antlr4_runtime)
        else()
            message(FATAL_ERROR "ANTLR4 found via config but no known target exists")
        endif()
    else()
        find_path(ANTLR4_INCLUDE_DIR
            NAMES antlr4-runtime.h
            HINTS
                "${ANTLR4_ROOT}/include/antlr4-runtime"
                "$ENV{ANTLR4_ROOT}/include/antlr4-runtime"
                "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_TARGET_TRIPLET}/include/antlr4-runtime"
            PATH_SUFFIXES
                include/antlr4-runtime
                antlr4-runtime
        )

        find_library(ANTLR4_LIBRARY
            NAMES antlr4-runtime
            HINTS
                "${ANTLR4_ROOT}"
                "$ENV{ANTLR4_ROOT}"
                "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_TARGET_TRIPLET}/lib"
            PATH_SUFFIXES
                lib
                lib64
        )

        if(NOT ANTLR4_INCLUDE_DIR OR NOT ANTLR4_LIBRARY)
            message(FATAL_ERROR
                "ANTLR4 C++ runtime not found. Install it with a package manager or pass "
                "-DANTLR4_ROOT=/path/to/antlr4, or pass -DANTLR4_INCLUDE_DIR=... "
                "and -DANTLR4_LIBRARY=..."
            )
        endif()
    endif()
endif()

message(STATUS "ANTLR4 Include Dir: ${ANTLR4_INCLUDE_DIR}")
message(STATUS "ANTLR4 Library: ${ANTLR4_LIBRARY}")

find_package(ZLIB REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(CURL REQUIRED)
find_package(libzip REQUIRED)
find_package(zstd REQUIRED)
if(NOT TARGET zstd::libzstd_shared AND TARGET zstd::libzstd)
    add_library(zstd::libzstd_shared ALIAS zstd::libzstd)
endif()
find_package(nlohmann_json REQUIRED)

find_package(libuv CONFIG QUIET)
if(NOT libuv_FOUND)
    message(STATUS "libuv CMake config not found; building from source via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        libuv
        URL https://github.com/libuv/libuv/archive/refs/tags/v1.48.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libuv)
    if(NOT TARGET libuv::uv AND TARGET uv)
        add_library(libuv::uv ALIAS uv)
    endif()
    if(NOT TARGET libuv::uv_a AND TARGET uv_a)
        add_library(libuv::uv_a ALIAS uv_a)
    endif()
endif()

find_package(LLVM QUIET CONFIG)
if(NOT LLVM_FOUND)
    if(WIN32 AND NOT "${LLVM_DIR}" STREQUAL "" AND EXISTS "${LLVM_DIR}" AND NOT EXISTS "${LLVM_DIR}/LLVMConfig.cmake" AND NOT EXISTS "${LLVM_DIR}/llvm-config.cmake")
        message(FATAL_ERROR
            "LLVM appears to be installed at '${LLVM_DIR}', but the CMake package files are missing. "
            "This project requires LLVMConfig.cmake from a full LLVM development installation. "
            "Reinstall LLVM with development components, or point -DLLVM_DIR to a directory containing LLVMConfig.cmake."
        )
    endif()

    message(FATAL_ERROR
        "LLVM CMake package not found. Pass -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm, "
        "or add the LLVM installation prefix to CMAKE_PREFIX_PATH."
    )
endif()
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs
    Core
    ExecutionEngine
    OrcJIT
    Support
    Target
    TargetParser
    native
)

find_package(Java REQUIRED COMPONENTS Runtime)

# =============================================================================
# ANTLR4 Parser Generation
# =============================================================================

set(ANTLR4_GENERATED_FILES
    "${ANTLR4_GENERATED_DIR}/HoocLexer.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocLexer.h"
    "${ANTLR4_GENERATED_DIR}/HoocParser.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocParser.h"
    "${ANTLR4_GENERATED_DIR}/HoocVisitor.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocListener.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocBaseVisitor.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocBaseVisitor.h"
    "${ANTLR4_GENERATED_DIR}/HoocVisitor.h"
    "${ANTLR4_GENERATED_DIR}/HoocBaseListener.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocBaseListener.h"
    "${ANTLR4_GENERATED_DIR}/HoocListener.h"
)

set(ANTLR4_GENERATED_BYPRODUCTS
    "${ANTLR4_GENERATED_DIR}/Hooc.interp"
    "${ANTLR4_GENERATED_DIR}/Hooc.tokens"
    "${ANTLR4_GENERATED_DIR}/HoocLexer.interp"
    "${ANTLR4_GENERATED_DIR}/HoocLexer.tokens"
)

add_custom_command(
    OUTPUT ${ANTLR4_GENERATED_FILES}
    BYPRODUCTS ${ANTLR4_GENERATED_BYPRODUCTS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4_GENERATED_DIR}
    COMMAND ${Java_JAVA_EXECUTABLE} -jar ${ANTLR4_JAR_PATH}
        -Dlanguage=Cpp -visitor -listener
        -o ${ANTLR4_GENERATED_DIR} -package hooc
        Hooc.g4
    DEPENDS ${ANTLR4_GRAMMAR_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/parsing
    COMMENT "Generating ANTLR4 C++ files from Hooc.g4"
    VERBATIM
)

add_custom_target(generate_parser DEPENDS ${ANTLR4_GENERATED_FILES})

add_library(hoo-parser ${ANTLR4_GENERATED_FILES})
if(MSVC)
    hoo_use_static_msvc_runtime(hoo-parser)
endif()
target_include_directories(hoo-parser PUBLIC ${ANTLR4_GENERATED_DIR} ${ANTLR4_INCLUDE_DIR})
target_link_libraries(hoo-parser PUBLIC ${ANTLR4_LIBRARY})

if(MSVC)
    target_compile_options(hoo-parser PRIVATE /FS)
endif()

if(ANTLR4_USE_STATIC_RUNTIME)
    target_compile_definitions(hoo-parser PUBLIC ANTLR4CPP_STATIC)
endif()

add_dependencies(hoo-parser generate_parser)

# =============================================================================
# Runtime Library (hoort) - Standalone C/C++ runtime, no LLVM dependency
# =============================================================================

if(HOO_BUILD_SHARED_RUNTIME)
    set(HOO_RUNTIME_LIBRARY_TYPE SHARED)
else()
    set(HOO_RUNTIME_LIBRARY_TYPE STATIC)
endif()

# Runtime library sources are grouped into broad top-level modules:
#   core/          - fundamental runtime (alloc/ARC), exception, ai, overload
#   mem/           - memory & collections (buffer, array, list, dict, map, tensor)
#   text/          - string, character, regex, encoding, csv
#   data/          - json, compression, hashing, uuid, datetime, decimal, math
#   concurrency/   - thread, future, event_loop
#   system/        - system, fs, path, process, args
#   io/            - io, net
add_library(hoort ${HOO_RUNTIME_LIBRARY_TYPE}
    src/runtime/lib/core/hoo_runtime.c
    src/runtime/lib/core/hoo_ai.cpp
    src/runtime/lib/core/hoo_exception.cpp
    src/runtime/lib/core/hoo_overload.cpp
    src/runtime/lib/mem/hoo_any.cpp
    src/runtime/lib/mem/hoo_list.cpp
    src/runtime/lib/mem/hoo_dict.cpp
    src/runtime/lib/mem/hoo_generic_array.cpp
    src/runtime/lib/mem/hoo_tensor.cpp
    src/runtime/lib/mem/hoo_map.cpp
    src/runtime/lib/mem/hoo_buffer.cpp
    src/runtime/lib/mem/hoo_byte_slice.cpp
    src/runtime/lib/text/hoo_string.cpp
    src/runtime/lib/text/hoo_character.cpp
    src/runtime/lib/text/hoo_regex.cpp
    src/runtime/lib/text/hoo_encoding.cpp
    src/runtime/lib/text/hoo_csv.cpp
    src/runtime/lib/data/hoo_json.cpp
    src/runtime/lib/data/hoo_compression.cpp
    src/runtime/lib/data/hoo_hashing.cpp
    src/runtime/lib/data/hoo_uuid.cpp
    src/runtime/lib/data/hoo_datetime.cpp
    src/runtime/lib/data/hoo_decimal.cpp
    src/runtime/lib/data/hoo_math.cpp
    src/runtime/lib/concurrency/hoo_thread.cpp
    src/runtime/lib/concurrency/hoo_future.cpp
    src/runtime/lib/concurrency/hoo_event_loop.cpp
    src/runtime/lib/system/hoo_system.cpp
    src/runtime/lib/system/hoo_fs.cpp
    src/runtime/lib/system/hoo_path.cpp
    src/runtime/lib/system/hoo_process.cpp
    src/runtime/lib/system/hoo_args.cpp
    src/runtime/lib/io/hoo_io.cpp
    src/runtime/lib/io/hoo_net.cpp
)
if(MSVC)
    hoo_use_static_msvc_runtime(hoort)
endif()


target_compile_definitions(hoort PRIVATE
    $<$<CONFIG:Debug>:HOO_DEBUG_MEMORY>
)

target_link_libraries(hoort PUBLIC OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(hoort PUBLIC ZLIB::ZLIB)
target_link_libraries(hoort PUBLIC $<TARGET_NAME_IF_EXISTS:libuv::uv>)
target_link_libraries(hoort PUBLIC $<TARGET_NAME_IF_EXISTS:libuv::uv_a>)


set_source_files_properties(
    src/runtime/lib/core/hoo_runtime.c
    PROPERTIES
        LANGUAGE C
)

if(MSVC)
    set_source_files_properties(
        src/runtime/lib/core/hoo_runtime.c
        PROPERTIES
            COMPILE_OPTIONS "/TC;/std:c11;/experimental:c11atomics"
    )
endif()

# =============================================================================
# Core Compiler + HVM (hoo-core) - Merged from hvm + hoo-compiler
# =============================================================================

add_library(hoo-core
    # AST
    src/ast/SimpleASTBuilder.cpp
    src/ast/ASTImpl.cpp
    # Code generation
    src/codegen/HVMCodeGenerator.cpp
    # Compiler core
    src/core/HooCompiler.cpp
    src/core/SymbolMangler.cpp
    src/core/DefaultIOProvider.cpp
    src/core/HooCLI.cpp
    # HVM module format and JIT
    src/hvm/HOModuleBase.cpp
    src/hvm/HOModule.cpp
    src/hvm/HVMInstruction.cpp
    src/hvm/HVMModuleBundle.cpp
    src/hvm/HVMJIT.cpp
    # Archive
    src/archive/HAManifest.cpp
    src/archive/LocalImportResolver.cpp
    src/archive/HAArchiveBuilder.cpp
    src/archive/HAArchive.cpp
    src/archive/HooArchiveCompiler.cpp
    src/archive/HooArchiveLoader.cpp
    # Module system (reserved for future use)
    # Parsing (manual wrapper)
    src/parsing/HooParserWrapper.cpp
)
if(MSVC)
    hoo_use_static_msvc_runtime(hoo-core)
endif()

target_include_directories(hoo-core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${LLVM_INCLUDE_DIRS}
    ${ANTLR4_INCLUDE_DIR}
    ${ANTLR4_GENERATED_DIR}
)

target_compile_definitions(hoo-core PUBLIC HOO_VERSION="${PROJECT_VERSION}")

if(WIN32)
    set(_windows_sdk_root "C:/Program Files (x86)/Windows Kits/10")
    set(_windows_sdk_version "")
    if(EXISTS "${_windows_sdk_root}/Lib")
        file(GLOB _windows_sdk_versions "${_windows_sdk_root}/Lib/*")
        if(_windows_sdk_versions)
            list(SORT _windows_sdk_versions)
            list(GET _windows_sdk_versions -1 _windows_sdk_version)
        endif()
    endif()

    if(_windows_sdk_version)
        set(PSAPI_LIBRARY "${_windows_sdk_version}/um/x64/psapi.lib")
        set(DBGHELP_LIBRARY "${_windows_sdk_version}/um/x64/dbghelp.lib")
    else()
        set(PSAPI_LIBRARY "")
        set(DBGHELP_LIBRARY "")
    endif()
endif()

target_link_libraries(hoo-core PUBLIC
    hoort
    hoo-parser
    ${llvm_libs}
    CURL::libcurl
    libzip::zip
    $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
    $<TARGET_NAME_IF_EXISTS:zstd::libzstd_static>
    nlohmann_json::nlohmann_json
)

if(WIN32)
    if(PSAPI_LIBRARY)
        target_link_libraries(hoo-core PUBLIC ${PSAPI_LIBRARY})
    else()
        message(WARNING "Could not find psapi.lib; continuing without explicit link")
    endif()
    if(DBGHELP_LIBRARY)
        target_link_libraries(hoo-core PUBLIC ${DBGHELP_LIBRARY})
    else()
        message(WARNING "Could not find dbghelp.lib; continuing without explicit link")
    endif()
endif()

# Generated ANTLR4 parser files must exist before hoo-core sources can include them
add_dependencies(hoo-core generate_parser)

add_library(hoorepl STATIC
    src/repl/REPLSession.cpp
    src/repl/REPLSession.h
)
if(MSVC)
    hoo_use_static_msvc_runtime(hoorepl)
endif()
target_link_libraries(hoorepl PRIVATE hoo-core)
target_include_directories(hoorepl PUBLIC ${CMAKE_SOURCE_DIR}/src)

# =============================================================================
# CLI Executable
# =============================================================================

add_executable(hoo src/core/main.cpp)
if(MSVC)
    hoo_use_static_msvc_runtime(hoo)
endif()
target_link_libraries(hoo hoo-core hoorepl)
target_include_directories(hoo PRIVATE ${CMAKE_SOURCE_DIR}/src)
set_target_properties(hoo PROPERTIES ENABLE_EXPORTS ON)

# =============================================================================
# Versioning for all components
# =============================================================================
set_target_properties(hoo-parser PROPERTIES VERSION ${HOO_VERSION} SOVERSION ${HOO_VERSION_MAJOR})
set_target_properties(hoort PROPERTIES VERSION ${HOO_VERSION} SOVERSION ${HOO_VERSION_MAJOR})
set_target_properties(hoo-core PROPERTIES VERSION ${HOO_VERSION} SOVERSION ${HOO_VERSION_MAJOR})
set_target_properties(hoorepl PROPERTIES VERSION ${HOO_VERSION} SOVERSION ${HOO_VERSION_MAJOR})
set_target_properties(hoo PROPERTIES VERSION ${HOO_VERSION} SOVERSION ${HOO_VERSION_MAJOR})

# =============================================================================
# Install
# =============================================================================

install(TARGETS hoo hoo-core hoo-parser hoort hoorepl
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${ANTLR4_GENERATED_DIR}/
    DESTINATION include/hoo
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY src/
    DESTINATION include/hoo
    FILES_MATCHING PATTERN "*.h"
)

# =============================================================================
# Utility Targets
# =============================================================================

message(STATUS "ANTLR4 JAR Path: ${ANTLR4_JAR_PATH}")
message(STATUS "Grammar file: ${ANTLR4_GRAMMAR_FILE}")
message(STATUS "Generated files directory: ${ANTLR4_GENERATED_DIR}")

if(NOT EXISTS ${ANTLR4_JAR_PATH})
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/tools")
    find_program(CURL_EXECUTABLE curl)
    if(NOT CURL_EXECUTABLE)
        message(FATAL_ERROR "ANTLR4 jar not found at ${ANTLR4_JAR_PATH}, and curl is not available to download it")
    endif()
    add_custom_target(download_antlr4
        COMMAND ${CURL_EXECUTABLE} -L -o ${ANTLR4_JAR_PATH}
            https://www.antlr.org/download/antlr-4.13.2-complete.jar
        COMMENT "Downloading ANTLR4 JAR file"
        VERBATIM
    )
    add_dependencies(generate_parser download_antlr4)
endif()

add_custom_target(clean_generated
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANTLR4_GENERATED_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4_GENERATED_DIR}
    COMMENT "Cleaning generated ANTLR4 files"
)

# =============================================================================
# Tests
# =============================================================================

if(HOO_BUILD_TESTS)
    enable_testing()
    if(WIN32)
        find_package(GTest CONFIG REQUIRED)
    else()
        find_package(GTest REQUIRED)
    endif()

    add_executable(hoo-tests
        tests/test_main.cpp
        tests/parsing/ArrayLiteralParsingTest.cpp
        tests/parsing/FunctionCallParsingTest.cpp
        tests/parsing/NullableTypeParsingTest.cpp
        tests/parsing/LowPrecisionTypeParsingTest.cpp
        tests/parsing/DecimalTypeParsingTest.cpp
        tests/parsing/TensorTypeParsingTest.cpp
        tests/parsing/VariableDeclarationParseTest.cpp
        tests/parsing/IfElseIfParsingTest.cpp
        tests/parsing/WhileLoopParsingTest.cpp
        tests/parsing/ForLoopParsingTest.cpp
        tests/parsing/FunctionModifiersTest.cpp
        tests/parsing/ImportStatementParsingTest.cpp
        tests/parsing/ModuleLevelVariableParsingTest.cpp
        tests/parsing/ClassDeclarationParsingTest.cpp
        tests/parsing/SimpleASTBuilderTest.cpp
        tests/parsing/MapTypeParsingTest.cpp
        tests/parsing/DictListParsingTest.cpp
        tests/parsing/OptionalReturnTypeTest.cpp
        tests/parsing/MathModuleParsingTest.cpp
        tests/parsing/NetworkModuleParsingTest.cpp
        tests/parsing/NewExpressionParsingTest.cpp
        tests/parsing/MemberAccessParsingTest.cpp
        tests/parsing/MethodCallParsingTest.cpp
        tests/parsing/QualifiedIdentifierParsingTest.cpp
        tests/parsing/QualifiedNewExpressionParsingTest.cpp
        tests/parsing/ThisKeywordParsingTest.cpp
        tests/parsing/HooParserWrapperTest.cpp
        tests/codegen/HVMCodeGeneratorComprehensiveTest.cpp
        tests/codegen/HVMCodeGeneratorTest.cpp
        tests/runtime/HooArrayPhase7Test.cpp
        tests/runtime/HooTensorTest.cpp
        tests/runtime/StringBasicsTest.cpp
        tests/runtime/HooCharacterTest.cpp
        tests/runtime/HooCharacterExtraTest.cpp
        tests/runtime/HooRuntimeTest.cpp
        tests/runtime/HooMapTest.cpp
        tests/runtime/HooAnyCollectionsTest.cpp
        tests/runtime/HooExceptionTest.cpp
        tests/runtime/HooMathTest.cpp
        tests/runtime/HooIOTest.cpp
        tests/runtime/HooNetTest.cpp
        tests/runtime/HooEncodingTest.cpp
        tests/runtime/HooFSTest.cpp
        tests/runtime/HooDateTimeTest.cpp
        tests/runtime/HooSystemTest.cpp
        tests/runtime/HooRegexTest.cpp
        tests/runtime/HooCsvTest.cpp
        tests/runtime/HooJsonTest.cpp
        tests/runtime/HooUuidTest.cpp
        tests/runtime/HooDecimalTest.cpp
        tests/runtime/HooPathTest.cpp
        tests/runtime/HooHashingTest.cpp
        tests/runtime/HooProcessTest.cpp
        tests/runtime/HooCompressionTest.cpp
        tests/runtime/HooArgsTest.cpp
        tests/runtime/HooBufferTest.cpp
        tests/runtime/HooThreadTest.cpp
        tests/runtime/HooOverloadTest.cpp
        tests/integration/jit/HooMathJitTest.cpp
        tests/integration/jit/HooStringJitTest.cpp
        tests/integration/jit/HooMapJitTest.cpp
        tests/integration/jit/HooAnyCollectionsJitTest.cpp
        tests/integration/jit/HooArrayJitTest.cpp
        tests/integration/jit/HooTensorJitTest.cpp
        tests/integration/jit/HooDecimalJitTest.cpp
        tests/integration/jit/HooStandardLibraryJitTest.cpp
        tests/integration/jit/HooThreadJitTest.cpp
        tests/integration/jit/HooCsvJitTest.cpp
        tests/integration/jit/HooDatetimeJitTest.cpp
        tests/integration/jit/HooPathJitTest.cpp
        tests/integration/jit/HooFsJitTest.cpp
        tests/integration/jit/HooHashingJitTest.cpp
        tests/integration/jit/HooEncodingJitTest.cpp
        tests/integration/jit/HooProcessJitTest.cpp
        tests/integration/jit/HooCompressionJitTest.cpp
        tests/integration/jit/HooClassApiTest.cpp
        tests/integration/jit/HooArgsJitTest.cpp
        tests/integration/jit/HooNetJitTest.cpp
        tests/integration/jit/HooBufferJitTest.cpp
        tests/integration/jit/HooJsonJitTest.cpp
        tests/integration/jit/IntegerTypesTest.cpp
        tests/integration/jit/NewLanguageFeaturesTest.cpp
        tests/integration/jit/HooFutureJitTest.cpp
        tests/integration/future/FutureAsyncCLIIntegrationTest.cpp
        tests/integration/jit/CharacterLiteralTest.cpp
        tests/integration/jit/HooCharacterApiJitTest.cpp
        tests/integration/jit/FloatingLiteralTest.cpp
        tests/integration/jit/LowPrecisionTypesTest.cpp
        tests/integration/jit/InterpolatedStringTest.cpp
        tests/integration/jit/BugFixVerificationTest.cpp
        tests/integration/jit/HVMJITLifecycleTest.cpp
        tests/core/HooCompilerTest.cpp
        tests/core/HooCLITest.cpp
        tests/integration/cli/HooCLIIntegrationTest.cpp
        tests/integration/primitive-types/PrimitiveTypesCLIIntegrationTest.cpp
        tests/integration/primitive-types/PrimitiveTypeOperationMatrixTest.cpp
        tests/integration/primitive-types/PrimitiveTypeFailureTest.cpp
        tests/integration/primitive-types/CompositeTypesCLIIntegrationTest.cpp
        tests/integration/expressions/ExpressionOperatorsIntegrationTest.cpp
        tests/integration/literals/LiteralIntegrationTest.cpp
        tests/integration/collections/CollectionIntegrationTest.cpp
        tests/integration/generic_array/GenericArrayCLIIntegrationTest.cpp
        tests/integration/args/ArgsCLIIntegrationTest.cpp
        tests/integration/statements/ForLoopCLIIntegrationTest.cpp
        tests/integration/statements/WhileLoopCLIIntegrationTest.cpp
        tests/integration/statements/IfStatementCLIIntegrationTest.cpp
        tests/integration/statements/ExceptionHandlingCLIIntegrationTest.cpp
        tests/integration/statements/SwitchStatementCLIIntegrationTest.cpp
        tests/integration/statements/VariableDeclarationCLIIntegrationTest.cpp
        tests/integration/buffer/BufferCLIIntegrationTest.cpp
        tests/integration/byte_slice/ByteSliceCLIIntegrationTest.cpp
        tests/integration/character/CharacterCLIIntegrationTest.cpp
        tests/integration/encoding/EncodingCLIIntegrationTest.cpp
        tests/integration/hashing/HashingCLIIntegrationTest.cpp
        tests/integration/csv/CsvCLIIntegrationTest.cpp
        tests/integration/compression/CompressionCLIIntegrationTest.cpp
        tests/integration/datetime/DatetimeCLIIntegrationTest.cpp
        tests/integration/fs/FsCLIIntegrationTest.cpp
        tests/integration/fs/FsModuleIntegrationTest.cpp
        tests/integration/json/JsonCLIIntegrationTest.cpp
        tests/core/SymbolManglerTest.cpp
        tests/hvm/HVMInstructionTest.cpp
        tests/hvm/HOModuleTest.cpp
        tests/hvm/HVMModuleBundleTest.cpp
        tests/archive/HAManifestTest.cpp
        tests/archive/HAArchiveTest.cpp
        tests/archive/HooArchiveLoaderTest.cpp
        tests/hvm/HVMJITLoaderTest.cpp
        tests/hvm/HVMJITInstructionSemanticsTest.cpp
        tests/ast/CompilationUnitTest.cpp
        tests/ast/ASTCoreTest.cpp
        tests/ast/ASTTypeTest.cpp
        tests/ast/ASTExpressionTest.cpp
        tests/ast/ASTStatementTest.cpp
        tests/repl/HooReplTest.cpp
        tests/repl/HooReplAdvancedTest.cpp
        tests/repl/HooReplDebugTest.cpp
        tests/verify/TypeIdConsistencyTest.cpp
        tests/verify/HOModuleBaseSerializationTest.cpp
    )
    set_target_properties(hoo-tests PROPERTIES VERSION ${HOO_VERSION})
    set_target_properties(hoo-tests PROPERTIES ENABLE_EXPORTS ON)
    if(MSVC)
        hoo_use_static_msvc_runtime(hoo-tests)
    endif()

    target_link_libraries(hoo-tests
        hoo-core
        hoorepl
    )
    add_dependencies(hoo-tests hoo)
    if(WIN32)
        target_link_libraries(hoo-tests GTest::gtest GTest::gtest_main)
        target_link_options(hoo-tests PRIVATE
            "/EXPORT:hooc_hvm_sys_alloc"
            "/EXPORT:hooc_hvm_sys_retain"
            "/EXPORT:hooc_hvm_sys_release"
            "/EXPORT:hooc_hvm_sys_refcount"
            "/EXPORT:hooc_hvm_sys_typeid"
            "/EXPORT:hooc_hvm_sys_exception_runtime"
            "/EXPORT:hooc_hvm_sys_push_handler_state"
            "/EXPORT:hooc_hvm_sys_pop_handler_state"
            "/EXPORT:hooc_hvm_sys_throw_to_handler_state"
            "/EXPORT:hooc_hvm_sys_rethrow_to_handler_state"
            "/EXPORT:hooc_hvm_sys_string_data"
            "/EXPORT:hooc_hvm_sys_should_stop_state"
            "/EXPORT:hooc_hvm_sys_thread_create"
            "/EXPORT:hooc_hvm_sys_thread_exit"
            "/EXPORT:hooc_hvm_sys_futex"
            "/EXPORT:hooc_hvm_sys_get_tid"
            "/EXPORT:hooc_hvm_sys_open"
            "/EXPORT:hooc_hvm_sys_read"
            "/EXPORT:hooc_hvm_sys_write"
            "/EXPORT:hooc_hvm_sys_close"
            "/EXPORT:hooc_hvm_sys_lseek"
            "/EXPORT:hooc_hvm_sys_fstat"
            "/EXPORT:hooc_hvm_sys_clock_gettime"
            "/EXPORT:hooc_hvm_sys_getrandom"
            "/EXPORT:hooc_hvm_arc_retain_if_managed"
            "/EXPORT:hooc_hvm_arc_release_if_managed"
            "/EXPORT:hooc_hvm_f8_arith"
        )
    else()
        target_link_libraries(hoo-tests GTest::gtest GTest::gtest_main)
    endif()

    target_compile_definitions(hoo-tests PRIVATE
        HOO_EXECUTABLE="$<TARGET_FILE:hoo>"
    )

    target_include_directories(hoo-tests PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${ANTLR4_INCLUDE_DIR}
        ${ANTLR4_GENERATED_DIR}
    )

    if(WIN32)
        set(HOO_WINDOWS_TEST_COMPAT_HEADER "${CMAKE_BINARY_DIR}/generated/hoo_windows_test_compat.h")
        configure_file(
            "${CMAKE_SOURCE_DIR}/cmake/hooc_windows_test_compat.h.in"
            "${HOO_WINDOWS_TEST_COMPAT_HEADER}"
            COPYONLY
        )
        target_compile_options(hoo-tests PRIVATE /FI${HOO_WINDOWS_TEST_COMPAT_HEADER})

        if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
            set(HOO_WINDOWS_TEST_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        elseif(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin")
            set(HOO_WINDOWS_TEST_PATH "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin")
        else()
            set(HOO_WINDOWS_TEST_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
        endif()
        set_target_properties(hoo-tests PROPERTIES
            VS_DEBUGGER_ENVIRONMENT "PATH=${HOO_WINDOWS_TEST_PATH};%PATH%"
        )
    endif()

    add_test(NAME HooUnitTests COMMAND hoo-tests)

    if(WIN32)
        string(REPLACE ";" "\\;" HOO_WINDOWS_TEST_ENV_PATH "${HOO_WINDOWS_TEST_PATH};$ENV{PATH}")
        set_tests_properties(HooUnitTests PROPERTIES
            ENVIRONMENT "PATH=${HOO_WINDOWS_TEST_ENV_PATH}"
        )
    endif()

    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS hoo-tests
        COMMENT "Running unit tests"
    )

    message(STATUS "GoogleTest found - unit tests enabled")
else()
    message(STATUS "Unit tests disabled. Configure with -DHOO_BUILD_TESTS=ON to enable them.")
endif()

# =============================================================================
# Packaging (CPack)
# =============================================================================

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    set(HOO_VCPKG_BIN_DIR "")
    if(DEFINED VCPKG_ROOT AND EXISTS "${VCPKG_ROOT}/installed/x64-windows/bin")
        set(HOO_VCPKG_BIN_DIR "${VCPKG_ROOT}/installed/x64-windows/bin")
    elseif(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
        set(HOO_VCPKG_BIN_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
        set(HOO_VCPKG_BIN_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
    endif()

    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/install_windows_dlls.cmake.in"
        "${CMAKE_BINARY_DIR}/cmake/install_windows_dlls.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/cmake/install_windows_dlls.cmake")
endif()

set(CPACK_PACKAGE_NAME "Hoo")
set(CPACK_PACKAGE_VERSION "${HOO_VERSION}")
set(CPACK_PACKAGE_VENDOR "Hoo Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hoo Programming Language Compiler")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    find_program(NSIS_EXECUTABLE makensis)
    if(NSIS_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "NSIS")
    endif()
    set(CPACK_PACKAGE_FILE_NAME "hoo-windows-x64")
    set(CPACK_NSIS_DISPLAY_NAME "Hoo Compiler")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

include(CPack)
