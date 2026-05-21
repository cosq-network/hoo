#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

#include "HoCLI.h"

namespace fs = std::filesystem;

namespace {
bool isPathUnder(const fs::path& root, const fs::path& candidate) {
    auto rootIt = root.begin();
    auto candidateIt = candidate.begin();

    for (; rootIt != root.end(); ++rootIt, ++candidateIt) {
        if (candidateIt == candidate.end() || *rootIt != *candidateIt) {
            return false;
        }
    }

    return true;
}
} // namespace

HoOptions parseHoArgs(const std::vector<std::string>& args) {
    HoOptions opts;

    if (args.empty()) {
        opts.showHelp = true;
        return opts;
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
            return opts;
        }
        else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
            return opts;
        }
        else if (arg == "--build") {
            opts.buildMode = true;
        }
        else if (!arg.empty() && arg[0] != '-') {
            if (!opts.inputFile.empty()) {
                opts.errorMessage = "Error: Multiple input files provided. Only one input file is allowed.";
                return opts;
            }
            opts.inputFile = arg;
        }
        else {
            opts.errorMessage = "Error: Unknown option: " + arg;
            return opts;
        }
    }

    if (opts.inputFile.empty()) {
        opts.errorMessage = "Error: No input file provided.";
        return opts;
    }

    return opts;
}

std::string validateHoInputFile(const HoOptions& opts) {
    if (opts.inputFile.empty()) {
        return "";
    }

    fs::path inputPath(opts.inputFile);

    if (inputPath.is_absolute()) {
        return "Error: Input file must be a relative path from the current working directory.";
    }

    std::string ext = inputPath.extension().string();
    if (ext != ".hoo" && ext != ".ho") {
        return "Error: Input file must have .hoo or .ho extension.";
    }

    if (ext == ".ho" && opts.buildMode) {
        return "Error: --build option is only allowed with .hoo files.";
    }

    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (ec) {
        return "Error: Unable to determine current working directory.";
    }

    cwd = fs::canonical(cwd, ec);
    if (ec) {
        return "Error: Unable to canonicalize current working directory.";
    }

    fs::path fullPath = cwd / inputPath;

    if (!fs::exists(fullPath, ec)) {
        return "Error: Input file does not exist: " + opts.inputFile;
    }
    if (ec) {
        return "Error: Unable to check input file existence: " + opts.inputFile;
    }

    if (!fs::is_regular_file(fullPath, ec)) {
        return "Error: Input path is not a regular file: " + opts.inputFile;
    }
    if (ec) {
        return "Error: Unable to inspect input file type: " + opts.inputFile;
    }

    fs::path canonicalPath = fs::canonical(fullPath, ec);
    if (ec) {
        return "Error: Unable to canonicalize input file path: " + opts.inputFile;
    }
    if (!isPathUnder(cwd, canonicalPath)) {
        return "Error: Input file must be under the current working directory.";
    }

    return "";
}

void printHoHelp() {
    std::cout << "Usage: ho [options] <input_file>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help     Display this help message\n";
    std::cout << "  -v, --version  Display version information\n";
    std::cout << "  --build        Build mode (only valid with .hoo files)\n";
    std::cout << "\nArguments:\n";
    std::cout << "  <input_file>   Input file (.hoo or .ho)\n";
    std::cout << "\nNotes:\n";
    std::cout << "  - Input file must be a relative path under the current working directory\n";
    std::cout << "  - Input file cannot escape the current working directory\n";
    std::cout << "  - --build option is only valid with .hoo files\n";
}

void printHoVersion() {
    std::cout << "ho version " << HO_VERSION << "\n";
}

int runHo(const std::vector<std::string>& args) {
    HoOptions opts = parseHoArgs(args);

    if (!opts.errorMessage.empty()) {
        std::cerr << opts.errorMessage << "\n";
        return 1;
    }

    if (opts.showHelp) {
        printHoHelp();
        return 0;
    }

    if (opts.showVersion) {
        printHoVersion();
        return 0;
    }

    std::string validationError = validateHoInputFile(opts);
    if (!validationError.empty()) {
        std::cerr << validationError << "\n";
        return 1;
    }

    std::cout << "Input file: " << opts.inputFile << "\n";
    if (opts.buildMode) {
        std::cout << "Mode: build\n";
        std::cerr << "Error: build mode is not implemented yet for ho.\n";
    } else {
        std::cout << "Mode: run\n";
        std::cerr << "Error: run mode is not implemented yet for ho.\n";
    }

    return 2;
}
