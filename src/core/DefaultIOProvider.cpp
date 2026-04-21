#include "DefaultIOProvider.h"
#include <fstream>
#include <iostream>

namespace hooc {

std::optional<std::string> DefaultIOProvider::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

bool DefaultIOProvider::writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    file << content;
    return file.good();
}

std::optional<std::vector<uint8_t>> DefaultIOProvider::readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return std::nullopt;
    }

    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return data;
}

bool DefaultIOProvider::writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::string DefaultIOProvider::readStdin() {
    std::string content(
        (std::istreambuf_iterator<char>(std::cin)),
        std::istreambuf_iterator<char>()
    );
    return content;
}

void DefaultIOProvider::writeStdout(const std::string& output) {
    std::cout << output;
    std::cout.flush();
}

void DefaultIOProvider::writeStderr(const std::string& output) {
    std::cerr << output;
    std::cerr.flush();
}

}