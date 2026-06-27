#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "core/IOProvider.h"
#include "LocalImportResolver.h"

namespace hooc {
namespace archive {

class HooBuildPlanner {
public:
    explicit HooBuildPlanner(IOProvider& ioProvider);
    std::vector<ResolvedModule> plan(const std::filesystem::path& rootSourcePath);

private:
    IOProvider& ioProvider_;
};

class HooArchiveCompiler {
public:
    explicit HooArchiveCompiler(IOProvider& ioProvider);
    void compile(const std::vector<ResolvedModule>& modules, const std::filesystem::path& outPath);

private:
    IOProvider& ioProvider_;
};

} // namespace archive
} // namespace hooc
