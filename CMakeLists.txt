cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(Hoo LANGUAGES C CXX)
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

    function(hoo_use_static_msvc_runtime target_name)
        set_property(TARGET ${target_name} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endfunction()
endif()

# Dependencies
if(WIN32 AND NOT ANTLR4_ROOT)
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
    find_path(ANTLR4_INCLUDE_DIR
        NAMES antlr4-runtime.h
        HINTS
            "${ANTLR4_ROOT}/include/antlr4-runtime"
            "$ENV{ANTLR4_ROOT}/include/antlr4-runtime"
        PATH_SUFFIXES
            include/antlr4-runtime
            antlr4-runtime
    )

    find_library(ANTLR4_LIBRARY
        NAMES antlr4-runtime
        HINTS
            "${ANTLR4_ROOT}"
            "$ENV{ANTLR4_ROOT}"
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

message(STATUS "ANTLR4 Include Dir: ${ANTLR4_INCLUDE_DIR}")
message(STATUS "ANTLR4 Library: ${ANTLR4_LIBRARY}")

find_package(ZLIB REQUIRED)

if(NOT APPLE)
    find_package(OpenSSL REQUIRED)
endif()

find_package(CURL REQUIRED)

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

add_library(hoort ${HOO_RUNTIME_LIBRARY_TYPE}
    src/runtime/lib/hoo_runtime.c
    src/runtime/lib/hoo_any.cpp
    src/runtime/lib/hoo_anyarray.cpp
    src/runtime/lib/hoo_hashmap.cpp
    src/runtime/lib/hoo_string.cpp
    src/runtime/lib/hoo_character.cpp
    src/runtime/lib/hoo_generic_array.cpp
    src/runtime/lib/hoo_tensor.cpp
    src/runtime/lib/hoo_map.cpp
    src/runtime/lib/hoo_io.cpp
    src/runtime/lib/hoo_exception.cpp
    src/runtime/lib/hoo_math.cpp
    src/runtime/lib/hoo_net.cpp
    src/runtime/lib/hoo_fs.cpp
    src/runtime/lib/hoo_datetime.cpp
    src/runtime/lib/hoo_encoding.cpp
    src/runtime/lib/hoo_system.cpp
    src/runtime/lib/hoo_regex.cpp
    src/runtime/lib/hoo_csv.cpp
    src/runtime/lib/hoo_uuid.cpp
    src/runtime/lib/hoo_hashing.cpp
    src/runtime/lib/hoo_process.cpp
    src/runtime/lib/hoo_compression.cpp
    src/runtime/lib/hoo_args.cpp
    src/runtime/lib/hoo_thread.cpp
    src/runtime/lib/hoo_json.cpp
    src/runtime/lib/hoo_buffer.cpp
)
if(MSVC)
    hoo_use_static_msvc_runtime(hoort)
endif()

target_include_directories(hoort PUBLIC
    ${CMAKE_SOURCE_DIR}/src/runtime/lib
)

target_compile_definitions(hoort PRIVATE
    $<$<CONFIG:Debug>:HOO_DEBUG_MEMORY>
)

if(NOT APPLE)
    target_link_libraries(hoort PUBLIC OpenSSL::Crypto)
endif()
target_link_libraries(hoort PUBLIC ZLIB::ZLIB)

set_source_files_properties(
    src/runtime/lib/hoo_runtime.c
    PROPERTIES
        LANGUAGE C
)

if(MSVC)
    set_source_files_properties(
        src/runtime/lib/hoo_runtime.c
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

target_link_libraries(hoo-core PUBLIC
    hoort
    hoo-parser
    ${llvm_libs}
    CURL::libcurl
)

if(WIN32)
    target_link_libraries(hoo-core PUBLIC
        Psapi
        Dbghelp
    )
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
        tests/parsing/TensorTypeParsingTest.cpp
        tests/parsing/VariableDeclarationParseTest.cpp
        tests/parsing/IfElseIfParsingTest.cpp
        tests/parsing/WhileLoopParsingTest.cpp
        tests/parsing/ImportStatementParsingTest.cpp
        tests/parsing/ModuleLevelVariableParsingTest.cpp
        tests/parsing/ClassDeclarationParsingTest.cpp
        tests/parsing/SimpleASTBuilderTest.cpp
        tests/parsing/MapTypeParsingTest.cpp
        tests/parsing/AnyHashMapParsingTest.cpp
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
        tests/runtime/HooPathTest.cpp
        tests/runtime/HooHashingTest.cpp
        tests/runtime/HooProcessTest.cpp
        tests/runtime/HooCompressionTest.cpp
        tests/runtime/HooArgsTest.cpp
        tests/runtime/HooBufferTest.cpp
        tests/runtime/HooNetTest.cpp
        tests/runtime/HooThreadTest.cpp
        tests/jit/HooMathJitTest.cpp
        tests/jit/HooStringJitTest.cpp
        tests/jit/HooMapJitTest.cpp
        tests/jit/HooAnyCollectionsJitTest.cpp
        tests/jit/HooArrayJitTest.cpp
        tests/jit/HooTensorJitTest.cpp
        tests/jit/HooStandardLibraryJitTest.cpp
        tests/jit/HooThreadJitTest.cpp
        tests/jit/HooCsvJitTest.cpp
        tests/jit/HooDatetimeJitTest.cpp
        tests/jit/HooPathJitTest.cpp
        tests/jit/HooHashingJitTest.cpp
        tests/jit/HooEncodingJitTest.cpp
        tests/jit/HooProcessJitTest.cpp
        tests/jit/HooCompressionJitTest.cpp
        tests/jit/HooClassApiTest.cpp
        tests/jit/HooArgsJitTest.cpp
        tests/jit/HooNetJitTest.cpp
        tests/jit/HooBufferJitTest.cpp
        tests/jit/HooJsonJitTest.cpp
        tests/jit/IntegerTypesTest.cpp
        tests/jit/NewLanguageFeaturesTest.cpp
        tests/jit/CharacterLiteralTest.cpp
        tests/jit/HooCharacterApiJitTest.cpp
        tests/jit/FloatingLiteralTest.cpp
        tests/jit/LowPrecisionTypesTest.cpp
        tests/jit/InterpolatedStringTest.cpp
        tests/jit/HVMJITLifecycleTest.cpp
        tests/core/HooCompilerTest.cpp
        tests/core/HooCLITest.cpp
        tests/core/HooCLIIntegrationTest.cpp
        tests/core/SymbolManglerTest.cpp
        tests/hvm/HVMInstructionTest.cpp
        tests/hvm/HOModuleTest.cpp
        tests/hvm/HVMModuleBundleTest.cpp
        tests/hvm/HVMJITLoaderTest.cpp
        tests/hvm/HVMJITInstructionSemanticsTest.cpp
        tests/ast/CompilationUnitTest.cpp
        tests/ast/ASTCoreTest.cpp
        tests/ast/ASTTypeTest.cpp
        tests/ast/ASTExpressionTest.cpp
        tests/ast/ASTStatementTest.cpp
        tests/repl/HooReplTest.cpp
    )
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
set(CPACK_PACKAGE_VERSION "0.1.0")
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
