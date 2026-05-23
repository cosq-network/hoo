#pragma once

#include "IOProvider.h"
#include <memory>
#include <string>
#include <string_view>
#include <optional>

namespace hooc {

class HooCLI {
public:
    explicit HooCLI(std::unique_ptr<IOProvider> ioProvider);
    ~HooCLI();

    [[nodiscard]] int run(int argc, char* argv[]);

    [[nodiscard]] IOProvider* getIOProvider() const;

private:
    struct Options {
        bool verbose = false;
        bool showHelp = false;
        bool showVersion = false;
        bool compileOnly = false;
        std::optional<std::string> inputFile;
        std::optional<std::string> outputFile;
        
        bool isBytecode = false;
        bool hasError = false;
        std::string errorMessage;
    };

    std::unique_ptr<IOProvider> ioProvider_;

    [[nodiscard]] Options parseArguments(int argc, char* argv[]) const;
    [[nodiscard]] std::string getUsage(std::string_view programName) const;
    [[nodiscard]] std::string getVersion() const;
    void verboseLog(const Options& opts, std::string_view message) const;
    [[nodiscard]] std::string extractModuleName(std::string_view filename) const;
    [[nodiscard]] int compileAndExecute(const Options& opts, std::string_view filename, std::string_view sourceCode) const;
};

}
