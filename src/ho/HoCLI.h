#ifndef HO_HO_CLI_H
#define HO_HO_CLI_H

#include <string>
#include <vector>

constexpr const char* HO_VERSION = "1.0.0";

struct HoOptions {
    bool showHelp = false;
    bool showVersion = false;
    bool buildMode = false;
    std::string inputFile;
    std::string errorMessage;
};

HoOptions parseHoArgs(const std::vector<std::string>& args);

std::string validateHoInputFile(const HoOptions& opts);

int runHo(const std::vector<std::string>& args);

void printHoHelp();

void printHoVersion();

#endif
