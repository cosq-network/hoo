#include <gtest/gtest.h>
#include "HooCompiler.h"
#include "LLVMCodeGenerator.h"
#include "SimpleASTBuilder.h"
#include "ProcessIsolatedParser.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "../antlr4/generated/HoocParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "antlr4-runtime.h"

namespace hooc {
namespace tests {

class MemberAccessCodeGenTest : public ::testing::Test {
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

// Test 1: Simple member access code generation
TEST_F(MemberAccessCodeGenTest, SimpleMemberAccessCodeGen) {
    std::string code = R"(
        class Person {
            var name: string;
            var age: int64;
            constructor() {}
        }

        func test() {
            var p: Person = new Person();
            var age = p.age;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify test function exists
    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Check IR for load instruction (member access should load the value)
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("load") != std::string::npos);
}

// Test 2: Member access with integer arithmetic
TEST_F(MemberAccessCodeGenTest, MemberAccessArithmetic) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor() {}
        }

        func test() {
            var p: Point = new Point();
            var sum = p.x + p.y;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Check for add instruction
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("add") != std::string::npos);
}

// Test 3: Chained member access code generation
TEST_F(MemberAccessCodeGenTest, ChainedMemberAccessCodeGen) {
    std::string code = R"(
        class Address {
            var city: string;
            constructor() {}
        }

        class Person {
            var address: Address;
            constructor() {}
        }

        func test() {
            var p: Person = new Person();
            var city = p.address.city;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 4: Member access in function call
TEST_F(MemberAccessCodeGenTest, MemberAccessAsArgument) {
    std::string code = R"(
        class Data {
            var value: int64;
            constructor() {}
        }

        func process(x: int64) -> int64 {
            return x;
        }

        func test() {
            var d: Data = new Data();
            var result = process(d.value);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    auto* processFunc = module->getFunction("process");
    ASSERT_NE(processFunc, nullptr);

    // Check IR for call to process
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("call") != std::string::npos);
}

// Test 5: Multiple member access
TEST_F(MemberAccessCodeGenTest, MultipleMemberAccess) {
    std::string code = R"(
        class Box {
            var width: int64;
            var height: int64;
            var depth: int64;
            constructor() {}
        }

        func test() {
            var b: Box = new Box();
            var w = b.width;
            var h = b.height;
            var d = b.depth;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Should have multiple load instructions
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    int loadCount = 0;
    size_t pos = 0;
    while ((pos = ir.find("load", pos)) != std::string::npos) {
        loadCount++;
        pos += 4;
    }
    EXPECT_GE(loadCount, 3); // At least 3 loads for width, height, depth
}

// Test 6: Member access in comparison
TEST_F(MemberAccessCodeGenTest, MemberAccessComparison) {
    std::string code = R"(
        class Value {
            var data: int64;
            constructor() {}
        }

        func test() {
            var v: Value = new Value();
            if (v.data > 10) {
                var x = 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Check for comparison
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("icmp") != std::string::npos || ir.find("fcmp") != std::string::npos);
}

// Test 7: Member access with array indexing
TEST_F(MemberAccessCodeGenTest, ArrayAccessOnMember) {
    std::string code = R"(
        class Collection {
            var items: int64[];
            constructor() {}
        }

        func test() {
            var c: Collection = new Collection();
            var first = c.items[0];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 8: Member access return value
TEST_F(MemberAccessCodeGenTest, MemberAccessReturn) {
    std::string code = R"(
        class Holder {
            var value: int64;
            constructor() {}
        }

        func getValue() -> int64 {
            var h: Holder = new Holder();
            return h.value;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("getValue");
    ASSERT_NE(func, nullptr);

    // Check for return
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    func->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("ret") != std::string::npos);
}

// Test 9: Multiple classes with member access
TEST_F(MemberAccessCodeGenTest, MultipleClassesMemberAccess) {
    std::string code = R"(
        class ClassA {
            var fieldA: int64;
            constructor() {}
        }

        class ClassB {
            var fieldB: double;
            constructor() {}
        }

        func test() {
            var a: ClassA = new ClassA();
            var b: ClassB = new ClassB();
            var x = a.fieldA;
            var y = b.fieldB;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Check IR for multiple loads (one for each field access)
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("load") != std::string::npos);
}

// Test 10: Member access with type casting/conversion
TEST_F(MemberAccessCodeGenTest, MemberAccessTypeCompatibility) {
    std::string code = R"(
        class Data {
            var intVal: int64;
            var floatVal: float;
            constructor() {}
        }

        func test() {
            var d: Data = new Data();
            var i = d.intVal;
            var f = d.floatVal;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

} // namespace tests
} // namespace hooc
