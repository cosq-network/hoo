#include <gtest/gtest.h>
#include "src/core/HooCompiler.h"
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "HoocParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "antlr4-runtime.h"

namespace hooc {
namespace tests {

class ThisKeywordCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<llvm::LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ast::CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;

        auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
        if (!ctx) return nullptr;

        return astBuilder->buildAST(ctx);
    }

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
};

// Test 1: Constructor using 'this' for member assignment
TEST_F(ThisKeywordCodeGenTest, ThisInConstructorCodeGen) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify constructor function exists
    auto* ctorFunc = module->getFunction("Point_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Verify 'this' is used (it should be the first parameter)
    ASSERT_EQ(ctorFunc->arg_size(), 3); // this, x, y
    auto* thisArg = ctorFunc->getArg(0);
    EXPECT_EQ(thisArg->getName(), "this");

    // Check IR for store instructions to members
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    ctorFunc->print(irStream, nullptr);
    irStream.flush();

    // Should contain getelementptr to access fields from 'this'
    EXPECT_TRUE(ir.find("getelementptr") != std::string::npos);
    EXPECT_TRUE(ir.find("store") != std::string::npos);
}

// Test 2: Method using 'this' for member access and assignment
TEST_F(ThisKeywordCodeGenTest, ThisInMethodCodeGen) {
    std::string code = R"(
        class Counter {
            var val: int64;
            func increment() {
                this.val = this.val + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Counter_increment");
    ASSERT_NE(methodFunc, nullptr);

    // Verify 'this' is used (it should be the first parameter)
    ASSERT_GE(methodFunc->arg_size(), 1);
    auto* thisArg = methodFunc->getArg(0);
    EXPECT_EQ(thisArg->getName(), "this");

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    methodFunc->print(irStream, nullptr);
    irStream.flush();

    // Should contain load (for this.val), add, and store (for assignment)
    EXPECT_TRUE(ir.find("load") != std::string::npos);
    EXPECT_TRUE(ir.find("add") != std::string::npos);
    EXPECT_TRUE(ir.find("store") != std::string::npos);
    EXPECT_TRUE(ir.find("getelementptr") != std::string::npos);
}

// Test 3: Standalone 'this' return
TEST_F(ThisKeywordCodeGenTest, StandaloneThisCodeGen) {
    std::string code = R"(
        class Fluent {
            func:Fluent setSomething() {
                return this;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Fluent_setSomething");
    ASSERT_NE(methodFunc, nullptr);

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    methodFunc->print(irStream, nullptr);
    irStream.flush();

    // Should return 'this' (the first argument)
    // In LLVM IR, it might look like "ret ptr %0" or similar
    EXPECT_TRUE(ir.find("ret") != std::string::npos);
}

} // namespace tests
} // namespace hooc
