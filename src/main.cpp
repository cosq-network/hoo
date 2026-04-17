#include <iostream>
#include <fstream>
#include <string>
#include <optional>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

#include "HoocJIT.h"

using namespace llvm;
using namespace hooc;

// ============================================================================
// Constants
// ============================================================================

constexpr const char* COMPILER_NAME = "hooc";
constexpr const char* VERSION = "1.0.0";

// ============================================================================
// Command-Line Options
// ============================================================================

struct Options {
    bool verbose      = false;
    bool showHelp    = false;
    bool showVersion = false;
    bool printIR     = false;
    std::optional<std::string> inputFile;
};

// ============================================================================
// Usage and Version Information
// ============================================================================

void printUsage(const char* programName, std::ostream& out) {
    out << "Usage: " << programName << " [options] <source_file>\n"
        << "\n"
        << "Options:\n"
        << "  -h, --help      Display this help message\n"
        << "  -v, --version   Display version information\n"
        << "  --verbose       Enable verbose logging\n"
        << "  --print-ir      Print generated LLVM IR\n"
        << "\n"
        << "Examples:\n"
        << "  " << programName << " hello.hoo\n"
        << "  " << programName << " --verbose --print-ir hello.hoo\n";
}

void printVersion(std::ostream& out) {
    out << COMPILER_NAME << " version " << VERSION << "\n";
}

// ============================================================================
// Argument Parsing
// ============================================================================

Options parseArguments(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        }
        else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
        }
        else if (arg == "--verbose") {
            opts.verbose = true;
        }
        else if (arg == "--print-ir") {
            opts.printIR = true;
        }
        else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            std::exit(1);
        }
        else if (!opts.inputFile.has_value()) {
            opts.inputFile = arg;
        }
        else {
            std::cerr << "Error: Unexpected argument '" << arg << "'\n";
            std::exit(1);
        }
    }

    return opts;
}

// ============================================================================
// File Operations
// ============================================================================

std::optional<std::string> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

std::string extractModuleName(const std::string& filename) {
    std::string moduleName = filename;

    size_t lastSlash = moduleName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        moduleName = moduleName.substr(lastSlash + 1);
    }

    constexpr const char* SOURCE_EXTENSION = ".hoo";
    size_t extPos = moduleName.rfind(SOURCE_EXTENSION);
    if (extPos != std::string::npos && extPos + 4 == moduleName.size()) {
        moduleName = moduleName.substr(0, extPos);
    }

    return moduleName;
}

// ============================================================================
// Verbose Logging
// ============================================================================

void verboseLog(const Options& opts, const std::string& message) {
    if (opts.verbose) {
        std::cerr << "[VERBOSE] " << message << "\n";
    }
}

// ============================================================================
// Compilation Pipeline
// ============================================================================

int compileAndExecute(const Options& opts,
                      const std::string& filename,
                      const std::string& sourceCode) {
    HoocJIT jit;

    std::string moduleName = extractModuleName(filename);
    verboseLog(opts, "Module name: " + moduleName);
    verboseLog(opts, "Compiling source code...");

    auto result = jit.compile(moduleName, sourceCode);

    if (!result.success) {
        std::cerr << "Compilation failed: " << result.error << "\n";
        return 1;
    }

    verboseLog(opts, "Compilation successful");

    if (opts.printIR && !result.ir.empty()) {
        std::cout << result.ir << "\n";
    }

    verboseLog(opts, "Executing main function...");

    auto execResult = jit.execute("main");

    if (!execResult) {
        std::cerr << "Execution failed: " << execResult->error << "\n";
        return 1;
    }

    verboseLog(opts, "Execution completed successfully");

    return 0;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    InitLLVM llvm(argc, argv);

    Options opts = parseArguments(argc, argv);

    if (opts.showHelp) {
        printUsage(argv[0], std::cout);
        return 0;
    }

    if (opts.showVersion) {
        printVersion(std::cout);
        return 0;
    }

    if (!opts.inputFile.has_value()) {
        std::cerr << "Error: No input file specified\n\n";
        printUsage(argv[0], std::cerr);
        return 1;
    }

    const std::string& filename = opts.inputFile.value();

    verboseLog(opts, "Reading file: " + filename);

    std::optional<std::string> sourceOpt = readFile(filename);

    if (!sourceOpt.has_value()) {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return 1;
    }

    const std::string& sourceCode = sourceOpt.value();

    if (sourceCode.empty()) {
        std::cerr << "Error: File is empty\n";
        return 1;
    }

    verboseLog(opts, "File loaded (" + std::to_string(sourceCode.size()) + " bytes)");
    verboseLog(opts, "Initializing JIT compiler...");

    try {
        return compileAndExecute(opts, filename, sourceCode);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
