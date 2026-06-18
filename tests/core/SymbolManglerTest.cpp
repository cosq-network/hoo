#include <gtest/gtest.h>
#include "src/core/SymbolMangler.h"
#include <vector>
#include <string>

using namespace hooc;

class SymbolManglerTest : public ::testing::Test {
protected:
    MangledFunctionParams makeParams() {
        MangledFunctionParams p;
        p.className = "";
        p.baseClassName = "";
        p.classModifiers = {};
        p.functionName = "";
        p.functionModifiers = {};
        p.returnType = "";
        p.parameterTypes = {};
        p.isConstructor = false;
        p.isDestructor = false;
        p.isStatic = false;
        p.isVirtual = false;
        return p;
    }
};

TEST_F(SymbolManglerTest, SimpleFunctionMangling) {
    auto params = makeParams();
    params.functionName = "foo";
    params.returnType = "int64";
    params.parameterTypes = {"int64"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_foo_i8_i8");
}

TEST_F(SymbolManglerTest, FunctionWithMultipleParameters) {
    auto params = makeParams();
    params.functionName = "add";
    params.returnType = "int64";
    params.parameterTypes = {"int64", "int64"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_add_i8_i8_i8");
}

TEST_F(SymbolManglerTest, FunctionOverloading) {
    auto params1 = makeParams();
    params1.functionName = "foo";
    params1.returnType = "int64";
    params1.parameterTypes = {"string"};

    auto params2 = makeParams();
    params2.functionName = "foo";
    params2.returnType = "int64";
    params2.parameterTypes = {"int64"};

    auto params3 = makeParams();
    params3.functionName = "foo";
    params3.returnType = "int64";
    params3.parameterTypes = {"string", "int64"};

    std::string m1 = SymbolMangler::mangleFunctionName(params1);
    std::string m2 = SymbolMangler::mangleFunctionName(params2);
    std::string m3 = SymbolMangler::mangleFunctionName(params3);

    EXPECT_EQ(m1, "_F_foo_i8_s");
    EXPECT_EQ(m2, "_F_foo_i8_i8");
    EXPECT_EQ(m3, "_F_foo_i8_s_i8");

    EXPECT_NE(m1, m2);
    EXPECT_NE(m2, m3);
    EXPECT_NE(m1, m3);
}

TEST_F(SymbolManglerTest, MemberFunctionMangling) {
    auto params = makeParams();
    params.className = "Person";
    params.functionName = "greet";
    params.returnType = "string";
    params.parameterTypes = {};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Person_greet_s");
}

TEST_F(SymbolManglerTest, MemberFunctionWithParameter) {
    auto params = makeParams();
    params.className = "Person";
    params.functionName = "setName";
    params.returnType = "void";
    params.parameterTypes = {"string"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Person_setName_v_s");
}

TEST_F(SymbolManglerTest, ConstructorMangling) {
    auto params = makeParams();
    params.className = "Person";
    params.isConstructor = true;
    params.parameterTypes = {"string"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Person_CT_s");
}

TEST_F(SymbolManglerTest, DestructorMangling) {
    auto params = makeParams();
    params.className = "Person";
    params.isDestructor = true;

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Person_DT");
}

TEST_F(SymbolManglerTest, InheritanceMangling) {
    auto params = makeParams();
    params.className = "Student";
    params.baseClassName = "Person";
    params.functionName = "study";
    params.returnType = "void";

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Student_Person_study_v");
}

TEST_F(SymbolManglerTest, ClassModifiersMangling) {
    auto params1 = makeParams();
    params1.className = "Config";
    params1.classModifiers = {"SINGLETON"};
    params1.functionName = "getInstance";
    params1.returnType = "string";

    auto params2 = makeParams();
    params2.className = "Cache";
    params2.classModifiers = {"IMMUTABLE"};
    params2.functionName = "getValue";
    params2.returnType = "int64";

    std::string m1 = SymbolMangler::mangleFunctionName(params1);
    std::string m2 = SymbolMangler::mangleFunctionName(params2);

    EXPECT_EQ(m1, "_F_Config_N_getInstance_s");
    EXPECT_EQ(m2, "_F_Cache_I_getValue_i8");
}

TEST_F(SymbolManglerTest, FunctionModifiersMangling) {
    auto params1 = makeParams();
    params1.className = "Person";
    params1.functionName = "save";
    params1.returnType = "void";
    params1.functionModifiers = {"PUBLIC"};

    auto params2 = makeParams();
    params2.className = "Person";
    params2.functionName = "save";
    params2.returnType = "void";
    params2.functionModifiers = {"PRIVATE"};

    std::string m1 = SymbolMangler::mangleFunctionName(params1);
    std::string m2 = SymbolMangler::mangleFunctionName(params2);

    EXPECT_EQ(m1, "_F_Person_save_Pb_v");
    EXPECT_EQ(m2, "_F_Person_save_Pv_v");
}

TEST_F(SymbolManglerTest, StaticFunctionMangling) {
    auto params = makeParams();
    params.className = "Math";
    params.functionName = "max";
    params.isStatic = true;
    params.returnType = "int64";
    params.parameterTypes = {"int64", "int64"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Math_max_static_i8_i8_i8");
}

TEST_F(SymbolManglerTest, VirtualFunctionMangling) {
    auto params = makeParams();
    params.className = "Shape";
    params.functionName = "draw";
    params.isVirtual = true;
    params.returnType = "void";

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Shape_draw_virtual_v");
}

TEST_F(SymbolManglerTest, FullInheritanceChainMangling) {
    auto params = makeParams();
    params.className = "Student";
    params.baseClassName = "Person";
    params.classModifiers = {"IMMUTABLE"};
    params.functionName = "study";
    params.returnType = "void";
    params.parameterTypes = {"string"};
    params.functionModifiers = {"PUBLIC"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_Student_Person_I_study_Pb_v_s");
}

TEST_F(SymbolManglerTest, ModuleSymbolMangling) {
    std::string mangled = SymbolMangler::mangleModuleSymbol({"hoo", "io"}, "print");
    EXPECT_EQ(mangled, "_H_hoo_io_print");

    std::string mangled2 = SymbolMangler::mangleModuleSymbol({"hoo", "collections", "List"}, "first");
    EXPECT_EQ(mangled2, "_H_hoo_collections_List_first");
}

TEST_F(SymbolManglerTest, TypeMangling) {
    EXPECT_EQ(SymbolMangler::mangleType("int64"), "i8");
    EXPECT_EQ(SymbolMangler::mangleType("int"), "i8");
    EXPECT_EQ(SymbolMangler::mangleType("string"), "s");
    EXPECT_EQ(SymbolMangler::mangleType("bool"), "b");
    EXPECT_EQ(SymbolMangler::mangleType("double"), "d");
    EXPECT_EQ(SymbolMangler::mangleType("f64"), "d");
    EXPECT_EQ(SymbolMangler::mangleType("f8"), "e");
    EXPECT_EQ(SymbolMangler::mangleType("float"), "f");
    EXPECT_EQ(SymbolMangler::mangleType("bit"), "x");
    EXPECT_EQ(SymbolMangler::mangleType("char"), "c");
    EXPECT_EQ(SymbolMangler::mangleType("void"), "v");
    EXPECT_EQ(SymbolMangler::mangleType("int8"), "i1");
    EXPECT_EQ(SymbolMangler::mangleType("byte"), "u1");
    EXPECT_EQ(SymbolMangler::mangleType("ptr"), "p");
    EXPECT_EQ(SymbolMangler::mangleType("tensor"), "t");
}

TEST_F(SymbolManglerTest, TypeDemangling) {
    EXPECT_EQ(SymbolMangler::demangleType("i8"), "int64");
    EXPECT_EQ(SymbolMangler::demangleType("s"), "string");
    EXPECT_EQ(SymbolMangler::demangleType("b"), "bool");
    EXPECT_EQ(SymbolMangler::demangleType("d"), "double");
    EXPECT_EQ(SymbolMangler::demangleType("e"), "f8");
    EXPECT_EQ(SymbolMangler::demangleType("f"), "float");
    EXPECT_EQ(SymbolMangler::demangleType("x"), "bit");
    EXPECT_EQ(SymbolMangler::demangleType("c"), "char");
    EXPECT_EQ(SymbolMangler::demangleType("v"), "void");
    EXPECT_EQ(SymbolMangler::demangleType("i1"), "int8");
    EXPECT_EQ(SymbolMangler::demangleType("u1"), "byte");
    EXPECT_EQ(SymbolMangler::demangleType("p"), "ptr");
    EXPECT_EQ(SymbolMangler::demangleType("t"), "tensor");
}

TEST_F(SymbolManglerTest, LowPrecisionFunctionMangling) {
    auto f8Params = makeParams();
    f8Params.functionName = "scale";
    f8Params.returnType = "f8";
    f8Params.parameterTypes = {"f8"};

    auto bitParams = makeParams();
    bitParams.functionName = "gate";
    bitParams.returnType = "bit";
    bitParams.parameterTypes = {"bit", "bit"};

    EXPECT_EQ(SymbolMangler::mangleFunctionName(f8Params), "_F_scale_e_e");
    EXPECT_EQ(SymbolMangler::mangleFunctionName(bitParams), "_F_gate_x_x_x");
}

TEST_F(SymbolManglerTest, DemanglingSimpleFunction) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_foo_i8_i8");
    EXPECT_EQ(sym.originalName, "_F_foo_i8_i8");
    EXPECT_FALSE(sym.className.empty());
}

TEST_F(SymbolManglerTest, DemanglingMemberFunction) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_Person_greet_s");
    EXPECT_EQ(sym.className, "Person");
}

TEST_F(SymbolManglerTest, DemanglingConstructor) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_Person_CT_s");
    EXPECT_EQ(sym.className, "Person");
    EXPECT_TRUE(sym.isConstructor);
}

TEST_F(SymbolManglerTest, DemanglingWithInheritance) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_Student_Person_study_v");
    EXPECT_EQ(sym.className, "Student");
    EXPECT_EQ(sym.baseClassName, "Person");
}

TEST_F(SymbolManglerTest, DemanglingWithModifiers) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_Config_N_getInstance_s");
    EXPECT_EQ(sym.className, "Config");
    EXPECT_EQ(sym.classModifiers.size(), 1);
    EXPECT_EQ(sym.classModifiers[0], "SINGLETON");
}

TEST_F(SymbolManglerTest, DemanglingWithFunctionModifiers) {
    DemangledSymbol sym = SymbolMangler::demangleSymbol("_F_Person_save_Pb_v");
    EXPECT_EQ(sym.className, "Person");
}

TEST_F(SymbolManglerTest, RoundTripFunction) {
    auto params = makeParams();
    params.functionName = "process";
    params.returnType = "string";
    params.parameterTypes = {"int64", "string"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);

    EXPECT_EQ(demangled.returnType, "string");
}

TEST_F(SymbolManglerTest, RoundTripMemberFunction) {
    auto params = makeParams();
    params.className = "User";
    params.baseClassName = "Person";
    params.functionName = "authenticate";
    params.returnType = "bool";
    params.parameterTypes = {"string", "string"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);

    EXPECT_EQ(demangled.className, "User");
    EXPECT_EQ(demangled.baseClassName, "Person");
}

TEST_F(SymbolManglerTest, EmptyParameterList) {
    auto params = makeParams();
    params.functionName = "getName";
    params.returnType = "string";
    params.parameterTypes = {};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_getName_s");

    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);
    EXPECT_EQ(demangled.parameterTypes.size(), 0);
}

TEST_F(SymbolManglerTest, AllPrimitiveTypes) {
    std::vector<std::string> types = {"int64", "bool", "char", "string", "double", "float", "void"};
    
    for (const auto& type : types) {
        std::string mangled = SymbolMangler::mangleType(type);
        std::string demangled = SymbolMangler::demangleType(mangled);
        EXPECT_EQ(demangled, type) << "Round trip failed for " << type;
    }
    
    EXPECT_EQ(SymbolMangler::mangleType("int8"), "i1");
    EXPECT_EQ(SymbolMangler::mangleType("byte"), "u1");
    EXPECT_EQ(SymbolMangler::mangleType("int"), "i8");
    EXPECT_EQ(SymbolMangler::mangleType("f64"), "d");
    EXPECT_EQ(SymbolMangler::demangleType("i1"), "int8");
    EXPECT_EQ(SymbolMangler::demangleType("u1"), "byte");
    EXPECT_EQ(SymbolMangler::demangleType("i8"), "int64");
    EXPECT_EQ(SymbolMangler::demangleType("d"), "double");
}

TEST_F(SymbolManglerTest, QualifiedReferenceTypeManglingRoundTrip) {
    std::string mangled = SymbolMangler::mangleType("foo.bar.User");
    EXPECT_NE(mangled, "o");
    EXPECT_EQ(SymbolMangler::demangleType(mangled), "foo.bar.User");
}

TEST_F(SymbolManglerTest, ArrayAndOptionalTypeManglingRoundTrip) {
    std::string mangled = SymbolMangler::mangleType("foo.bar.User[]?");
    EXPECT_NE(mangled, "o");
    EXPECT_EQ(SymbolMangler::demangleType(mangled), "foo.bar.User[]?");
}

TEST_F(SymbolManglerTest, TypeManglingIgnoresWhitespace) {
    std::string compact = SymbolMangler::mangleType("map[string,map[foo.User[],int64?]]");
    std::string spaced = SymbolMangler::mangleType(" map [ string , map [ foo.User[] , int64 ? ] ] ");
    EXPECT_EQ(compact, spaced);
}

TEST_F(SymbolManglerTest, MapTypeManglingRoundTrip) {
    std::string mangled = SymbolMangler::mangleType("map[string,int64]");
    EXPECT_NE(mangled, "o");
    EXPECT_EQ(SymbolMangler::demangleType(mangled), "map[string,int64]");
}

TEST_F(SymbolManglerTest, NestedMapArrayOptionalTypeManglingRoundTrip) {
    std::string type = "map[string,map[foo.bar.User[],int64?]]";
    std::string mangled = SymbolMangler::mangleType(type);
    EXPECT_NE(mangled, "o");
    EXPECT_EQ(SymbolMangler::demangleType(mangled), type);
}

TEST_F(SymbolManglerTest, FunctionOverloadWithReferenceTypesProducesDistinctMangles) {
    auto p1 = makeParams();
    p1.functionName = "build";
    p1.returnType = "void";
    p1.parameterTypes = {"foo.User"};

    auto p2 = makeParams();
    p2.functionName = "build";
    p2.returnType = "void";
    p2.parameterTypes = {"foo.Admin"};

    std::string m1 = SymbolMangler::mangleFunctionName(p1);
    std::string m2 = SymbolMangler::mangleFunctionName(p2);

    EXPECT_NE(m1, m2);
}

TEST_F(SymbolManglerTest, DemangleSymbolDecodesStructuredTypeSignatures) {
    auto params = makeParams();
    params.functionName = "process";
    params.returnType = "map[string,int64]";
    params.parameterTypes = {"foo.User[]?", "int64"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);

    EXPECT_EQ(demangled.returnType, "map[string,int64]");
    ASSERT_GE(demangled.parameterTypes.size(), 1U);
    EXPECT_EQ(demangled.parameterTypes[0], "foo.User[]?");
}

TEST_F(SymbolManglerTest, MalformedTypeDoesNotCollapseToGenericUnknownCode) {
    std::string malformed = "map[string,]";
    std::string mangled = SymbolMangler::mangleType(malformed);
    EXPECT_NE(mangled, "o");
    EXPECT_EQ(SymbolMangler::demangleType(mangled), malformed);
}

TEST_F(SymbolManglerTest, InvalidStructuredTypeDemanglesToUnknown) {
    EXPECT_EQ(SymbolMangler::demangleType("QabcZ"), "unknown"); // odd hex-length payload
    EXPECT_EQ(SymbolMangler::demangleType("QzzZ"), "unknown");  // non-hex payload
    EXPECT_EQ(SymbolMangler::demangleType("M"), "unknown");     // incomplete map
    EXPECT_EQ(SymbolMangler::demangleType("A"), "unknown");     // incomplete array
    EXPECT_EQ(SymbolMangler::demangleType("O"), "unknown");     // incomplete optional
}

TEST_F(SymbolManglerTest, ModuleSymbolDemangleFallsBackToOriginalName) {
    std::string mangled = SymbolMangler::mangleModuleSymbol({"pkg", "mod"}, "sym");
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);
    EXPECT_EQ(demangled.originalName, mangled);
    EXPECT_TRUE(demangled.className.empty());
    EXPECT_EQ(SymbolMangler::demangle(mangled), mangled);
}

TEST_F(SymbolManglerTest, EncodedComponentRoundTrip) {
    std::string original = "name-with.dot and space";
    std::string encoded = encodeComponent(original);
    EXPECT_NE(encoded, original);
    EXPECT_EQ(decodeComponent(encoded), original);
}

TEST_F(SymbolManglerTest, MemberFunctionNameWithSpecialCharsRoundTrip) {
    auto params = makeParams();
    params.className = "Printer";
    params.functionName = "print-value.v2";
    params.returnType = "void";
    params.parameterTypes = {"string"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);

    EXPECT_EQ(demangled.functionName, "print-value.v2");
    EXPECT_EQ(demangled.returnType, "void");
    ASSERT_EQ(demangled.parameterTypes.size(), 1U);
    EXPECT_EQ(demangled.parameterTypes[0], "string");
}

TEST_F(SymbolManglerTest, MemberAndBaseClassWithSpecialCharsRoundTrip) {
    auto params = makeParams();
    params.className = "foo.bar-User";
    params.baseClassName = "core.base-Type";
    params.functionName = "act-now";
    params.returnType = "bool";
    params.parameterTypes = {"int64"};

    std::string mangled = SymbolMangler::mangleFunctionName(params);
    DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);

    EXPECT_EQ(demangled.className, "foo.bar-User");
    EXPECT_EQ(demangled.baseClassName, "core.base-Type");
}

TEST_F(SymbolManglerTest, ModuleSymbolWithSpecialCharsManglingUsesEncoding) {
    std::string mangled = SymbolMangler::mangleModuleSymbol({"hoo.std", "io-utils"}, "print.line");
    EXPECT_NE(mangled.find("E"), std::string::npos);
}
