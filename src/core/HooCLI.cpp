#include "core/HooCLI.h"
#include "core/HooCompiler.h"
#include "core/SymbolMangler.h"
#include "hvm/HOModule.h"
#include "hvm/HVMJIT.h"
#include "runtime/lib/hoo_args.h"
#include "repl/REPLSession.h"
#include "archive/HooArchiveCompiler.h"
#include "archive/HooArchiveLoader.h"

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
    usage += "  <file>.hoo      Source file for HVMJIT compilation to .ha archive\n";
    usage += "  <file>.ha       Archive file for direct execution via HVMJIT\n";
    usage += "\n";
    usage += "Options:\n";
    usage += "  -h, --help      Display this help message\n";
    usage += "  -v, --version   Display version information\n";
    usage += "  -o, --output <file>\n";
    usage += "                  Specify output .ha file path\n";
    usage += "  --exec          Execute the compiled output immediately\n";
    usage += "  --repl          Enter interactive REPL mode\n";
    usage += "  --verbose       Enable verbose logging\n";
    usage += "  --              End hoo options; remaining arguments are passed to the Hoo program\n";
    usage += "\n";
    usage += "Examples:\n";
    usage += "  " + std::string(programName) + " script.hoo           # Compile to script.ha\n";
    usage += "  " + std::string(programName) + " script.hoo --exec    # Compile to script.ha and run\n";
    usage += "  " + std::string(programName) + " app.ha               # Execute app.ha archive\n";
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
        }
        else if (arg.rfind("-o=", 0) == 0) {
            std::string value = std::string(arg.substr(std::string_view("-o=").size()));
            if (value.empty()) {
                opts.hasError = true;
                opts.errorMessage = "Error: -o option requires an output file path\n";
                return opts;
            }
            opts.outputFile = std::move(value);
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
        else if (arg == "--exec") {
            opts.exec = true;
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
            opts.errorMessage = "Error: Multiple input files specified. Only one .hoo or .ha file is allowed.\n";
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

    if (ext == ".ha") {
        if (opts.repl) {
            opts.hasError = true;
            opts.errorMessage = "Error: REPL preload input must be a .hoo source file\n";
            return opts;
        }
        opts.isArchive = true;
    } else if (ext == ".hoo") {
        opts.isArchive = false;
    } else {
        opts.hasError = true;
        opts.errorMessage = "Error: Invalid file extension for '" + filename + "'. Expected .hoo or .ha\n";
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
    
    std::string targetArchivePath = opts.outputFile.value_or(moduleName + ".ha");

    if (opts.isArchive) {
        verboseLog(opts, "Loading .ha archive: " + std::string(filename));
        archive::HooArchiveLoader loader(jit);
        if (!loader.load(std::string(filename))) {
            ioProvider_->writeStderr("Failed to load archive: " + loader.getLastError() + "\n");
            return 1;
        }
        
        std::string entryPoint = loader.getEntryPointSymbol();
        if (entryPoint.empty()) {
            ioProvider_->writeStderr("Execution failed: No entry point found in archive\n");
            return 1;
        }
        
        verboseLog(opts, "Executing entry point: " + entryPoint);
        int64_t result = jit.run(entryPoint);
        if (jit.hasError()) {
            ioProvider_->writeStderr("Execution failed: " + jit.getLastError() + "\n");
            return 1;
        }
        ioProvider_->writeStdout(std::to_string(result) + "\n");
        return 0;
    }

    verboseLog(opts, "Planning build for: " + std::string(filename));
    archive::HooBuildPlanner planner(*ioProvider_);
    std::vector<archive::ResolvedModule> modules;
    try {
        modules = planner.plan(std::string(filename));
    } catch (const std::exception& e) {
        ioProvider_->writeStderr("Build planning failed: " + std::string(e.what()) + "\n");
        return 1;
    }
    
    verboseLog(opts, "Compiling " + std::to_string(modules.size()) + " module(s) into archive...");
    archive::HooArchiveCompiler compiler(*ioProvider_);
    try {
        compiler.compile(modules, targetArchivePath);
        verboseLog(opts, "Successfully built archive: " + targetArchivePath);
    } catch (const std::exception& e) {
        ioProvider_->writeStderr("Compilation failed: " + std::string(e.what()) + "\n");
        return 1;
    }

    if (!opts.exec && !opts.repl) {
        ioProvider_->writeStdout("Archive successfully built: " + targetArchivePath + "\n");
        return 0;
    }
    
    if (opts.exec) {
        verboseLog(opts, "Loading compiled archive into JIT...");
        archive::HooArchiveLoader loader(jit);
        if (!loader.load(targetArchivePath)) {
            ioProvider_->writeStderr("Failed to load archive: " + loader.getLastError() + "\n");
            return 1;
        }
        
        std::string entryPoint = loader.getEntryPointSymbol();
        if (entryPoint.empty()) {
            ioProvider_->writeStderr("Execution failed: No entry point found in compiled archive\n");
            return 1;
        }
        
        verboseLog(opts, "Executing entry point: " + entryPoint);
        int64_t result = jit.run(entryPoint);
        if (jit.hasError()) {
            ioProvider_->writeStderr("Execution failed: " + jit.getLastError() + "\n");
            return 1;
        }
        ioProvider_->writeStdout(std::to_string(result) + "\n");
    }

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
    
    if (opts.isArchive) {
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
