cmake_minimum_required(VERSION 3.16)
project(Hooc LANGUAGES C CXX)

option(HOOC_BUILD_TESTS "Build Hooc unit tests" OFF)
option(HOOC_BUILD_SHARED_RUNTIME "Build the Hooc runtime as a shared library" OFF)
option(ANTLR4_USE_STATIC_RUNTIME "Define ANTLR4CPP_STATIC when linking a static ANTLR4 C++ runtime" OFF)

set(ANTLR4_ROOT "" CACHE PATH "ANTLR4 C++ runtime installation prefix")
set(ANTLR4_JAR_PATH "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar" CACHE FILEPATH "Path to ANTLR4 JAR file")
set(ANTLR4_GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/parsing/Hooc.g4" CACHE FILEPATH "Path to the Hooc ANTLR4 grammar")
set(ANTLR4_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/antlr4" CACHE PATH "Directory for generated ANTLR4 C++ files")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MTd" "/MT" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
    add_compile_options(/wd4624)
endif()

# Dependencies
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

message(STATUS "ANTLR4 Include Dir: ${ANTLR4_INCLUDE_DIR}")
message(STATUS "ANTLR4 Library: ${ANTLR4_LIBRARY}")

find_package(LLVM REQUIRED CONFIG)
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
target_include_directories(hoo-parser PUBLIC ${ANTLR4_GENERATED_DIR} ${ANTLR4_INCLUDE_DIR})
target_link_libraries(hoo-parser PUBLIC ${ANTLR4_LIBRARY})

if(ANTLR4_USE_STATIC_RUNTIME)
    target_compile_definitions(hoo-parser PUBLIC ANTLR4CPP_STATIC)
endif()

add_dependencies(hoo-parser generate_parser)

# =============================================================================
# Runtime Library (hoort) - Standalone C/C++ runtime, no LLVM dependency
# =============================================================================

if(HOOC_BUILD_SHARED_RUNTIME)
    set(HOOC_RUNTIME_LIBRARY_TYPE SHARED)
else()
    set(HOOC_RUNTIME_LIBRARY_TYPE STATIC)
endif()

add_library(hoort ${HOOC_RUNTIME_LIBRARY_TYPE}
    src/runtime/lib/hoo_runtime.c
    src/runtime/lib/hoo_string.cpp
    src/runtime/lib/hoo_character.cpp
    src/runtime/lib/hoo_generic_array.cpp
    src/runtime/lib/hoo_map.cpp
    src/runtime/lib/hoo_io.cpp
    src/runtime/lib/hoo_exception.cpp
    src/runtime/lib/hoo_math.cpp
    src/runtime/lib/hoo_net.cpp
)

target_include_directories(hoort PUBLIC
    ${CMAKE_SOURCE_DIR}/src/runtime/lib
)

target_compile_definitions(hoort PRIVATE
    $<$<CONFIG:Debug>:HOO_DEBUG_MEMORY>
)

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
    # Module system
    src/modules/ModuleSystem.cpp
    # Parsing (manual wrapper)
    src/parsing/HooParserWrapper.cpp
)

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
)

# Generated ANTLR4 parser files must exist before hoo-core sources can include them
add_dependencies(hoo-core generate_parser)

# =============================================================================
# CLI Executable
# =============================================================================

add_executable(hoo src/core/main.cpp)
target_link_libraries(hoo hoo-core)
target_include_directories(hoo PRIVATE ${CMAKE_SOURCE_DIR}/src)

# =============================================================================
# Install
# =============================================================================

install(TARGETS hoo hoo-core hoo-parser hoort
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${ANTLR4_GENERATED_DIR}/
    DESTINATION include/hooc
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY src/
    DESTINATION include/hooc
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

if(HOOC_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(hoo-tests
        tests/test_main.cpp
        tests/parsing/ArrayLiteralParsingTest.cpp
        tests/parsing/FunctionCallParsingTest.cpp
        tests/parsing/NullableTypeParsingTest.cpp
        tests/parsing/VariableDeclarationParseTest.cpp
        tests/parsing/IfElseIfParsingTest.cpp
        tests/parsing/WhileLoopParsingTest.cpp
        tests/parsing/ImportStatementParsingTest.cpp
        tests/parsing/ModuleLevelVariableParsingTest.cpp
        tests/parsing/ClassDeclarationParsingTest.cpp
        tests/parsing/SimpleASTBuilderTest.cpp
        tests/parsing/MapTypeParsingTest.cpp
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
        tests/codegen/ModuleRegistryTest.cpp
        tests/runtime/HooArrayPhase7Test.cpp
        tests/runtime/StringBasicsTest.cpp
        tests/runtime/HooCharacterTest.cpp
        tests/runtime/HooRuntimeTest.cpp
        tests/runtime/HooMapTest.cpp
        tests/runtime/HooExceptionTest.cpp
        tests/runtime/HooMathTest.cpp
        tests/runtime/HooIOTest.cpp
        tests/runtime/HooNetTest.cpp
        tests/jit/IntegerTypesTest.cpp
        tests/jit/NewLanguageFeaturesTest.cpp
        tests/jit/CharacterLiteralTest.cpp
        tests/jit/FloatingLiteralTest.cpp
        tests/jit/InterpolatedStringTest.cpp
        tests/core/HooCompilerTest.cpp
        tests/core/HooCLITest.cpp
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
    )

    target_link_libraries(hoo-tests
        hoo-core
        GTest::gtest
        GTest::gtest_main
    )

    target_include_directories(hoo-tests PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${ANTLR4_INCLUDE_DIR}
        ${ANTLR4_GENERATED_DIR}
    )

    add_test(NAME HooUnitTests COMMAND hoo-tests)

    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS hoo-tests
        COMMENT "Running unit tests"
    )

    message(STATUS "GoogleTest found - unit tests enabled")
else()
    message(STATUS "Unit tests disabled. Configure with -DHOOC_BUILD_TESTS=ON to enable them.")
endif()
