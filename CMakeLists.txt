cmake_minimum_required(VERSION 3.16)
project(HoocParser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ANTLR4 C++ runtime
find_path(ANTLR4_INCLUDE_DIR 
    NAMES antlr4-runtime.h
    PATHS /opt/homebrew/include/antlr4-runtime
          /usr/local/include/antlr4-runtime
          /opt/homebrew/opt/antlr4-cpp-runtime/include/antlr4-runtime
)

find_library(ANTLR4_LIBRARY
    NAMES antlr4-runtime
    PATHS /opt/homebrew/lib
          /usr/local/lib
          /opt/homebrew/opt/antlr4-cpp-runtime/lib
)

if(NOT ANTLR4_INCLUDE_DIR OR NOT ANTLR4_LIBRARY)
    message(FATAL_ERROR "ANTLR4 C++ runtime not found. Please install it with: brew install antlr4-cpp-runtime")
endif()

message(STATUS "ANTLR4 Include Dir: ${ANTLR4_INCLUDE_DIR}")
message(STATUS "ANTLR4 Library: ${ANTLR4_LIBRARY}")

# Find LLVM
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm" ${CMAKE_PREFIX_PATH})
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

# Set ANTLR4 variables
set(ANTLR4_JAR_PATH "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar" CACHE STRING "Path to ANTLR4 JAR file")
set(ANTLR4_GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/Hooc.g4")
set(ANTLR4_GENERATED_DIR "${CMAKE_SOURCE_DIR}/antlr4/generated")

# Create generated directory if it doesn't exist
file(MAKE_DIRECTORY ${ANTLR4_GENERATED_DIR})

# ANTLR4 generation command
set(ANTLR4_GENERATED_FILES
    "${ANTLR4_GENERATED_DIR}/HoocLexer.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocLexer.h"
    "${ANTLR4_GENERATED_DIR}/HoocParser.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocParser.h"
    "${ANTLR4_GENERATED_DIR}/HoocBaseVisitor.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocBaseVisitor.h"
    "${ANTLR4_GENERATED_DIR}/HoocVisitor.h"
    "${ANTLR4_GENERATED_DIR}/HoocBaseListener.cpp"
    "${ANTLR4_GENERATED_DIR}/HoocBaseListener.h"
    "${ANTLR4_GENERATED_DIR}/HoocListener.h"
)

# Custom target to generate ANTLR4 files
add_custom_command(
    OUTPUT ${ANTLR4_GENERATED_FILES}
    COMMAND java -jar ${ANTLR4_JAR_PATH} 
        -Dlanguage=Cpp 
        -visitor 
        -listener 
        -o ${ANTLR4_GENERATED_DIR} 
        -package hooc 
        ${ANTLR4_GRAMMAR_FILE}
    DEPENDS ${ANTLR4_GRAMMAR_FILE}
    COMMENT "Generating ANTLR4 C++ files from Hooc.g4"
    VERBATIM
)

# Create a target for the generated files
add_custom_target(generate_parser
    DEPENDS ${ANTLR4_GENERATED_FILES}
)

# Library for the generated parser
add_library(hooc_parser
    ${ANTLR4_GENERATED_FILES}
)

# Include directories
target_include_directories(hooc_parser PUBLIC
    ${ANTLR4_GENERATED_DIR}
    ${ANTLR4_INCLUDE_DIR}
)

# Link ANTLR4 runtime
target_link_libraries(hooc_parser PUBLIC
    ${ANTLR4_LIBRARY}
)

# Set compile definitions
target_compile_definitions(hooc_parser PUBLIC
    ANTLR4CPP_STATIC
)

# Ensure generated files are created before building the library
add_dependencies(hooc_parser generate_parser)

# Create main hooc compiler executable
add_executable(hooc
    src/main.cpp
    src/HoocJIT.cpp
    src/ProcessIsolatedParser.cpp
    src/CodeGenerator.cpp
    src/SimpleASTBuilder.cpp
    src/ast/ASTImpl.cpp
)

target_link_libraries(hooc
    hooc_parser
    ${llvm_libs}
)

target_include_directories(hooc PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LLVM_INCLUDE_DIRS}
    ${ANTLR4_INCLUDE_DIR}
    ${ANTLR4_GENERATED_DIR}
)

# Install targets
install(TARGETS hooc_parser
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${ANTLR4_GENERATED_DIR}/
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
    add_custom_target(download_antlr4
        COMMAND curl -L -o ${ANTLR4_JAR_PATH} 
            https://www.antlr.org/download/antlr-4.13.1-complete.jar
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