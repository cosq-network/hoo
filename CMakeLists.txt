cmake_minimum_required(VERSION 3.16)
project(HoocParser LANGUAGES C CXX)

option(HOOC_BUILD_TESTS "Build Hooc unit tests" OFF)
option(HOOC_BUILD_SHARED_RUNTIME "Build the Hooc runtime as a shared library" OFF)
option(ANTLR4_USE_STATIC_RUNTIME "Define ANTLR4CPP_STATIC when linking a static ANTLR4 C++ runtime" OFF)

set(ANTLR4_ROOT "" CACHE PATH "ANTLR4 C++ runtime installation prefix")
set(ANTLR4_JAR_PATH "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar" CACHE FILEPATH "Path to ANTLR4 JAR file")
set(ANTLR4_GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/Hooc.g4" CACHE FILEPATH "Path to the Hooc ANTLR4 grammar")
set(ANTLR4_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/antlr4" CACHE PATH "Directory for generated ANTLR4 C++ files")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On Windows, force Release runtime to match LLVM libraries
# Use RelWithDebInfo to get debug symbols while using Release runtime
if(WIN32 AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Force Release runtime library on Windows for all configurations
if(MSVC)
    # Use Release runtime (/MD) even in Debug mode to match LLVM
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    # Override default flags for Debug configuration to use /MD instead of /MDd
    string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MTd" "/MT" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")

    # Suppress C4624 warning from LLVM headers (destructor implicitly defined as deleted)
    # This is a false positive in LLVM's intentional design
    add_compile_options(/wd4624)
endif()

# Find ANTLR4 C++ runtime
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

# Set up LLVM
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find the LLVM libraries we need
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

# ANTLR4 generation command
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

# Custom target to generate ANTLR4 files
add_custom_command(
    OUTPUT ${ANTLR4_GENERATED_FILES}
    BYPRODUCTS ${ANTLR4_GENERATED_BYPRODUCTS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4_GENERATED_DIR}
    COMMAND ${Java_JAVA_EXECUTABLE} -jar ${ANTLR4_JAR_PATH}
        -Dlanguage=Cpp
        -visitor
        -listener
        -o ${ANTLR4_GENERATED_DIR}
        -package hooc
        Hooc.g4
    DEPENDS ${ANTLR4_GRAMMAR_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
    COMMENT "Generating ANTLR4 C++ files from Hooc.g4"
    VERBATIM
)

# Create a target for the generated files
add_custom_target(generate_parser
    DEPENDS ${ANTLR4_GENERATED_FILES}
)

# Library for the generated parser
add_library(hoo-parser
    ${ANTLR4_GENERATED_FILES}
)

# Include directories
target_include_directories(hoo-parser PUBLIC
    ${ANTLR4_GENERATED_DIR}
    ${ANTLR4_INCLUDE_DIR}
)

# Link ANTLR4 runtime
target_link_libraries(hoo-parser PUBLIC
    ${ANTLR4_LIBRARY}
)

if(ANTLR4_USE_STATIC_RUNTIME)
    target_compile_definitions(hoo-parser PUBLIC
        ANTLR4CPP_STATIC
    )
endif()

# Ensure generated files are created before building the library
add_dependencies(hoo-parser generate_parser)

# Create hoo-compiler library
add_library(hoo-compiler
    src/HooCompiler.cpp
    src/HoocJIT.cpp
    src/ProcessIsolatedParser.cpp
    src/LLVMCodeGenerator.cpp
    src/SimpleASTBuilder.cpp
    src/ast/ASTImpl.cpp
    src/ModuleSystem.cpp
    src/runtime/RuntimeRegistry.cpp
    src/rt/hoo_string_registration.cpp
    src/rt/hoo_array_registration.cpp
)

if(HOOC_BUILD_SHARED_RUNTIME)
    set(HOOC_RUNTIME_LIBRARY_TYPE SHARED)
else()
    set(HOOC_RUNTIME_LIBRARY_TYPE STATIC)
endif()

# Hooc runtime library (reference counting, memory management, string support, generic array support)
add_library(hoort ${HOOC_RUNTIME_LIBRARY_TYPE}
    src/rt/hoo_runtime.c
    src/rt/hoo_string.cpp
    src/rt/hoo_generic_array.cpp
)

target_include_directories(hoort PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/rt
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(hoort PRIVATE
    $<$<CONFIG:Debug>:HOO_DEBUG_MEMORY>
)

target_link_libraries(hoo-compiler PUBLIC
    hoo-parser
    hoort
    ${llvm_libs}
)

target_include_directories(hoo-compiler PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/rt
    ${LLVM_INCLUDE_DIRS}
    ${ANTLR4_INCLUDE_DIR}
    ${ANTLR4_GENERATED_DIR}
)

# Create main hooc compiler executable
add_executable(hooc
    src/main.cpp
)

target_link_libraries(hooc
    hoo-compiler
)

# Install targets
install(TARGETS hooc hoo-compiler hoo-parser hoort
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

# Print configuration information
message(STATUS "ANTLR4 JAR Path: ${ANTLR4_JAR_PATH}")
message(STATUS "Grammar file: ${ANTLR4_GRAMMAR_FILE}")
message(STATUS "Generated files directory: ${ANTLR4_GENERATED_DIR}")

# Custom target to download ANTLR4 JAR if not present
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

# Clean target for generated files
add_custom_target(clean_generated
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANTLR4_GENERATED_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4_GENERATED_DIR}
    COMMENT "Cleaning generated ANTLR4 files"
)

if(HOOC_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    # Test executable
    add_executable(hoo-tests
        tests/test_main.cpp
        tests/ArrayLiteralParsingTest.cpp
        tests/FunctionCallParsingTest.cpp
        tests/NullableTypeParsingTest.cpp
        tests/VariableDeclarationParseTest.cpp
        tests/IfElseIfParsingTest.cpp
        tests/WhileLoopParsingTest.cpp
        tests/ImportStatementParsingTest.cpp
        tests/BasicCodeGenTest.cpp
        tests/FunctionCallCodeGenTest.cpp
        tests/VariableDeclarationCodeGenTest.cpp
        tests/ArrayLiteralCodeGenTest.cpp
        tests/ArrayGenericRuntimeTest.cpp
        tests/ArrayGenericIntegrationTest.cpp
        tests/IfElseIfCodeGenTest.cpp
        tests/WhileLoopCodeGenTest.cpp
        tests/NullableCodeGenTest.cpp
        tests/ModuleLevelVariableParsingTest.cpp
        tests/ClassDeclarationParsingTest.cpp
        tests/OptionalReturnTypeTest.cpp
        tests/NewExpressionParsingTest.cpp
        tests/NewExpressionCodeGenTest.cpp
        tests/ObjectCreationCodeGenTest.cpp
        tests/MemberAccessParsingTest.cpp
        tests/MemberAccessCodeGenTest.cpp
        tests/MethodCallParsingTest.cpp
        tests/MethodCallCodeGenTest.cpp
        tests/StringBasicsTest.cpp
        tests/StringCodeGenTest.cpp
        tests/ClassArrayCodeGenTest.cpp
        tests/HooArrayPhase7Test.cpp
        tests/QualifiedIdentifierParsingTest.cpp
        tests/QualifiedNewExpressionParsingTest.cpp
        tests/ModuleRegistryTest.cpp
        tests/StdConstructorCodeGenTest.cpp
    )

    target_link_libraries(hoo-tests
        hoo-compiler
        GTest::gtest
        GTest::gtest_main
    )

    # Include directories for tests
    target_include_directories(hoo-tests PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${ANTLR4_INCLUDE_DIR}
        ${ANTLR4_GENERATED_DIR}
    )

    # Add test target
    add_test(NAME HooUnitTests COMMAND hoo-tests)

    # Custom test target for convenience
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS hoo-tests
        COMMENT "Running unit tests"
    )

    message(STATUS "GoogleTest found - unit tests enabled")
else()
    message(STATUS "Unit tests disabled. Configure with -DHOOC_BUILD_TESTS=ON to enable them.")
endif()
