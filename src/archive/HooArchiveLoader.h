#pragma once
#include <string>
#include <filesystem>
#include "hvm/HVMJIT.h"

namespace hooc {
namespace archive {

class HooArchiveLoader {
public:
    explicit HooArchiveLoader(HVMJIT& jit);
    bool load(const std::filesystem::path& archivePath);
    
    std::string getLastError() const { return error_; }
    const std::string& getEntryPointSymbol() const { return entryPointSymbol_; }

private:
    HVMJIT& jit_;
    std::string error_;
    std::string entryPointSymbol_;
};

} // namespace archive
} // namespace hooc
