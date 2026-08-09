#include "HooCompiler.h"
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"
#include "codegen/HVMCodeGenerator.h"
#include "HoocParser.h"
#include "ast/Declaration.h"
#include "ast/Type.h"
#include <numeric>

namespace hooc {

static std::string exportedTypeName(const ast::Type* type, std::string* returnClass = nullptr) {
    if (!type) return "void";
    if (dynamic_cast<const ast::ByteSliceType*>(type)) return "ptr";
    if (dynamic_cast<const ast::ArrayType*>(type) ||
        dynamic_cast<const ast::FutureType*>(type) ||
        dynamic_cast<const ast::MapType*>(type) ||
        dynamic_cast<const ast::HashMapType*>(type) ||
        dynamic_cast<const ast::TensorType*>(type) ||
        dynamic_cast<const ast::AnyArrayType*>(type)) return "ptr";
    if (auto base = dynamic_cast<const ast::BaseType*>(type)) {
        if (!base->isPrimitive()) {
            if (returnClass) *returnClass = base->getIdentifier();
            return "ptr";
        }
        switch (base->getPrimitiveType()->getKind()) {
            case ast::PrimitiveTypeKind::INT64: return "int64";
            case ast::PrimitiveTypeKind::FLOAT:
            case ast::PrimitiveTypeKind::DOUBLE:
            case ast::PrimitiveTypeKind::F64: return "double";
            case ast::PrimitiveTypeKind::BOOL: return "bool";
            case ast::PrimitiveTypeKind::INT8: return "int8";
            case ast::PrimitiveTypeKind::BYTE: return "byte";
            case ast::PrimitiveTypeKind::CHAR: return "char";
            case ast::PrimitiveTypeKind::BIT: return "bit";
            case ast::PrimitiveTypeKind::F8: return "f8";
            case ast::PrimitiveTypeKind::STRING: return "string";
            case ast::PrimitiveTypeKind::VOID: return "void";
            default: return "ptr";
        }
    }
    return "ptr";
}

static void collectExportedFunctionMetadata(
    const ast::FunctionDeclaration& fn,
    std::unordered_map<std::string, ExternalFunctionInfo>& output,
    ExternalFunctionMetadataSets& sets) {
    ExternalFunctionInfo info;
    info.returnType = exportedTypeName(fn.getReturnType(), &info.returnClass);
    for (const auto& parameter : fn.getParameters()) {
        info.parameterTypes.push_back(exportedTypeName(&parameter->getType()));
    }
    sets[fn.getName()].push_back(info);
    output.emplace(fn.getName(), std::move(info));
}

// ============================================================================
// Construction / Destruction
// ============================================================================

HooCompiler::HooCompiler()
    : lastCompilationSuccessful_(false)
    , parser_(std::make_unique<HooParserWrapper>())
    , astBuilder_(std::make_unique<SimpleASTBuilder>())
    , codeGenerator_(std::make_unique<HVMCodeGenerator>()) {
}

HooCompiler::~HooCompiler() = default;

void HooCompiler::setExternalFunctionImports(
    const std::unordered_map<std::string, std::pair<std::string, std::string>>& functions) {
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) {
        hvmCodeGen->setExternalFunctionImports(functions);
    }
}

void HooCompiler::setExternalFunctionMetadata(
    const std::unordered_map<std::string, ExternalFunctionInfo>& functions) {
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) hvmCodeGen->setExternalFunctionMetadata(functions);
}

void HooCompiler::setExternalFunctionMetadataSets(const ExternalFunctionMetadataSets& functions) {
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) hvmCodeGen->setExternalFunctionMetadataSets(functions);
}

// ============================================================================
// Compilation API
// ============================================================================

std::unique_ptr<hvm::HOModule> HooCompiler::compile(
    const std::string& moduleName,
    const std::string& sourceCode) {

    lastCompilationSuccessful_ = false;
    lastError_.clear();
    exportedFunctionMetadata_.clear();
    exportedFunctionMetadataSets_.clear();

    // 1. Parse
    auto* parseTree = parser_->parseForAST(sourceCode);
    if (!parseTree) {
        lastError_ = "Parse error: " + parser_->getLastError();
        return nullptr;
    }

    // 2. Build AST
    std::unique_ptr<ast::CompilationUnit> ast;
    try {
        ast = astBuilder_->buildAST(parseTree);
    } catch (const std::exception& e) {
        lastError_ = std::string("AST building failed: ") + e.what();
        return nullptr;
    }

    if (!ast) {
        lastError_ = "AST building failed: unknown error";
        return nullptr;
    }

    for (const auto& declaration : ast->getDeclarations()) {
        if (auto fn = dynamic_cast<const ast::FunctionDeclaration*>(declaration.get())) {
            collectExportedFunctionMetadata(*fn, exportedFunctionMetadata_, exportedFunctionMetadataSets_);
        } else if (auto overloads = dynamic_cast<const ast::OverloadList*>(declaration.get())) {
            for (const auto& fn : overloads->getFunctions()) {
                collectExportedFunctionMetadata(*fn, exportedFunctionMetadata_, exportedFunctionMetadataSets_);
            }
        }
    }

    // 3. Generate HVM Bytecode
    // We can cast here as long as we only support HVMCodeGenerator in this method,
    // but we use the virtual interface where possible.
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) {
        hvmCodeGen->setModuleContext(moduleName);
    }
    
    auto generatedModule = codeGenerator_->generateModule(*ast);

    if (hvmCodeGen && hvmCodeGen->hasErrors()) {
        const auto& errors = hvmCodeGen->getErrors();
        if (errors.size() == 1) {
            lastError_ = "HVM Code generation failed: " + errors.front();
        } else {
            lastError_ = "HVM Code generation failed with " + std::to_string(errors.size()) + " errors:\n";
            for (const auto& err : errors) {
                lastError_ += "  - " + err + "\n";
            }
        }
        return nullptr;
    }

    if (!generatedModule) {
        lastError_ = "HVM generation failed";
        return nullptr;
    }

    auto hvmModule = static_cast<HVMGeneratedModule*>(generatedModule.get())->takeModule();
    hvmModule->setName(moduleName);

    lastCompilationSuccessful_ = true;
    return hvmModule;
}

}  // namespace hooc
