#include "core/HooCLI.h"
#include "core/HooCompiler.h"
#include "core/SymbolMangler.h"
#include "hvm/HOModule.h"
#include "hvm/HVMJIT.h"
#include "runtime/lib/hoo_args.h"
#include "repl/REPLSession.h"

#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace hooc;

namespace fs = std::filesystem;

constexpr const char* COMPILER_NAME = "hoo";
constexpr const char* VERSION = HOO_VERSION;

HooCLI::HooCLI(std::unique_ptr<IOProvider> ioProvider)
    : ioProvider_(std::move(ioProvider)) {}

HooCLI::~HooCLI() = default;

IOProvider* HooCLI::getIOProvider() const {
    return ioProvider_.get();
}

std::string HooCLI::getUsage(std::string_view programName) const {
    std::string usage;
    usage += "Usage: " + std::string(programName) + " [options] [<input_file>]\n";
    usage += "\n";
    usage += "Input File:\n";
    usage += "  <file>.hoo      Source file for HVMJIT execution or AOT compilation\n";
    usage += "  <file>.ho       Bytecode file for direct execution via HVMJIT\n";
    usage += "\n";
    usage += "Options:\n";
    usage += "  -h, --help      Display this help message\n";
    usage += "  -v, --version   Display version information\n";
    usage += "  -o, --output <file>\n";
    usage += "                  Specify output .ho file path (valid only for .hoo, implies compilation-only mode)\n";
    usage += "  --repl          Enter interactive REPL mode\n";
    usage += "  --verbose       Enable verbose logging\n";
    usage += "  --              End hoo options; remaining arguments are passed to the Hoo program\n";
    usage += "\n";
    usage += "Examples:\n";
    usage += "  " + std::string(programName) + " script.hoo           # Compile and run via HVMJIT\n";
    usage += "  " + std::string(programName) + " script.hoo -o out.ho # Build AOT HVM bytecode\n";
    usage += "  " + std::string(programName) + " script.hoo -- arg    # Pass arg to the Hoo program\n";
    usage += "  " + std::string(programName) + " out.ho               # Execute AOT bytecode\n";
    return usage;
}

std::string HooCLI::getVersion() const {
    return std::string(COMPILER_NAME) + " - " + std::string("Hoo") + " v" + VERSION +
           "\n" + "HVM v1.4 Physical" +
           "\n" + "Licensed under the Apache License, Version 2.0\n";
}

HooCLI::Options HooCLI::parseArguments(int argc, char* argv[]) const {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--") {
            for (++i; i < argc; ++i) {
                opts.programArgs.emplace_back(argv[i]);
            }
            break;
        }
        else if (arg.rfind("--output=", 0) == 0) {
            std::string value = std::string(arg.substr(std::string_view("--output=").size()));
            if (value.empty()) {
                opts.hasError = true;
                opts.errorMessage = "Error: --output option requires an output file path\n";
                return opts;
            }
            opts.outputFile = std::move(value);
            opts.compileOnly = true;
        }
        else if (arg.rfind("-o=", 0) == 0) {
            std::string value = std::string(arg.substr(std::string_view("-o=").size()));
            if (value.empty()) {
                opts.hasError = true;
                opts.errorMessage = "Error: -o option requires an output file path\n";
                return opts;
            }
            opts.outputFile = std::move(value);
            opts.compileOnly = true;
        }
        else if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        }
        else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                std::string_view value = argv[i + 1];
                if (value == "--" || (!value.empty() && value[0] == '-')) {
                    opts.hasError = true;
                    opts.errorMessage = "Error: " + std::string(arg) + " option requires an output file path\n";
                    return opts;
                }
                opts.outputFile = argv[++i];
                opts.compileOnly = true;
            } else {
                opts.hasError = true;
                opts.errorMessage = "Error: " + std::string(arg) + " option requires an output file path\n";
                return opts;
            }
        }
        else if (arg == "--verbose") {
            opts.verbose = true;
        }
        else if (arg == "--repl") {
            opts.repl = true;
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
        if (opts.repl) {
            return opts;
        }
        opts.hasError = true;
        opts.errorMessage = "Error: No input file specified\n";
        return opts;
    }

    const std::string& filename = opts.inputFile.value();
    fs::path filePath(filename);
    std::string ext = filePath.extension().string();

    if (ext == ".ho") {
        if (opts.repl) {
            opts.hasError = true;
            opts.errorMessage = "Error: REPL preload input must be a .hoo source file\n";
            return opts;
        }
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
    HVMJIT jit(*ioProvider_);

    std::string moduleName = extractModuleName(filename);
    verboseLog(opts, "Module name: " + moduleName);

    if (opts.isBytecode) {
        verboseLog(opts, "Loading AOT HVM bytecode: " + std::string(filename));
        if (!jit.loadBytecode(std::string(filename))) {
            ioProvider_->writeStderr("Failed to load bytecode: " + jit.getLastError() + "\n");
            return 1;
        }
        
        verboseLog(opts, "Executing main function...");
        MangledFunctionParams mp;
        mp.modulePath = {moduleName};
        mp.functionName = "main";
        mp.returnType = "int64";
        std::string entryPoint = SymbolMangler::mangleFunctionName(mp);
        int64_t result = jit.run(entryPoint);
        if (jit.hasError()) {
            // Fallback: try legacy bare name for interpreter path
            verboseLog(opts, "JIT entry point '" + entryPoint + "' not found, trying legacy '_F_main_v'");
            jit.clearError();
            result = jit.run("_F_main_v");
        }
        if (jit.hasError()) {
            ioProvider_->writeStderr("Execution failed: " + jit.getLastError() + "\n");
            return 1;
        }
        ioProvider_->writeStdout(std::to_string(result) + "\n");
        verboseLog(opts, "Execution completed successfully (result: " + std::to_string(result) + ")");
        return 0;
    }

    verboseLog(opts, "Compiling source code to HVM bytecode...");
    HooCompiler compiler;
    auto hvmModule = compiler.compile(moduleName, std::string(sourceCode));

    if (!hvmModule) {
        ioProvider_->writeStderr("Compilation failed: " + compiler.getLastError() + "\n");
        return 1;
    }

    verboseLog(opts, "HVM Compilation successful");

    if (opts.outputFile.has_value()) {
        std::string outPath = opts.outputFile.value();
        verboseLog(opts, "Saving HVM bytecode to: " + outPath);
        
        std::vector<uint8_t> bytes;
        hvmModule->serialize(bytes);
        if (!ioProvider_->writeBinaryFile(outPath, bytes)) {
            ioProvider_->writeStderr("Error: Failed to save bytecode to " + outPath + "\n");
            return 1;
        }
        ioProvider_->writeStdout("HVM bytecode saved to " + outPath + "\n");
        return 0;
    }

    if (opts.compileOnly) {
        verboseLog(opts, "Compile-only mode: validation successful.");
        return 0;
    }

    verboseLog(opts, "Loading module into HVMJIT...");
    if (!jit.loadModule(std::move(hvmModule))) {
        ioProvider_->writeStderr("Failed to load module: " + jit.getLastError() + "\n");
        return 1;
    }

    verboseLog(opts, "Executing main function...");
    MangledFunctionParams mp;
    mp.modulePath = {moduleName};
    mp.functionName = "main";
    mp.returnType = "int64";
    std::string entryPoint = SymbolMangler::mangleFunctionName(mp);
    int64_t result = jit.run(entryPoint);
    if (jit.hasError()) {
        // Fallback: try legacy bare name for interpreter path
        verboseLog(opts, "JIT entry point '" + entryPoint + "' not found, trying legacy '_F_main_v'");
        jit.clearError();
        result = jit.run("_F_main_v");
    }

    if (jit.hasError()) {
        ioProvider_->writeStderr("Execution failed: " + jit.getLastError() + "\n");
        return 1;
    }

    ioProvider_->writeStdout(std::to_string(result) + "\n");
    verboseLog(opts, "Execution completed successfully (result: " + std::to_string(result) + ")");

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

    std::vector<std::string> runtimeArgStorage;
    runtimeArgStorage.push_back(opts.inputFile.value_or(argv[0]));
    runtimeArgStorage.insert(runtimeArgStorage.end(), opts.programArgs.begin(), opts.programArgs.end());
    std::vector<const char*> runtimeArgv;
    runtimeArgv.reserve(runtimeArgStorage.size());
    for (const std::string& arg : runtimeArgStorage) {
        runtimeArgv.push_back(arg.c_str());
    }
    hoo_args_init(static_cast<int64_t>(runtimeArgv.size()), runtimeArgv.data());
    struct ArgsCleanup { ~ArgsCleanup() { hoo_args_shutdown(); } } argsCleanup;

    if (opts.repl) {
        hooc::repl::REPLSession session;
        if (opts.inputFile.has_value()) {
            const std::string& filename = opts.inputFile.value();
            verboseLog(opts, "Preloading REPL source file: " + filename);
            std::optional<std::string> sourceOpt = ioProvider_->readFile(filename);
            if (!sourceOpt.has_value()) {
                ioProvider_->writeStderr("Error: Cannot open file '" + filename + "'\n");
                return 1;
            }
            if (sourceOpt->empty()) {
                ioProvider_->writeStderr("Error: File is empty\n");
                return 1;
            }
            std::string error;
            if (!session.loadSource(*sourceOpt, error)) {
                ioProvider_->writeStderr("Error: Failed to preload REPL source: " + error + "\n");
                return 1;
            }
            verboseLog(opts, "REPL source preloaded.");
        }
        session.run();
        return 0;
    }

    const std::string& filename = opts.inputFile.value();
    
    if (opts.isBytecode) {
        return compileAndExecute(opts, filename, "");
    }

    verboseLog(opts, "Reading file: " + filename);
    std::optional<std::string> sourceOpt = ioProvider_->readFile(filename);

    if (!sourceOpt.has_value()) {
        ioProvider_->writeStderr("Error: Cannot open file '" + filename + "'\n");
        return 1;
    }

    const std::string& sourceCode = sourceOpt.value();
    if (sourceCode.empty()) {
        ioProvider_->writeStderr("Error: File is empty\n");
        return 1;
    }

    verboseLog(opts, "File loaded (" + std::to_string(sourceCode.size()) + " bytes)");
    verboseLog(opts, "Initializing HVMJIT pipeline...");

    try {
        return compileAndExecute(opts, filename, sourceCode);
    }
    catch (const std::exception& e) {
        ioProvider_->writeStderr("Error: " + std::string(e.what()) + "\n");
        return 1;
    }
}
