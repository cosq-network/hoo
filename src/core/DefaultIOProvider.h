#pragma once

#include "IOProvider.h"

namespace hooc {

class DefaultIOProvider : public IOProvider {
public:
    std::optional<std::string> readFile(const std::string& filename) override;
    bool writeFile(const std::string& filename, const std::string& content) override;
    std::optional<std::vector<uint8_t>> readBinaryFile(const std::string& filename) override;
    bool writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) override;
    std::string readStdin() override;
    void writeStdout(const std::string& output) override;
    void writeStderr(const std::string& output) override;
};

}