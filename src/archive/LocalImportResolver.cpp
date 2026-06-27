#include "LocalImportResolver.h"
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"
#include <algorithm>
#include <cctype>

namespace hooc {
namespace archive {

std::string LocalImportResolver::deriveModuleName(const std::filesystem::path& rootDir, const std::filesystem::path& sourcePath) {
    auto relPath = std::filesystem::relative(sourcePath, rootDir);
    if (relPath.empty() || relPath.string().find("..") == 0) {
        throw std::runtime_error("Source file " + sourcePath.string() + " is outside of root directory " + rootDir.string());
    }

    std::string moduleName;
    bool first = true;
    for (const auto& part : relPath) {
        std::string s = part.string();
        if (s == "." || s == "..") continue;
        
        // Remove .hoo extension from the last part
        if (part == relPath.filename() && s.size() > 4 && s.substr(s.size() - 4) == ".hoo") {
            s = s.substr(0, s.size() - 4);
        }

        // Convert to snake case and sanitize
        std::string sanitized;
        for (char c : s) {
            if (std::isalnum(c)) {
                sanitized += static_cast<char>(std::tolower(c));
            } else {
                sanitized += '_';
            }
        }
        
        if (!first) {
            moduleName += ".";
        }
        moduleName += sanitized;
        first = false;
    }
    
    return moduleName;
}

void LocalImportResolver::resolveRecursive(const std::filesystem::path& rootDir, const std::filesystem::path& currentSource) {
    std::string moduleName = deriveModuleName(rootDir, currentSource);
    
    if (modules_.find(moduleName) != modules_.end()) {
        if (modules_[moduleName].sourcePath != currentSource) {
            throw std::runtime_error("Error: multiple source files normalize to module '" + moduleName + "'");
        }
        return; // Already resolved
    }

    auto sourceContent = ioProvider_.readFile(currentSource.string());
    if (!sourceContent) {
        throw std::runtime_error("Error: cannot read source file " + currentSource.string());
    }

    HooParserWrapper parser;
    auto parseTree = parser.parseForAST(*sourceContent);
    if (!parser.wasSuccessful()) {
        throw std::runtime_error("Parse errors in " + currentSource.string());
    }

    SimpleASTBuilder builder;
    auto compilationUnit = builder.buildAST(parseTree);

    ResolvedModule rm;
    rm.moduleName = moduleName;
    rm.sourcePath = currentSource;

    for (const auto& importStmtPtr : compilationUnit->getImports()) {
        const ast::ModulePath* modulePath = nullptr;
        if (auto basicImport = dynamic_cast<const ast::BasicImport*>(importStmtPtr.get())) {
            modulePath = basicImport->getModule();
        } else if (auto fromImport = dynamic_cast<const ast::FromImport*>(importStmtPtr.get())) {
            modulePath = fromImport->getModule();
        }

        if (!modulePath) continue;

        std::string impName;
        bool firstComp = true;
        for (const auto& comp : modulePath->getComponents()) {
            if (!firstComp) impName += ".";
            impName += comp;
            firstComp = false;
        }

        if (impName.rfind("hoo.", 0) == 0) {
            continue; // Skip built-in runtime modules like hoo.args, hoo.io, hoo.math
        }

        // Convert module.name to module/name.hoo
        std::string relPathStr = impName;
        std::replace(relPathStr.begin(), relPathStr.end(), '.', '/');
        relPathStr += ".hoo";
        
        std::filesystem::path targetSource = rootDir / relPathStr;
        
        if (std::filesystem::exists(targetSource)) {
            rm.localImports.push_back(impName);
            resolveRecursive(rootDir, targetSource);
        }
    }

    modules_[moduleName] = rm;
}

std::vector<ResolvedModule> LocalImportResolver::topologicalSort() {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> visiting;
    std::vector<ResolvedModule> sorted;

    for (const auto& pair : modules_) {
        if (visited.find(pair.first) == visited.end()) {
            dfs(pair.first, visited, visiting, sorted);
        }
    }

    return sorted;
}

void LocalImportResolver::dfs(const std::string& moduleName, 
                              std::unordered_set<std::string>& visited, 
                              std::unordered_set<std::string>& visiting, 
                              std::vector<ResolvedModule>& sorted) {
    if (visiting.find(moduleName) != visiting.end()) {
        throw std::runtime_error("Error: local import cycle detected involving '" + moduleName + "'");
    }
    if (visited.find(moduleName) != visited.end()) {
        return;
    }

    visiting.insert(moduleName);

    for (const auto& imp : modules_[moduleName].localImports) {
        dfs(imp, visited, visiting, sorted);
    }

    visiting.erase(moduleName);
    visited.insert(moduleName);
    sorted.push_back(modules_[moduleName]);
}

std::vector<ResolvedModule> LocalImportResolver::resolve(const std::filesystem::path& rootSourcePath) {
    modules_.clear();
    std::filesystem::path rootDir = rootSourcePath.parent_path();
    if (rootDir.empty()) {
        rootDir = ".";
    }
    
    resolveRecursive(rootDir, rootSourcePath);
    return topologicalSort();
}

} // namespace archive
} // namespace hooc
