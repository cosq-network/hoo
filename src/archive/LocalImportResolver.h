#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <stdexcept>
#include "ast/AST.h"
#include "core/IOProvider.h"

namespace hooc {
namespace archive {

struct ResolvedModule {
    std::string moduleName;
    std::filesystem::path sourcePath;
    std::vector<std::string> localImports;
};

class LocalImportResolver {
public:
    explicit LocalImportResolver(IOProvider& ioProvider) : ioProvider_(ioProvider) {}

    // Derives module name from relative path
    static std::string deriveModuleName(const std::filesystem::path& rootDir, const std::filesystem::path& sourcePath);

    // Main entry point: resolves all local imports starting from the root source file
    // Returns a list of modules in topological order (dependencies first)
    std::vector<ResolvedModule> resolve(const std::filesystem::path& rootSourcePath);

private:
    IOProvider& ioProvider_;
    std::unordered_map<std::string, ResolvedModule> modules_; // moduleName -> ResolvedModule
    
    // Internal recursive resolution
    void resolveRecursive(const std::filesystem::path& rootDir, const std::filesystem::path& currentSource);
    
    // Topological sort and cycle detection
    std::vector<ResolvedModule> topologicalSort();
    void dfs(const std::string& moduleName, 
             std::unordered_set<std::string>& visited, 
             std::unordered_set<std::string>& visiting, 
             std::vector<ResolvedModule>& sorted);
};

} // namespace archive
} // namespace hooc
