#include "core/HooCLI.h"
#include "jit/HoocJIT.h"
#include "core/HooCompiler.h"
#include "hvm/HoModule.h"

#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace hooc;

namespace fs = std::filesystem;

constexpr const char* COMPILER_NAME = "hooc";
constexpr const char* VERSION = "1.1.0";

HooCLI::HooCLI(std::unique_ptr<IOProvider> ioProvider)
    : ioProvider_(std::move(ioProvider)) {}

HooCLI::~HooCLI() = default;

IOProvider* HooCLI::getIOProvider() const {
    return ioProvider_.get();
}

std::string HooCLI::getUsage(std::string_view programName) const {
    std::string usage;
    usage += "Usage: " + std::string(programName) + " [options] <input_file>\n";
    usage += "\n";
    usage += "Input File:\n";
    usage += "  <file>.hoo      Source file for JIT execution or compilation\n";
    usage += "  <file>.ho       Bytecode file for AOT JIT execution (reserved for future)\n";
    usage += "\n";
    usage += "Options:\n";
    usage += "  -h, --help      Display this help message\n";
    usage += "  -v, --version   Display version information\n";
    usage += "  -c, --compile   Compile only, do not execute (valid only for .hoo)\n";
    usage += "  -o, --output    Specify output .ho file path (valid only for .hoo, implies -c)\n";
    usage += "  --backend <val> Specify backend: 'llvm' (default) or 'hvm'\n";
    usage += "  --verbose       Enable verbose logging\n";
    usage += "  --print-ir      Print generated LLVM IR\n";
    usage += "\n";
    usage += "Examples:\n";
    usage += "  " + std::string(programName) + " script.hoo           # Compile and run via JIT\n";
    usage += "  " + std::string(programName) + " script.hoo -c        # Compile and validate source\n";
    usage += "  " + std::string(programName) + " script.hoo -o out.ho # Build AOT bytecode (reserved)\n";
    usage += "  " + std::string(programName) + " out.ho               # Run AOT bytecode (reserved)\n";
    return usage;
}

std::string HooCLI::getVersion() const {
    return std::string(COMPILER_NAME) + " version " + VERSION + "\n";
}

HooCLI::Options HooCLI::parseArguments(int argc, char* argv[]) const {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        }
        else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
        }
        else if (arg == "-c" || arg == "--compile") {
            opts.compileOnly = true;
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                opts.outputFile = argv[++i];
                opts.compileOnly = true;
            } else {
                opts.hasError = true;
                opts.errorMessage = "Error: -o option requires an output file path\n";
                return opts;
            }
        }
        else if (arg == "--backend") {
            if (i + 1 < argc) {
                std::string_view backendVal = argv[++i];
                if (backendVal == "llvm") {
                    opts.backend = Options::Backend::LLVM;
                } else if (backendVal == "hvm") {
                    opts.backend = Options::Backend::HVM;
                } else {
                    opts.hasError = true;
                    opts.errorMessage = "Error: Invalid backend '" + std::string(backendVal) + "'. Expected 'llvm' or 'hvm'\n";
                    return opts;
                }
            } else {
                opts.hasError = true;
                opts.errorMessage = "Error: --backend option requires a value ('llvm' or 'hvm')\n";
                return opts;
            }
        }
        else if (arg == "--verbose") {
            opts.verbose = true;
        }
        else if (arg == "--print-ir") {
            opts.printIR = true;
        }
        else if (arg.rfind("--", 0) == 0 || (arg.size() > 1 && arg[0] == '-')) {
            opts.hasError = true;
            opts.errorMessage = "Error: Unknown option '" + std::string(arg) + "'\n";
            return opts;
        }
        else if (!opts.inputFile.has_value()) {
            opts.inputFile = std::string(arg);
        }
        else {
            opts.hasError = true;
            opts.errorMessage = "Error: Multiple input files specified. Only one .hoo or .ho file is allowed.\n";
            return opts;
        }
    }

    if (opts.showHelp || opts.showVersion) {
        return opts;
    }

    if (!opts.inputFile.has_value()) {
        opts.hasError = true;
        opts.errorMessage = "Error: No input file specified\n";
        return opts;
    }

    const std::string& filename = opts.inputFile.value();
    fs::path filePath(filename);
    std::string ext = filePath.extension().string();

    if (ext == ".ho") {
        opts.isBytecode = true;
        if (opts.compileOnly) {
            opts.hasError = true;
            opts.errorMessage = "Error: Cannot use compilation flags (-c, -o) with bytecode file (.ho)\n";
            return opts;
        }
    } else if (ext == ".hoo") {
        opts.isBytecode = false;
    } else {
        opts.hasError = true;
        opts.errorMessage = "Error: Invalid file extension for '" + filename + "'. Expected .hoo or .ho\n";
        return opts;
    }

    return opts;
}

void HooCLI::verboseLog(const Options& opts, std::string_view message) const {
    if (opts.verbose) {
        ioProvider_->writeStderr("[VERBOSE] " + std::string(message) + "\n");
    }
}

std::string HooCLI::extractModuleName(std::string_view filename) const {
    fs::path p(filename);
    return p.stem().string();
}

int HooCLI::compileAndExecute(const Options& opts,
                              std::string_view filename,
                              std::string_view sourceCode) const {
    HoocJIT jit;

    std::string moduleName = extractModuleName(filename);
    verboseLog(opts, "Module name: " + moduleName);

    if (opts.isBytecode) {
        verboseLog(opts, "AOT JIT execution requested for: " + std::string(filename));
        ioProvider_->writeStderr("Info: AOT JIT execution (.ho files) will be implemented in a future update.\n");
        return 0;
    }

    if (opts.backend == Options::Backend::HVM) {
        verboseLog(opts, "Compiling source code to HVM bytecode...");
        HooCompiler compiler;
        auto hvmModule = compiler.compileToHVM(moduleName, std::string(sourceCode));

        if (!hvmModule) {
            ioProvider_->writeStderr("HVM Compilation failed: " + compiler.getLastError() + "\n");
            return 1;
        }

        verboseLog(opts, "HVM Compilation successful");

        if (opts.outputFile.has_value()) {
            std::string outPath = opts.outputFile.value();
            verboseLog(opts, "Saving HVM bytecode to: " + outPath);
            if (!hvmModule->serializeToFile(outPath)) {
                ioProvider_->writeStderr("Error: Failed to save bytecode to " + outPath + ": " + hvmModule->getError() + "\n");
                return 1;
            }
            ioProvider_->writeStdout("HVM bytecode saved to " + outPath + "\n");
        } else if (opts.compileOnly) {
            verboseLog(opts, "HVM Compile-only mode: validation successful.");
        } else {
            ioProvider_->writeStderr("Info: HVM runtime execution is not yet integrated into hooc. Use 'ho' tool to run .ho files.\n");
        }
        return 0;
    }

    verboseLog(opts, "Compiling source code...");
    auto result = jit.compile(moduleName, std::string(sourceCode));

    if (!result.success) {
        ioProvider_->writeStderr("Compilation failed: " + result.error + "\n");
        return 1;
    }

    verboseLog(opts, "Compilation successful");

    if (opts.printIR && !result.ir.empty()) {
        ioProvider_->writeStdout(result.ir + "\n");
    }

    if (opts.compileOnly) {
        if (opts.outputFile.has_value()) {
            verboseLog(opts, "Output file requested: " + opts.outputFile.value());
            ioProvider_->writeStderr("Info: AOT build and output emission (-o) will be implemented in a future update.\n");
        } else {
            verboseLog(opts, "Compile-only mode: validation successful.");
        }
        return 0;
    }

    verboseLog(opts, "Executing main function...");

    auto execResult = jit.execute("main");

    if (!execResult) {
        ioProvider_->writeStderr("Execution failed: " + execResult->error + "\n");
        return 1;
    }

    verboseLog(opts, "Execution completed successfully");

    return 0;
}

int HooCLI::run(int argc, char* argv[]) {
    Options opts = parseArguments(argc, argv);

    if (opts.hasError) {
        ioProvider_->writeStderr(opts.errorMessage);
        if (!opts.showHelp && !opts.showVersion) {
            ioProvider_->writeStderr("\n" + getUsage(argv[0]));
        }
        return 1;
    }

    if (opts.showHelp) {
        ioProvider_->writeStdout(getUsage(argv[0]));
        return 0;
    }

    if (opts.showVersion) {
        ioProvider_->writeStdout(getVersion());
        return 0;
    }

    const std::string& filename = opts.inputFile.value();
    verboseLog(opts, "Reading file: " + filename);

    std::optional<std::string> sourceOpt = ioProvider_->readFile(filename);

    if (!sourceOpt.has_value()) {
        ioProvider_->writeStderr("Error: Cannot open file '" + filename + "'\n");
        return 1;
    }

    const std::string& sourceCode = sourceOpt.value();

    if (!opts.isBytecode && sourceCode.empty()) {
        ioProvider_->writeStderr("Error: File is empty\n");
        return 1;
    }

    verboseLog(opts, "File loaded (" + std::to_string(sourceCode.size()) + " bytes)");

    if (!opts.isBytecode) {
        verboseLog(opts, "Initializing JIT compiler for source...");
    } else {
        verboseLog(opts, "Initializing JIT executor for bytecode...");
    }

    try {
        return compileAndExecute(opts, filename, sourceCode);
    }
    catch (const std::exception& e) {
        ioProvider_->writeStderr("Error: " + std::string(e.what()) + "\n");
        return 1;
    }
}
