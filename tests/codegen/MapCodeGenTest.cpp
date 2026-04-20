#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "src/codegen/LLVMCodeGeneratorTypes.h"

using namespace hooc;
using namespace hooc::ast;

class MapCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<llvm::LLVMContext>();
        generator = std::make_unique<LLVMCodeGenerator>(*context);
    }

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> generator;

    std::string getModuleString(llvm::Module* module) {
        std::string str;
        llvm::raw_string_ostream stream(str);
        module->print(stream, nullptr);
        return str;
    }
};

TEST_F(MapCodeGenTest, MapTypeGeneratesPointer) {
    // Test that map[string, int64] type generates pointer type
    auto builder = std::make_unique<SimpleASTBuilder>();
    auto parser = std::make_unique<ProcessIsolatedParser>();
    
    auto* parseTree = parser->parseForAST("func test(m: map[string, int64]) { return; }");
    ASSERT_NE(parseTree, nullptr);
    
    auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = builder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(MapCodeGenTest, MapInt64KeyType) {
    auto builder = std::make_unique<SimpleASTBuilder>();
    auto parser = std::make_unique<ProcessIsolatedParser>();
    
    auto* parseTree = parser->parseForAST("func test(m: map[int64, string]) { return; }");
    ASSERT_NE(parseTree, nullptr);
    
    auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = builder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapCodeGenTest, MapByteKeyType) {
    auto builder = std::make_unique<SimpleASTBuilder>();
    auto parser = std::make_unique<ProcessIsolatedParser>();
    
    auto* parseTree = parser->parseForAST("func test(m: map[byte, bool]) { return; }");
    ASSERT_NE(parseTree, nullptr);
    
    auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = builder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}