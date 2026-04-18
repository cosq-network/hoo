#pragma once

#include <string>
#include <optional>
#include <vector>

namespace hooc {

class IOProvider {
public:
    virtual ~IOProvider() = default;

    virtual std::optional<std::string> readFile(const std::string& filename) = 0;
    virtual bool writeFile(const std::string& filename, const std::string& content) = 0;
    virtual std::string readStdin() = 0;
    virtual void writeStdout(const std::string& output) = 0;
    virtual void writeStderr(const std::string& output) = 0;
    virtual std::string getStdout() const { return {}; }
    virtual std::string getStderr() const { return {}; }
};

}