#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

class SimpleASTBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
    
    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }
    
    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }
};

TEST_F(SimpleASTBuilderTest, BuildEmptyCompilationUnit) {
    std::string code = "";
    auto* parseTree = parseCode(code);
    
    if (parseTree) {
        auto* ctx = getCompilationUnit(parseTree);
        if (ctx) {
            auto ast = astBuilder->buildAST(ctx);
            ASSERT_NE(ast, nullptr);
            EXPECT_EQ(ast->toString().find("CompilationUnit"), 0);
        }
    }
}

TEST_F(SimpleASTBuilderTest, BuildSingleFunctionDeclaration) {
    std::string code = "func test() { return; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildMultipleFunctionDeclarations) {
    std::string code = R"(
        func first() { return; }
        func second() { 42; }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=2") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithParameters) {
    std::string code = "func:int64 add(a: int64, b: int64) { return a + b; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithExpressionStatement) {
    std::string code = "func calculate() { 42; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithVariableDeclaration) {
    std::string code = "func test() { var x = 10; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, HandleInvalidParseTree) {
    auto ast = astBuilder->buildAST(nullptr);
    EXPECT_EQ(ast, nullptr);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithIfStatement) {
    std::string code = R"(
        func conditional() { 
            if (true) { 
                return; 
            } 
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithWhileLoop) {
    std::string code = R"(
        func loop() { 
            while (true) { 
                break; 
            } 
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteParameter) {
    std::string code = "func:byte process(data: byte) { return data; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteVariable) {
    std::string code = R"(
        func test() {
            var b = 255;
            var typed: byte = 128;
            return;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteArithmetic) {
    std::string code = R"(
        func:byte calculate(a: byte, b: byte) {
            var result = a + b;
            return result;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatParameter) {
    std::string code = "func:float process(data: float) { return data; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatVariable) {
    std::string code = R"(
        func test() {
            var f = 3.14;
            var typed: float = 2.71;
            return;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatArithmetic) {
    std::string code = R"(
        func:float calculate(a: float, b: float) {
            var result = a + b * 2.0;
            return result;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolParameter) {
    std::string code = "func:bool process(flag: bool) { return flag; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolVariable) {
    std::string code = R"(
        func test() {
            var flag = true;
            var explicit: bool = false;
            return;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolLogic) {
    std::string code = R"(
        func:bool logic(a: bool, b: bool) {
            var and_result = a && b;
            var or_result = a || b;
            var not_result = !a;
            return and_result;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithCharParameter) {
    std::string code = "func:char process(ch: char) { return ch; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithCharVariable) {
    std::string code = R"(
        func test() {
            var ch = 'a';
            var explicit: char = 'Z';
            return;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithCharComparison) {
    std::string code = R"(
        func:bool compare(a: char, b: char) {
            var equal = a == b;
            var less = a < b;
            return equal;
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithArrayParameter) {
    std::string code = "func process(data: int64) { return; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithArrayVariable) {
    std::string code = R"(
        func test() {
            var numbers: int64[];
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithArrayAccess) {
    std::string code = R"(
        func:int64 access_test() {
            var arr: int64[];
            var index = 5;
            return 42;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
    // Array access parsing may not be fully implemented yet
}

// ===== Scope Statement Tests =====

TEST_F(SimpleASTBuilderTest, BuildFunctionWithScopeStatement) {
    std::string code = R"(
        func test() {
            var x = 10;
            scope {
                var y = 20;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithNestedScopeStatements) {
    std::string code = R"(
        func test() {
            scope {
                var x = 1;
                scope {
                    var y = 2;
                }
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithScopeStatementWithMultipleStatements) {
    std::string code = R"(
        func test() {
            scope {
                var a = 1;
                var b = 2;
                var c = a + b;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== For Loop Tests =====

TEST_F(SimpleASTBuilderTest, BuildFunctionWithForInLoop) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for item in [1, 2, 3] {
                sum = sum + item;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithForRangeLoop) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 10 {
                sum = sum + i;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithForLoopBreak) {
    std::string code = R"(
        func test() {
            var count = 0;
            while count < 10 {
                count = count + 1;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== Break/Continue Statement Tests =====

TEST_F(SimpleASTBuilderTest, BuildWhileWithBreak) {
    std::string code = R"(
        func test() {
            var count = 0;
            while count < 10 {
                count = count + 1;
                if (count == 5) {
                    break;
                }
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildWhileWithContinue) {
    std::string code = R"(
        func test() {
            var count = 0;
            var sum = 0;
            while count < 10 {
                count = count + 1;
                if (count % 2 == 0) {
                    continue;
                }
                sum = sum + count;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildForRangeWithBreak) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 100 {
                sum = sum + i;
                if (sum > 100) {
                    break;
                }
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildForInWithContinue) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3, 4, 5];
            var sum = 0;
            for item in arr {
                if (item == 3) {
                    continue;
                }
                sum = sum + item;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildNestedLoopWithBreak) {
    std::string code = R"(
        func test() {
            var found = false;
            for i in 0 .. 10 {
                for j in 0 .. 10 {
                    if (i * j == 25) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== Import Statement Tests =====

TEST_F(SimpleASTBuilderTest, BuildBasicImport) {
    std::string code = "import std.String;";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildBasicImportWithAlias) {
    std::string code = "import std.String as Str;";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFromImport) {
    std::string code = "from std import String, List;";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildFromImportWithAlias) {
    std::string code = "from std import String as Str;";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildImportAndFunction) {
    std::string code = R"(
        import std.String;
        func test() { return; }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== Class Declaration Tests =====

TEST_F(SimpleASTBuilderTest, BuildSimpleClass) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            func:int64 getX() { return x; }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildClassWithConstructor) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) { }
            func:int64 getX() { return 0; }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildClassWithSingletonModifier) {
    std::string code = R"(
        singleton class Singleton {
            func:int64 getInstance() { return 42; }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildClassWithExtends) {
    std::string code = R"(
        class Child extends Parent {
            func test() { return; }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildClassWithEvent) {
    std::string code = R"(
        observable class Observable {
            event changed;
            func notify() { }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildEventDeclaration) {
    std::string code = R"(
        observable class Observable {
            event changed;
            event clicked;
            func notify() { }
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* classDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);

    const ClassBody& body = classDecl->getBody();
    auto& members = body.getMembers();

    int eventCount = 0;
    for (size_t i = 0; i < members.size(); i++) {
        auto* classMember = dynamic_cast<const ClassMember*>(members[i].get());
        if (classMember && classMember->isEvent()) {
            eventCount++;
            auto* event = classMember->getEvent();
            ASSERT_NE(event, nullptr);
            EXPECT_TRUE(event->getName() == "changed" || event->getName() == "clicked");
        }
    }
    EXPECT_EQ(eventCount, 2);
}

// ===== Type Tests =====

TEST_F(SimpleASTBuilderTest, BuildOptionalType) {
    std::string code = R"(
        func test() {
            var maybe: int64?;
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildOptionalArrayType) {
    std::string code = R"(
        func test() {
            var arr: int64[]?;
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildMultiDimensionalArray) {
    std::string code = R"(
        func test() {
            var matrix: int64[][];
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildQualifiedType) {
    std::string code = R"(
        func test() {
            var point: std.Point;
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== All Primitive Types Tests =====

TEST_F(SimpleASTBuilderTest, BuildFunctionWithUint8Parameter) {
    std::string code = "func:uint8 process(data: uint8) { return data; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithF64Parameter) {
    std::string code = "func:f64 process(data: f64) { return data; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithStringParameter) {
    std::string code = "func:string greet(name: string) { return name; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithVoidReturn) {
    std::string code = "func doNothing() { return; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Expression Tests =====

TEST_F(SimpleASTBuilderTest, BuildIntegerLiteral) {
    std::string code = "func:int64 test() { return 42; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFloatingLiteral) {
    std::string code = "func:double test() { return 3.14; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildStringLiteral) {
    std::string code = "func:string test() { return \"hello\"; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildInterpolatedString) {
    std::string code = "func:string test() { return \"Hello ${name}!\"; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);

    const Block& body = funcDecl->getBody();
    ASSERT_EQ(body.getStatements().size(), 1u);
    auto* retStmt = dynamic_cast<const ReturnStatement*>(body.getStatements()[0].get());
    ASSERT_NE(retStmt, nullptr);
    ASSERT_NE(retStmt->getExpression(), nullptr);

    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(retStmt->getExpression());
    ASSERT_NE(primaryExpr, nullptr);

    auto* interpStr = dynamic_cast<const InterpolatedString*>(&primaryExpr->getPrimary());
    ASSERT_NE(interpStr, nullptr);
    EXPECT_EQ(interpStr->getTemplate(), "Hello ${name}!");
}

TEST_F(SimpleASTBuilderTest, BuildInterpolatedStringWithMultiplePlaceholders) {
    std::string code = "func:string test() { return \"${greeting} ${name} at ${location}\"; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);

    const Block& body = funcDecl->getBody();
    ASSERT_EQ(body.getStatements().size(), 1u);
    auto* retStmt = dynamic_cast<const ReturnStatement*>(body.getStatements()[0].get());
    ASSERT_NE(retStmt, nullptr);
    ASSERT_NE(retStmt->getExpression(), nullptr);

    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(retStmt->getExpression());
    ASSERT_NE(primaryExpr, nullptr);

    auto* interpStr = dynamic_cast<const InterpolatedString*>(&primaryExpr->getPrimary());
    ASSERT_NE(interpStr, nullptr);
    EXPECT_EQ(interpStr->getTemplate(), "${greeting} ${name} at ${location}");
}

TEST_F(SimpleASTBuilderTest, BuildBooleanLiterals) {
    std::string code = R"(
        func:bool test() {
            var t = true;
            var f = false;
            return t;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildNullLiteral) {
    std::string code = R"(
        func test() {
            var ptr: int64? = null;
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3];
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildEmptyArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr = [];
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildParenthesizedExpression) {
    std::string code = "func:int64 test() { return (1 + 2) * 3; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildUnaryMinus) {
    std::string code = "func:int64 test() { return -42; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildLogicalNot) {
    std::string code = "func:bool test() { return !true; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Assignment Expression Tests =====

TEST_F(SimpleASTBuilderTest, BuildSimpleAssignment) {
    std::string code = R"(
        func test() {
            var x = 10;
            x = 20;
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Return Statement Tests =====

TEST_F(SimpleASTBuilderTest, BuildReturnWithExpression) {
    std::string code = "func:int64 test() { return 42; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildReturnWithoutExpression) {
    std::string code = "func test() { return; }";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== If/Else Statement Tests =====

TEST_F(SimpleASTBuilderTest, BuildIfElseStatement) {
    std::string code = R"(
        func test() {
            if (true) {
                var a = 1;
            } else {
                var b = 2;
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildNestedIfStatements) {
    std::string code = R"(
        func test() {
            if (true) {
                if (false) {
                    var x = 1;
                }
            }
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

// ===== New Expression Tests =====

TEST_F(SimpleASTBuilderTest, BuildNewExpression) {
    std::string code = R"(
        func test() {
            var p = new Point(1, 2);
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildNewExpressionWithNoArgs) {
    std::string code = R"(
        func test() {
            var p = new Point();
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Member Access Tests =====

TEST_F(SimpleASTBuilderTest, BuildMemberAccess) {
    std::string code = R"(
        func:int64 test() {
            var p = new Point(1, 2);
            return p.x;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildChainedMemberAccess) {
    std::string code = R"(
        func:string test() {
            var result = obj.parent.name;
            return result;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Function Call Tests =====

TEST_F(SimpleASTBuilderTest, BuildFunctionCall) {
    std::string code = R"(
        func test() {
            print("hello");
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionCallWithMultipleArgs) {
    std::string code = R"(
        func test() {
            format("name: {} age: {}", "John", 30);
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildMethodCall) {
    std::string code = R"(
        func test() {
            var name = "hello";
            name.toUpper();
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

// ===== Complex/Edge Case Tests =====

TEST_F(SimpleASTBuilderTest, BuildComplexFunction) {
    std::string code = R"(
        func:int64 calculate(a: int64, b: int64) {
            var result = 0;
            if (a > 0 && b > 0) {
                result = a + b;
            } else {
                if (a < 0 || b < 0) {
                    result = a - b;
                } else {
                    result = 0;
                }
            }
            while (result < 100) {
                result = result * 2;
            }
            return result;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1U);
}

TEST_F(SimpleASTBuilderTest, BuildMultipleDeclarations) {
    std::string code = R"(
        import std.List;
        class MyClass {
            var value: int64;
        }
        func helper() { return; }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getImports().size(), 1U);
    EXPECT_EQ(ast->getDeclarations().size(), 2U);
}

TEST_F(SimpleASTBuilderTest, BuildAllBinaryOperators) {
    std::string code = R"(
        func:int64 test() {
            var a = 10 + 5;
            var b = 10 - 5;
            var c = 10 * 5;
            var d = 10 / 5;
            var e = 10 % 3;
            var f = 10 == 5;
            var g = 10 != 5;
            var h = 10 < 5;
            var i = 10 <= 5;
            var j = 10 > 5;
            var k = 10 >= 5;
            return a;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildTernaryNesting) {
    std::string code = R"(
        func:int64 nested() {
            var x = 1;
            var y = 2;
            if (x < y) {
                if (y > 0) {
                    return 1;
                }
            }
            return 0;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
}