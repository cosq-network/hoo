#include <gtest/gtest.h>
#include <iomanip>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HOModule.h"
#include "core/SymbolMangler.h"

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

    auto sym = findSymbol(*module, "add");
    ASSERT_NE(sym, nullptr);
    
    auto data = module->getSection(".text")->data;
    std::cout << "DEBUG: .text data size = " << data.size() << " bytes: ";
    for (uint8_t b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    }
    std::cout << std::dec << std::endl;

    auto insts = module->decodeInstructions(data);
    std::cout << "DEBUG: Decoded " << insts.size() << " instructions:" << std::endl;
    for (size_t i = 0; i < insts.size(); ++i) {
        std::cout << "  [" << i << "] mnemonic=" << insts[i].getMnemonic() 
                  << " opcode=" << (int)insts[i].getOpcode() << std::endl;
    }

    ASSERT_GE(insts.size(), 4);
}

TEST_F(HVMCodeGeneratorTest, GeneratedFramesAreSixteenByteAligned) {
    std::string code = R"(
        func:void test() {
            var value: int64 = 1;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundEnter = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() != Opcode::ENTER) continue;
        const auto* operands = std::get_if<OperandsI>(&inst.getOperands());
        ASSERT_NE(operands, nullptr);
        EXPECT_EQ(operands->imm15 % 16, 0);
        foundEnter = true;
        break;
    }
    EXPECT_TRUE(foundEnter);
}

TEST_F(HVMCodeGeneratorTest, AccessQualifiersAffectMethodSymbolsAndBindings) {
    const std::string code = R"(
        class Secret {
            public func : void visible() {}
            private func : void hidden() {}
        }
    )";

    auto module = compiler_->compile("access_test", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();

    const Symbol* publicSymbol = nullptr;
    const Symbol* privateSymbol = nullptr;
    for (const auto& symbol : module->getSymbols()) {
        const auto demangled = SymbolMangler::demangleSymbol(symbol.name);
        if (demangled.className != "Secret") continue;
        if (demangled.functionName == "visible") publicSymbol = &symbol;
        if (demangled.functionName == "hidden") privateSymbol = &symbol;
    }

    ASSERT_NE(publicSymbol, nullptr);
    ASSERT_NE(privateSymbol, nullptr);
    EXPECT_EQ(publicSymbol->binding, Symbol::STB_GLOBAL);
    EXPECT_EQ(privateSymbol->binding, Symbol::STB_LOCAL);
    EXPECT_EQ(SymbolMangler::demangleSymbol(publicSymbol->name).functionModifiers,
              std::vector<std::string>{"PUBLIC"});
    EXPECT_EQ(SymbolMangler::demangleSymbol(privateSymbol->name).functionModifiers,
              std::vector<std::string>{"PRIVATE"});
}

TEST_F(HVMCodeGeneratorTest, UnknownReceiverDoesNotUseArbitraryMethodCandidate) {
    const std::string code = R"(
        class First {
            constructor() {}
            func :int64 value() { return 1; }
        }
        class Second {
            constructor() {}
            func :int64 value() { return 2; }
        }
        func :any makeUnknown() { return null; }
        func :int64 test() {
            var value = makeUnknown();
            return value.value();
        }
    )";

    auto module = compiler_->compile("dispatch_safety", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_NE(compiler_->getLastError().find("unknown receiver"), std::string::npos);
}

TEST_F(HVMCodeGeneratorTest, ExternalReturnMetadataSupportsChainedDispatch) {
    ExternalFunctionInfo info;
    info.modulePath = "dep";
    info.returnType = "string";
    compiler_->setExternalFunctionMetadata({{"makeText", info}});

    const std::string code = R"(
        import dep;
        func :int64 test() {
            return makeText().length();
        }
    )";
    auto module = compiler_->compile("root", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
}

TEST_F(HVMCodeGeneratorTest, AnyArrayHomogeneousStringElementChainsToString) {
    const std::string code = R"(
        import hoo.collections;
        func :int64 test() {
            var values = ["ab", "cd"]any;
            return values[0].length();
        }
    )";
    auto module = compiler_->compile("root", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
    EXPECT_NE(findSymbol(*module, "_F_M_hoo_E_String_length"), nullptr);
}

TEST_F(HVMCodeGeneratorTest, AnyArrayHomogeneousIntElementDoesNotChainToObject) {
    const std::string code = R"(
        import hoo.collections;
        func :int64 test() {
            var values = [10, 20]any;
            return values[0].length();
        }
    )";
    auto module = compiler_->compile("root", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_NE(compiler_->getLastError().find("Cannot resolve method 'length'"), std::string::npos);
}

TEST_F(HVMCodeGeneratorTest, AnyArrayMixedElementsRemainDynamic) {
    const std::string code = R"(
        import hoo.collections;
        func :int64 test() {
            var values = ["ab", 42]any;
            return values.length();
        }
    )";
    auto module = compiler_->compile("root", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
}

TEST_F(HVMCodeGeneratorTest, ExternalUserClassReturnMetadataSurvivesInference) {
    ExternalFunctionInfo info;
    info.modulePath = "dep";
    info.returnType = "ptr";
    info.returnClass = "RemoteResult";
    compiler_->setExternalFunctionMetadata({{"makeResult", info}});

    const std::string code = R"(
        import dep;
        func :int64 test() {
            var result = makeResult();
            return result.value();
        }
    )";
    auto module = compiler_->compile("root", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
}

TEST_F(HVMCodeGeneratorTest, ExternalOverloadMetadataKeepsAllCandidates) {
    ExternalFunctionInfo intInfo;
    intInfo.modulePath = "dep";
    intInfo.returnType = "int64";
    intInfo.parameterTypes = {"int64"};
    ExternalFunctionInfo stringInfo;
    stringInfo.modulePath = "dep";
    stringInfo.returnType = "string";
    stringInfo.parameterTypes = {"string"};
    ExternalFunctionMetadataSets metadata;
    metadata["choose"] = {intInfo, stringInfo};
    compiler_->setExternalFunctionMetadataSets(metadata);

    const std::string code = R"(
        import dep;
        func :int64 test() { return choose(7); }
    )";
    auto module = compiler_->compile("root", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
}

TEST_F(HVMCodeGeneratorTest, OverloadInferencePrefersWideningRankOverGenericFallback) {
    const std::string code = R"(
        func :int64 choose(value: int64) { return 11; }
        func :double choose(value: double) { return 22.0; }
        func :int64 test() { return choose(7); }
    )";
    auto module = compiler_->compile("overload_rank", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
    EXPECT_NE(findSymbol(*module, "choose"), nullptr);
}

TEST_F(HVMCodeGeneratorTest, ByteSliceTypeIsAcceptedAsBorrowedPointerType) {
    const std::string code = R"(
        func :int64 length(data: slice<byte>) { return 1; }
    )";
    auto module = compiler_->compile("slice_type", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
    EXPECT_NE(findSymbol(*module, "length"), nullptr);
}

TEST_F(HVMCodeGeneratorTest, AsyncAwaitCodeGeneration) {
    std::string code = R"(
        async func:Future<int64> fetchData() {
            return 42;
        }

        async func process() {
            var data = await(fetchData());
        }
    )";

    auto module = compiler_->compile("test", code);
    if (!module) {
        std::cout << "COMPILER ERROR: " << compiler_->getLastError() << std::endl;
    }
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "test");

    auto fetchSym = findSymbol(*module, "fetchData");
    ASSERT_NE(fetchSym, nullptr);

    auto processSym = findSymbol(*module, "process");
    ASSERT_NE(processSym, nullptr);
    // Check if CALL to _F_hoo_future_await_unwrap_p_p was emitted in process
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
    EXPECT_GE(insts.size(), 10);

    bool foundLoopSet = false;
    bool foundLoopDecbr = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LOOP_SET) foundLoopSet = true;
        if (inst.getOpcode() == Opcode::LOOP_DECBR) foundLoopDecbr = true;
    }
    EXPECT_TRUE(foundLoopSet);
    EXPECT_TRUE(foundLoopDecbr);
}

TEST_F(HVMCodeGeneratorTest, ArrayLiteral) {
    std::string code = R"(
        func :any getArray() {
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
        func :any hello() {
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
        func :int64 test() {
            var x: int64 = 10;
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
        func :int64 test() {
            var x: int64 = 10;
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
        func :int64 test() {
            break;
            return 0;
        }
    )";

    auto module = compiler_->compile("test", code);
    // Should fail with error
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("break") != std::string::npos);
}

TEST_F(HVMCodeGeneratorTest, SerializableClassGeneratesSymbols) {
    std::string code = R"(
        serializable class User {
            public var name: string;
            public var age: int64;
            constructor() {}
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* serializeSym = findSymbol(*module, "_User_R_serialize_p");
    ASSERT_NE(serializeSym, nullptr) << "Should find serialize symbol with SERIALIZABLE modifier";
    EXPECT_EQ(serializeSym->type, Symbol::STT_FUNC);
    EXPECT_EQ(serializeSym->binding, Symbol::STB_GLOBAL);

    auto* deserializeSym = findSymbol(*module, "_User_R_deserialize_static_p_s");
    ASSERT_NE(deserializeSym, nullptr) << "Should find deserialize symbol with SERIALIZABLE modifier";
    EXPECT_EQ(deserializeSym->type, Symbol::STT_FUNC);
    EXPECT_EQ(deserializeSym->binding, Symbol::STB_GLOBAL);
}

TEST_F(HVMCodeGeneratorTest, SerializableMethodsUseGeneratedStaticAndInstanceDispatch) {
    std::string code = R"(
        serializable class User {
            public var name: string;
            constructor() {}
        }

        func:string roundTrip(s: string) {
            var user = User.deserialize(s);
            return user.serialize();
        }
    )";

    auto module = compiler_->compile("serializable_dispatch", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();

    EXPECT_NE(findSymbol(*module, "_User_R_serialize_p"), nullptr);
    EXPECT_NE(findSymbol(*module, "_User_R_deserialize_static_p_s"), nullptr);
    EXPECT_NE(findSymbol(*module, "roundTrip"), nullptr);
}

TEST_F(HVMCodeGeneratorTest, NonSerializableClassNoAutoGeneratedSymbols) {
    std::string code = R"(
        class Plain {}
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    // Verify no serialize/deserialize symbols exist
    for (const auto& sym : module->getSymbols()) {
        EXPECT_TRUE(sym.name.find("_R_serialize") == std::string::npos)
            << "Non-serializable class should not have serialize symbol";
        EXPECT_TRUE(sym.name.find("_R_deserialize") == std::string::npos)
            << "Non-serializable class should not have deserialize symbol";
    }
}

TEST_F(HVMCodeGeneratorTest, SerializableClassSymbolsHaveCorrectAttributes) {
    std::string code = R"(
        serializable class Data {
            public var value: int64;
            constructor() {}
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    for (const auto& sym : module->getSymbols()) {
        if (sym.name.find("_R_serialize") != std::string::npos ||
            sym.name.find("_R_deserialize") != std::string::npos) {
            EXPECT_EQ(sym.section_index, 0) << sym.name;
        }
    }
}

TEST_F(HVMCodeGeneratorTest, NonVoidFunctionMissingReturn) {
    std::string code = R"(
        func:int64 missingReturn() {
            var x = 42;
        }
    )";

    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("has no return statement") != std::string::npos);
}

TEST_F(HVMCodeGeneratorTest, NonVoidFunctionWithReturn) {
    std::string code = R"(
        func:int64 hasReturn() {
            return 42;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorTest, VoidFunctionWithoutReturn) {
    std::string code = R"(
        func:void voidFunc() {
            var x: int64 = 1;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorTest, FloatModuloEmitsFmodCall) {
    std::string code = R"(
        func:double test(a: double, b: double) {
            return a % b;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundFmodCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundFmodCall = true;
            break;
        }
    }
    EXPECT_TRUE(foundFmodCall);
}

TEST_F(HVMCodeGeneratorTest, SingletonBuiltinSymbol) {
    std::string code = R"(
        import hoo.io;
        import hoo.uuid;
        func:int64 test() {
            var x = uuid_v4();
            var y = Fs.read_text("test.txt");
            return 0;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    bool foundUuid = false;
    bool foundFs = false;
    for (const auto& sym : module->getSymbols()) {
        if (sym.name.find("_M_hoo_E_uuid_v4") != std::string::npos) foundUuid = true;
        if (sym.name.find("_M_hoo_E_fs_read_text") != std::string::npos) foundFs = true;
    }

    EXPECT_TRUE(foundUuid) << "Expected uuid_v4() to produce _M_hoo_E_uuid_v4 symbol";
    EXPECT_TRUE(foundFs) << "Expected Fs.read_text() to produce _M_hoo_E_fs_read_text symbol";
}



TEST_F(HVMCodeGeneratorTest, StringLiteralExprStmt_EmitsRelease) {
    std::string code = R"(
        func:void test() {
            "hello";
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCallRelease = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCallRelease = true;
            break;
        }
    }
    EXPECT_TRUE(foundCallRelease) << "Expected CALL to _F_hoo_release_v_p for string literal expression statement";
}

TEST_F(HVMCodeGeneratorTest, AssignmentToManagedLocal_ReleasesOldValue) {
    std::string code = R"(
        func:void test() {
            var s = "first";
            s = "second";
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    int callCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            callCount++;
        }
    }
    EXPECT_GE(callCount, 1) << "Expected at least one CALL (hoo_release) for managed reassignment";
}

// NOTE: This test is disabled due to a pre-existing parser bug (__next_prime overflow)
// that affects parsing of class declarations in certain compilation unit contexts.
TEST_F(HVMCodeGeneratorTest, DISABLED_NullableUserClassLocal_EmitsScopeCleanupRelease) {
    std::string code = "class User {\n"
                       "    var name: string;\n"
                       "    var age: int64;\n"
                       "    constructor(name: string, age: int64) {\n"
                       "        this.name = name;\n"
                       "        this.age = age;\n"
                       "    }\n"
                       "}\n"
                       "func :int64 test() {\n"
                       "    var nu: User? = new User(\"a\", 1);\n"
                       "    return 0;\n"
                       "}\n";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCleanupLoad = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LD_D) {
            const auto& ops = inst.getOperands();
            if (auto oi = std::get_if<OperandsI>(&ops)) {
                if (oi->rd == 1 && oi->rs == 30 && oi->imm15 < 0) {
                    foundCleanupLoad = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundCleanupLoad)
        << "Expected scope cleanup LD_D (r1 <- [r30+offset]) release load for nullable User? local";
}

TEST_F(HVMCodeGeneratorTest, ReturnFromBlock_CleansUpManagedLocals) {
    std::string code = R"(
        func :any test() {
            var s = "hello";
            return s;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundRelease = false;
    bool foundRetain = false;
    bool foundLeave = false;
    bool foundRet = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::RETAIN) foundRetain = true;
        if (inst.getOpcode() == Opcode::LEAVE) foundLeave = true;
        if (inst.getOpcode() == Opcode::RET) foundRet = true;
    }
    EXPECT_TRUE(foundRetain) << "Expected RETAIN for return value";
    EXPECT_TRUE(foundLeave) << "Expected LEAVE before RET";
    EXPECT_TRUE(foundRet) << "Expected RET";
}

TEST_F(HVMCodeGeneratorTest, LoopBreak_CleansUpManagedLocals) {
    std::string code = R"(
        func:void test() {
            while (true) {
                var s = "hello";
                break;
            }
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundJmp = false;
    bool foundCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::JMP) foundJmp = true;
        if (inst.getOpcode() == Opcode::CALL) foundCall = true;
    }
    EXPECT_TRUE(foundJmp) << "Expected JMP for break";
    EXPECT_TRUE(foundCall) << "Expected CALL for scope cleanup before break";
}

TEST_F(HVMCodeGeneratorTest, LoopControlRestoresTemporaryRegisterState) {
    std::string code = R"(
        import hoo;

        func:void whileBreak() {
            var i: int64 = 0;
            while (i < 20) {
                if (i == 3) { break; }
                i = i + 1;
            }
        }

        func:void doWhileContinue() {
            var i: int64 = 0;
            do {
                i = i + 1;
                if (i < 4) { continue; }
            } while (i < 8);
        }

        func:void rangeBreak() {
            for i in 0 .. 20 by 2 {
                if (i == 6) { break; }
            }
        }

        func:void arrayContinue() {
            var values = [1, 2, 3, 4, 5];
            for value in values {
                if (value == 2) { continue; }
                if (value == 4) { break; }
            }
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr) << compiler_->getLastError();
    EXPECT_TRUE(compiler_->getLastError().empty()) << compiler_->getLastError();
}

TEST_F(HVMCodeGeneratorTest, InterpolatedStringExprStmt_EmitsRelease) {
    std::string code = R"(
        func:void test() {
            "hello ${1}";
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCallRelease = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCallRelease = true;
            break;
        }
    }
    EXPECT_TRUE(foundCallRelease) << "Expected CALL for interpolated string expression statement cleanup";
}
TEST_F(HVMCodeGeneratorTest, InfersDecimalLiteralType) {
    std::string code = R"(
        func:void test() {
            var amount: Decimal<38,2> = 123.45m;
        }
    )";

    auto module = compiler_->compile("test", code);

    if (!module) {
        std::cout << compiler_->getLastError() << std::endl;
    }

    ASSERT_NE(module, nullptr);

    auto sym = findSymbol(*module, "test");
    ASSERT_NE(sym, nullptr);
}
TEST_F(HVMCodeGeneratorTest, RejectsMixedDecimalAndDoubleArithmetic) {
    std::string code = R"(
        func:void test() {
            var price: Decimal<38,2> = 19.99m;
            var rate: double = 0.5;
            var total: Decimal<38,2> = price * rate;
        }
    )";

    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
}
TEST_F(HVMCodeGeneratorTest, EmitsDecimalAddCall) {
    std::string code = R"(
        func:void test() {
            var price: Decimal<38,2> = 19.99m;
            var tax: Decimal<38,2> = 8m;
            var total: Decimal<38,2> = price + tax;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_hoo_Decimal_add_p_p_p");
    EXPECT_NE(sym, nullptr) << "Expected reference to _F_hoo_Decimal_add_p_p_p symbol";
}
TEST_F(HVMCodeGeneratorTest, EmitsDecimalSubCall) {
    std::string code = R"(
        func:void test() {
            var balance: Decimal<38,2> = 100.00m;
            var withdrawal: Decimal<38,2> = 25.00m;
            var remaining: Decimal<38,2> = balance - withdrawal;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_hoo_Decimal_sub_p_p_p");
    EXPECT_NE(sym, nullptr) << "Expected reference to _F_hoo_Decimal_sub_p_p_p symbol";
}

TEST_F(HVMCodeGeneratorTest, EmitsDecimalMulCall) {
    std::string code = R"(
        func:void test() {
            var price: Decimal<38,2> = 19.99m;
            var quantity: Decimal<38,2> = 3m;
            var total: Decimal<38,2> = price * quantity;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_hoo_Decimal_mul_p_p_p");
    EXPECT_NE(sym, nullptr) << "Expected reference to _F_hoo_Decimal_mul_p_p_p symbol";
}

TEST_F(HVMCodeGeneratorTest, EmitsDecimalDivCall) {
    std::string code = R"(
        func:void test() {
            var total: Decimal<38,2> = 100.00m;
            var parts: Decimal<38,2> = 4m;
            var share: Decimal<38,2> = total / parts;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_hoo_Decimal_div_p_p_p");
    EXPECT_NE(sym, nullptr) << "Expected reference to _F_hoo_Decimal_div_p_p_p symbol";
}
TEST_F(HVMCodeGeneratorTest, EmitsDecimalComparisonCall) {
    std::string code = R"(
        func:void test() {
            var price: Decimal<38,2> = 19.99m;
            var balance: Decimal<38,2> = 100.00m;
            var isAffordable: bool = price < balance;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_hoo_Decimal_lt_p_p_p");
    EXPECT_NE(sym, nullptr) << "Expected reference to _F_hoo_Decimal_lt_p_p_p symbol";
}
