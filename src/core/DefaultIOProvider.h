#pragma once

#include "IOProvider.h"

namespace hooc {

class DefaultIOProvider : public IOProvider {
public:
    std::optional<std::string> readFile(const std::string& filename) override;
    bool writeFile(const std::string& filename, const std::string& content) override;
    std::string readStdin() override;
    void writeStdout(const std::string& output) override;
    void writeStderr(const std::string& output) override;
};

}