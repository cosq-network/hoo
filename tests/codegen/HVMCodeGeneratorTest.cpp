#include <gtest/gtest.h>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HOModule.h"

using namespace hooc;
using namespace hvm;

class HVMCodeGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    const Symbol* findSymbol(const HOModule& mod, const std::string& baseName) {
        for (const auto& sym : mod.getSymbols()) {
            if (sym.name.find(baseName) != std::string::npos) return &sym;
        }
        return nullptr;
    }

    std::unique_ptr<HooCompiler> compiler_;
};

TEST_F(HVMCodeGeneratorTest, CompileSimpleFunction) {
    std::string code = R"(
        func:int64 add(a: int64, b: int64) {
            return a + b;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "test");

    // Check symbols (robustly)
    auto sym = findSymbol(*module, "add");
    ASSERT_NE(sym, nullptr);
    
    // Check instructions
    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    ASSERT_GE(insts.size(), 4);
    
    EXPECT_EQ(insts[0].getOpcode(), Opcode::ENTER);
    
    bool foundAdd = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops) && std::get<OperandsR>(ops).func == 0) {
                foundAdd = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundAdd);
}

TEST_F(HVMCodeGeneratorTest, GlobalVariables) {
    std::string code = R"(
        var g_alpha: int64 = 1;
        var g_beta: int64 = 2;
        func main() { return; }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto sym1 = module->getSymbol("g_alpha");
    if (!sym1) sym1 = findSymbol(*module, "g_alpha");
    ASSERT_NE(sym1, nullptr);
    EXPECT_EQ(sym1->type, Symbol::STT_OBJECT);

    auto sym2 = module->getSymbol("g_beta");
    if (!sym2) sym2 = findSymbol(*module, "g_beta");
    ASSERT_NE(sym2, nullptr);
    EXPECT_EQ(sym2->type, Symbol::STT_OBJECT);

    auto dataSec = module->getSection(".data");
    ASSERT_NE(dataSec, nullptr);
    EXPECT_GE(dataSec->data.size(), 16); 
}

TEST_F(HVMCodeGeneratorTest, IfElse) {
    std::string code = R"(
        func:int64 max(a: int64, b: int64) {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCmp = false;
    bool foundBne = false;
    bool foundBeq = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CMP) foundCmp = true;
        if (inst.getOpcode() == Opcode::BEQ) foundBeq = true;
        if (inst.getOpcode() == Opcode::BNE) foundBne = true;
    }
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundBeq || foundBne);
}

TEST_F(HVMCodeGeneratorTest, WhileLoop) {
    std::string code = R"(
        func:int64 sum(n: int64) {
            var i: int64 = 0;
            var total: int64 = 0;
            while (i < n) {
                total = total + i;
                i = i + 1;
            }
            return total;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundJmp = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::JMP) {
            foundJmp = true;
            break;
        }
    }
    EXPECT_TRUE(foundJmp);
}

TEST_F(HVMCodeGeneratorTest, ForRange) {
    std::string code = R"(
        func:int64 sumRange(n: int64) {
            var total: int64 = 0;
            for i in 0..n {
                total = total + i;
            }
            return total;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    // Should have multiple LD_D/ST_D and jumps
    EXPECT_GE(insts.size(), 10);
}

TEST_F(HVMCodeGeneratorTest, ArrayLiteral) {
    std::string code = R"(
        func:object getArray() {
            return [1, 2, 3];
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCall = true;
            break;
        }
    }
    EXPECT_TRUE(foundCall);
}

TEST_F(HVMCodeGeneratorTest, StringLiteral) {
    std::string code = R"(
        func:object hello() {
            return "hello";
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto rodata = module->getSection(".rodata");
    ASSERT_NE(rodata, nullptr);
    std::string content(reinterpret_cast<char*>(rodata->data.data()));
    EXPECT_EQ(content, "hello");

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundLda = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LDA) {
            foundLda = true;
            break;
        }
    }
    EXPECT_TRUE(foundLda);
}

TEST_F(HVMCodeGeneratorTest, CompoundAssignment) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            x += 5;
            return x;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundAdd = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            foundAdd = true;
            break;
        }
    }
    EXPECT_TRUE(foundAdd);
}

TEST_F(HVMCodeGeneratorTest, PostfixIncrement) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            x++;
            return x;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundAdd = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            foundAdd = true;
            break;
        }
    }
    EXPECT_TRUE(foundAdd);
}

TEST_F(HVMCodeGeneratorTest, InvalidBreak) {
    std::string code = R"(
        func test() {
            break;
        }
    )";

    auto module = compiler_->compile("test", code);
    // Should fail with error
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("break") != std::string::npos);
}

TEST_F(HVMCodeGeneratorTest, SingletonBuiltinSymbol) {
    std::string code = R"(
        func:int64 test() {
            var x = System.hostname();
            var y = Fs.readText("test.txt");
            return 0;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    bool foundSystem = false;
    bool foundFs = false;
    for (const auto& sym : module->getSymbols()) {
        if (sym.name.find("_M_hoo_E_System_N_hostname") != std::string::npos) foundSystem = true;
        if (sym.name.find("_M_hoo_E_fs_readText") != std::string::npos) foundFs = true;
    }

    EXPECT_TRUE(foundSystem) << "Expected System.hostname() to produce _M_hoo_E_System_N_hostname symbol";
    EXPECT_TRUE(foundFs) << "Expected Fs.readText() to produce _M_hoo_E_fs_readText symbol";
}


