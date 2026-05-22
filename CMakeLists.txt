cmake_minimum_required(VERSION 3.16)
project(HoocParser LANGUAGES C CXX)

option(HOOC_BUILD_TESTS "Build Hooc unit tests" OFF)
option(HOOC_BUILD_SHARED_RUNTIME "Build the Hooc runtime as a shared library" OFF)
option(ANTLR4_USE_STATIC_RUNTIME "Define ANTLR4CPP_STATIC when linking a static ANTLR4 C++ runtime" OFF)

set(ANTLR4_ROOT "" CACHE PATH "ANTLR4 C++ runtime installation prefix")
set(ANTLR4_JAR_PATH "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar" CACHE FILEPATH "Path to ANTLR4 JAR file")
set(ANTLR4_GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/parsing/Hooc.g4" CACHE FILEPATH "Path to the Hooc ANTLR4 grammar")
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
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/parsing
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

# HVM (Hooc Virtual Machine) library - object file format and module management
add_library(hvm STATIC
    src/hvm/HoModuleBase.cpp
    src/hvm/HoModule.cpp
    src/hvm/HInstruction.cpp
    src/hvm/ModuleBundle.cpp
)

target_include_directories(hvm PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/hvm
)

target_link_libraries(hvm PUBLIC
    ${llvm_libs}
)

# Ensure generated files are created before building the library
add_dependencies(hoo-parser generate_parser)

# Create hoo-compiler library
add_library(hoo-compiler
    src/core/HooCompiler.cpp
    src/core/SymbolMangler.cpp
    src/core/DefaultIOProvider.cpp
    src/core/HooCLI.cpp
    src/jit/HoocJIT.cpp
    src/parsing/ProcessIsolatedParser.cpp
    src/codegen/LLVMCodeGenerator.cpp
    src/codegen/HVMCodeGenerator.cpp
    src/ast/SimpleASTBuilder.cpp
    src/ast/ASTImpl.cpp
    src/modules/ModuleSystem.cpp
    src/runtime/llvm/RuntimeRegistry.cpp
    src/runtime/llvm/RuntimeMethodRegistry.cpp
    src/runtime/llvm/hoo_string_registration.cpp
    src/runtime/llvm/hoo_array_registration.cpp
    src/runtime/llvm/hoo_io_registration.cpp
    src/runtime/llvm/hoo_map_registration.cpp
    src/runtime/llvm/RuntimeStringMethodsRegistration.cpp
)

target_link_libraries(hoo-compiler PUBLIC
    hvm
    hoo-parser
    hoort
    ${llvm_libs}
)

if(HOOC_BUILD_SHARED_RUNTIME)
    set(HOOC_RUNTIME_LIBRARY_TYPE SHARED)
else()
    set(HOOC_RUNTIME_LIBRARY_TYPE STATIC)
endif()

# Hooc runtime library (reference counting, memory management, string support, generic array support, I/O, exceptions)
add_library(hoort ${HOOC_RUNTIME_LIBRARY_TYPE}
    src/runtime/lib/hoo_runtime.c
    src/runtime/lib/hoo_string.cpp
    src/runtime/lib/hoo_generic_array.cpp
    src/runtime/lib/hoo_map.cpp
    src/runtime/lib/hoo_io.cpp
    src/runtime/lib/hoo_exception.cpp
    src/runtime/lib/hoo_math.cpp
    src/runtime/lib/hoo_net.cpp
    src/runtime/llvm/hoo_io_registration.cpp
    src/runtime/llvm/hoo_map_registration.cpp
    src/runtime/llvm/hoo_math_registration.cpp
    src/runtime/llvm/hoo_net_registration.cpp
)

target_include_directories(hoort PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/runtime/lib
    ${CMAKE_SOURCE_DIR}/src/runtime/llvm
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
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/hvm
    ${CMAKE_SOURCE_DIR}/src/jit
    ${CMAKE_SOURCE_DIR}/src/parsing
    ${CMAKE_SOURCE_DIR}/src/codegen
    ${CMAKE_SOURCE_DIR}/src/modules
    ${CMAKE_SOURCE_DIR}/src/ast
    ${CMAKE_SOURCE_DIR}/src/runtime/lib
    ${CMAKE_SOURCE_DIR}/src/runtime/llvm
    ${LLVM_INCLUDE_DIRS}
    ${ANTLR4_INCLUDE_DIR}
    ${ANTLR4_GENERATED_DIR}
)

# Create main hooc compiler executable
add_executable(hooc
    src/core/main.cpp
)

target_link_libraries(hooc
    hoo-compiler
)

target_include_directories(hooc PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

# Create ho compiler library
add_library(ho-lib
    src/ho/HoCLI.cpp
)

target_include_directories(ho-lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ho
)

# Create ho compiler executable
add_executable(ho
    src/ho/main.cpp
)

target_link_libraries(ho
    ho-lib
)

target_include_directories(ho PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ho
)

# Install targets
install(TARGETS hooc ho hoo-compiler hoo-parser hoort
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
        tests/parsing/ArrowFunctionSyntaxTest.cpp
        tests/parsing/MathModuleParsingTest.cpp
        tests/parsing/NetworkModuleParsingTest.cpp
        tests/parsing/NewExpressionParsingTest.cpp
        tests/parsing/MemberAccessParsingTest.cpp
        tests/parsing/MethodCallParsingTest.cpp
        tests/parsing/QualifiedIdentifierParsingTest.cpp
        tests/parsing/QualifiedNewExpressionParsingTest.cpp
        tests/parsing/ThisKeywordParsingTest.cpp
        tests/parsing/FfiAndGrammarAstTest.cpp
        tests/codegen/BasicCodeGenTest.cpp
        tests/codegen/FunctionCallCodeGenTest.cpp
        tests/codegen/VariableDeclarationCodeGenTest.cpp
        tests/codegen/ArrayLiteralCodeGenTest.cpp
        tests/codegen/IfElseIfCodeGenTest.cpp
        tests/codegen/WhileLoopCodeGenTest.cpp
        tests/codegen/ThisKeywordCodeGenTest.cpp
        tests/codegen/NullableCodeGenTest.cpp
        tests/codegen/NewExpressionCodeGenTest.cpp
        tests/codegen/ObjectCreationCodeGenTest.cpp
        tests/codegen/MemberAccessCodeGenTest.cpp
        tests/codegen/MethodCallCodeGenTest.cpp
        tests/codegen/StringCodeGenTest.cpp
        tests/codegen/ClassArrayCodeGenTest.cpp
        tests/codegen/MapCodeGenTest.cpp
        tests/codegen/ArrowFunctionSyntaxCodeGenTest.cpp
        tests/codegen/MathModuleCodeGenTest.cpp
        tests/codegen/NetworkModuleCodeGenTest.cpp
        tests/codegen/StdConstructorCodeGenTest.cpp
        tests/codegen/QualifiedIdentifierCodeGenTest.cpp
        tests/codegen/GlobalVariableCodeGenTest.cpp
        tests/codegen/ConstantCodeGenTest.cpp
        tests/codegen/ImportStatementCodeGenTest.cpp
        tests/codegen/ModuleRegistryTest.cpp
        tests/codegen/LLVMCodeGeneratorComprehensiveTest.cpp
        tests/codegen/HVMCodeGeneratorTest.cpp
        tests/codegen/HVMCodeGeneratorComprehensiveTest.cpp
        tests/runtime/ArrayGenericRuntimeTest.cpp
        tests/runtime/HooArrayPhase7Test.cpp
        tests/runtime/StringBasicsTest.cpp
        tests/runtime/HooRuntimeTest.cpp
        tests/runtime/HooMapTest.cpp
        tests/runtime/HooExceptionTest.cpp
        tests/runtime/HooMathTest.cpp
        tests/runtime/HooIOTest.cpp
        tests/runtime/HooNetTest.cpp
        tests/integration/ArrayGenericIntegrationTest.cpp
        tests/jit/HoocJITTest.cpp
        tests/jit/IOFunctionTest.cpp
        tests/jit/IntegerTypesTest.cpp
        tests/jit/NewLanguageFeaturesTest.cpp
        tests/core/HooCompilerTest.cpp
        tests/core/HooCLITest.cpp
        tests/core/SymbolManglerTest.cpp
        tests/ho/HoCLITest.cpp
        tests/hvm/HInstructionTest.cpp
        tests/hvm/HoModuleTest.cpp
        tests/hvm/ModuleBundleTest.cpp
        tests/ast/CompilationUnitTest.cpp
    )

    target_link_libraries(hoo-tests
        hvm
        hoo-compiler
        ho-lib
        GTest::gtest
        GTest::gtest_main
    )

    # Include directories for tests
    target_include_directories(hoo-tests PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/ho
        ${CMAKE_SOURCE_DIR}/src/hvm
        ${CMAKE_SOURCE_DIR}/src/jit
        ${CMAKE_SOURCE_DIR}/src/parsing
        ${CMAKE_SOURCE_DIR}/src/codegen
        ${CMAKE_SOURCE_DIR}/src/modules
        ${CMAKE_SOURCE_DIR}/src/ast
        ${CMAKE_SOURCE_DIR}/tests/ast
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
