#include "core/HooCLI.h"
#include "core/HooCompiler.h"
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
constexpr const char* VERSION = "1.4.0"; // Aligned with HVM 1.4

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
    usage += "  <file>.hoo      Source file for HVMJIT execution or AOT compilation\n";
    usage += "  <file>.ho       Bytecode file for direct execution via HVMJIT\n";
    usage += "\n";
    usage += "Options:\n";
    usage += "  -h, --help      Display this help message\n";
    usage += "  -v, --version   Display version information\n";
    usage += "  -c, --compile   Compile only, do not execute (valid only for .hoo)\n";
    usage += "  -o, --output    Specify output .ho file path (valid only for .hoo, implies -c)\n";
    usage += "  --repl          Enter interactive REPL mode\n";
    usage += "  --verbose       Enable verbose logging\n";
    usage += "\n";
    usage += "Examples:\n";
    usage += "  " + std::string(programName) + " script.hoo           # Compile and run via HVMJIT\n";
    usage += "  " + std::string(programName) + " script.hoo -c        # Compile and validate source\n";
    usage += "  " + std::string(programName) + " script.hoo -o out.ho # Build AOT HVM bytecode\n";
    usage += "  " + std::string(programName) + " out.ho               # Execute AOT bytecode\n";
    return usage;
}

std::string HooCLI::getVersion() const {
    return std::string(COMPILER_NAME) + " version " + VERSION + " (HVM v1.4 Physical)\n";
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

    if (opts.showHelp || opts.showVersion || opts.repl) {
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
        std::string entryPoint = "_F_M_" + moduleName + "_E_main_i8";
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
    std::string entryPoint = "_F_M_" + moduleName + "_E_main_i8";
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
    hoo_args_init(argc, (const char* const*)argv);

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

    if (opts.repl) {
        hooc::repl::REPLSession session;
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
