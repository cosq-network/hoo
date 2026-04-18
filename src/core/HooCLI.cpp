#include "core/HooCLI.h"
#include "jit/HoocJIT.h"

#include <iostream>
#include <cstdlib>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace hooc;

constexpr const char* COMPILER_NAME = "hooc";
constexpr const char* VERSION = "1.0.0";

HooCLI::HooCLI(std::unique_ptr<IOProvider> ioProvider)
    : ioProvider_(std::move(ioProvider)) {}

HooCLI::~HooCLI() = default;

IOProvider* HooCLI::getIOProvider() {
    return ioProvider_.get();
}

std::string HooCLI::getUsage(const char* programName) {
    std::string usage;
    usage += "Usage: " + std::string(programName) + " [options] <source_file>\n";
    usage += "\n";
    usage += "Options:\n";
    usage += "  -h, --help      Display this help message\n";
    usage += "  -v, --version   Display version information\n";
    usage += "  --verbose       Enable verbose logging\n";
    usage += "  --print-ir      Print generated LLVM IR\n";
    usage += "\n";
    usage += "Examples:\n";
    usage += "  " + std::string(programName) + " hello.hoo\n";
    usage += "  " + std::string(programName) + " --verbose --print-ir hello.hoo\n";
    return usage;
}

std::string HooCLI::getVersion() {
    return std::string(COMPILER_NAME) + " version " + VERSION + "\n";
}

HooCLI::Options HooCLI::parseArguments(int argc, char* argv[]) {
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
            ioProvider_->writeStderr("Error: Unknown option '" + arg + "'\n");
            std::exit(1);
        }
        else if (!opts.inputFile.has_value()) {
            opts.inputFile = arg;
        }
        else {
            ioProvider_->writeStderr("Error: Unexpected argument '" + arg + "'\n");
            std::exit(1);
        }
    }

    return opts;
}

void HooCLI::verboseLog(const Options& opts, const std::string& message) {
    if (opts.verbose) {
        ioProvider_->writeStderr("[VERBOSE] " + message + "\n");
    }
}

std::string HooCLI::extractModuleName(const std::string& filename) {
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

int HooCLI::compileAndExecute(const Options& opts,
                              const std::string& filename,
                              const std::string& sourceCode) {
    HoocJIT jit;

    std::string moduleName = extractModuleName(filename);
    verboseLog(opts, "Module name: " + moduleName);
    verboseLog(opts, "Compiling source code...");

    auto result = jit.compile(moduleName, sourceCode);

    if (!result.success) {
        ioProvider_->writeStderr("Compilation failed: " + result.error + "\n");
        return 1;
    }

    verboseLog(opts, "Compilation successful");

    if (opts.printIR && !result.ir.empty()) {
        ioProvider_->writeStdout(result.ir + "\n");
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

    if (opts.showHelp) {
        ioProvider_->writeStdout(getUsage(argv[0]));
        return 0;
    }

    if (opts.showVersion) {
        ioProvider_->writeStdout(getVersion());
        return 0;
    }

    if (!opts.inputFile.has_value()) {
        ioProvider_->writeStderr("Error: No input file specified\n\n");
        ioProvider_->writeStderr(getUsage(argv[0]));
        return 1;
    }

    const std::string& filename = opts.inputFile.value();

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
    verboseLog(opts, "Initializing JIT compiler...");

    try {
        return compileAndExecute(opts, filename, sourceCode);
    }
    catch (const std::exception& e) {
        ioProvider_->writeStderr("Error: " + std::string(e.what()) + "\n");
        return 1;
    }
}