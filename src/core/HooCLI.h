#pragma once

#include "IOProvider.h"
#include <memory>
#include <string>
#include <optional>

namespace hooc {

class HooCLI {
public:
    explicit HooCLI(std::unique_ptr<IOProvider> ioProvider);
    ~HooCLI();

    int run(int argc, char* argv[]);

    IOProvider* getIOProvider();

private:
    struct Options {
        bool verbose = false;
        bool showHelp = false;
        bool showVersion = false;
        bool printIR = false;
        std::optional<std::string> inputFile;
    };

    std::unique_ptr<IOProvider> ioProvider_;

    Options parseArguments(int argc, char* argv[]);
    std::string getUsage(const char* programName);
    std::string getVersion();
    void verboseLog(const Options& opts, const std::string& message);
    std::string extractModuleName(const std::string& filename);
    int compileAndExecute(const Options& opts, const std::string& filename, const std::string& sourceCode);
};

}