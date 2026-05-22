#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <fstream>
#include <cstring>
#include "hvm/HoModule.h"

using namespace hvm;

namespace {
uint64_t readU64LE(const std::vector<uint8_t>& data, size_t offset) {
    uint64_t value = 0;
    for (size_t i = 0; i < 8; ++i) {
        value |= (static_cast<uint64_t>(data[offset + i]) << (8 * i));
    }
    return value;
}

void writeU32LE(std::vector<uint8_t>& data, size_t offset, uint32_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFU);
    data[offset + 1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
}
}

class HoModuleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HoModuleTest, CreateDefaultModule) {
    auto module = HoModule::create();
    ASSERT_NE(module, nullptr);
    
    EXPECT_EQ(module->getMagic(), HoModule::MAGIC);
    EXPECT_EQ(module->getVersionMajor(), HoModule::VERSION_MAJOR);
    EXPECT_EQ(module->getVersionMinor(), HoModule::VERSION_MINOR);
    EXPECT_EQ(module->getFileType(), FileType::ObjectFile);
    EXPECT_EQ(module->getTargetArch(), TargetArch::Any);
    EXPECT_EQ(module->getEndianness(), Endianness::Little);
    EXPECT_EQ(module->getPointerSize(), 8);
    EXPECT_EQ(module->getFlags(), 0);
    EXPECT_EQ(module->getEntryPoint(), 0);
    EXPECT_EQ(module->getBaseAddress(), 0);
}

TEST_F(HoModuleTest, SetVersion) {
    auto module = HoModule::create();
    module->setVersion(2, 5);
    EXPECT_EQ(module->getVersionMajor(), 2);
    EXPECT_EQ(module->getVersionMinor(), 5);
}

TEST_F(HoModuleTest, SetFileType) {
    auto module = HoModule::create();
    module->setFileType(FileType::Executable);
    EXPECT_EQ(module->getFileType(), FileType::Executable);
    
    module->setFileType(FileType::SharedObject);
    EXPECT_EQ(module->getFileType(), FileType::SharedObject);
}

TEST_F(HoModuleTest, SetTargetArch) {
    auto module = HoModule::create();
    module->setTargetArch(TargetArch::X86_64);
    EXPECT_EQ(module->getTargetArch(), TargetArch::X86_64);
    
    module->setTargetArch(TargetArch::ARM64);
    EXPECT_EQ(module->getTargetArch(), TargetArch::ARM64);
}

TEST_F(HoModuleTest, SetEndianness) {
    auto module = HoModule::create();
    module->setEndianness(Endianness::Big);
    EXPECT_EQ(module->getEndianness(), Endianness::Big);
}

TEST_F(HoModuleTest, SetPointerSize) {
    auto module = HoModule::create();
    module->setPointerSize(4);
    EXPECT_EQ(module->getPointerSize(), 4);
}

TEST_F(HoModuleTest, SetFlags) {
    auto module = HoModule::create();
    module->setFlags(0x12345678);
    EXPECT_EQ(module->getFlags(), 0x12345678);
}

TEST_F(HoModuleTest, SetEntryPointAndBaseAddress) {
    auto module = HoModule::create();
    module->setEntryPoint(0x1000);
    module->setBaseAddress(0x400000);
    EXPECT_EQ(module->getEntryPoint(), 0x1000);
    EXPECT_EQ(module->getBaseAddress(), 0x400000);
}

TEST_F(HoModuleTest, AddAndGetSections) {
    auto module = HoModule::create();
    
    Section section;
    section.name = ".text";
    section.type = SectionType::SHT_TEXT;
    section.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    section.virtual_size = 1024;
    section.alignment = 16;
    section.data = {0x00, 0x01, 0x02, 0x03};
    
    module->addSection(std::move(section));
    
    EXPECT_EQ(module->getSectionCount(), 1);
    
    auto* sec = module->getSection(".text");
    ASSERT_NE(sec, nullptr);
    EXPECT_EQ(sec->name, ".text");
    EXPECT_EQ(sec->type, SectionType::SHT_TEXT);
    EXPECT_EQ(sec->flags, SectionFlags::ALLOC | SectionFlags::EXECUTE);
    EXPECT_EQ(sec->virtual_size, 1024);
    EXPECT_EQ(sec->alignment, 16);
    EXPECT_EQ(sec->data.size(), 4);
    
    EXPECT_EQ(module->getSection(".missing"), nullptr);
}

TEST_F(HoModuleTest, GetSectionsRef) {
    auto module = HoModule::create();
    
    Section sec1;
    sec1.name = "a";
    sec1.type = SectionType::SHT_TEXT;
    Section sec2;
    sec2.name = "b";
    sec2.type = SectionType::SHT_DATA;
    module->addSection(std::move(sec1));
    module->addSection(std::move(sec2));
    
    const auto& sections = module->getSections();
    EXPECT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0].name, "a");
    EXPECT_EQ(sections[1].name, "b");
    
    auto& mutableSections = module->getSections();
    EXPECT_EQ(mutableSections.size(), 2);
}

TEST_F(HoModuleTest, StringPoolAddAndGet) {
    auto module = HoModule::create();
    
    auto off1 = module->addString("hello");
    ASSERT_TRUE(off1.has_value());
    
    auto off2 = module->addString("world");
    ASSERT_TRUE(off2.has_value());
    EXPECT_GT(*off2, *off1);
    
    EXPECT_EQ(module->getString(*off1), "hello");
    EXPECT_EQ(module->getString(*off2), "world");
    
    const auto& pool = module->getStringPool();
    EXPECT_TRUE(pool.find("hello") != std::string::npos);
    EXPECT_TRUE(pool.find("world") != std::string::npos);
}

TEST_F(HoModuleTest, AddAndGetSymbols) {
    auto module = HoModule::create();
    
    Symbol sym;
    sym.name = "main";
    sym.binding = Symbol::STB_GLOBAL;
    sym.type = Symbol::STT_FUNC;
    sym.visibility = Symbol::STV_DEFAULT;
    sym.value = 0x1000;
    sym.size = 256;
    sym.section_index = 1;
    sym.symbol_index = 0;
    
    module->addSymbol(sym);
    
    const auto& symbols = module->getSymbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0].name, "main");
    EXPECT_EQ(symbols[0].binding, Symbol::STB_GLOBAL);
    EXPECT_EQ(symbols[0].type, Symbol::STT_FUNC);
    
    const auto* found = module->getSymbol("main");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "main");
    EXPECT_EQ(found->value, 0x1000);
    
    EXPECT_EQ(module->getSymbol("nonexistent"), nullptr);
}

TEST_F(HoModuleTest, AddAndGetRelocations) {
    auto module = HoModule::create();
    
    Relocation reloc;
    reloc.offset = 0x100;
    reloc.symbol_index = 5;
    reloc.relocation_type = 0x10;
    reloc.addend = -8;
    
    module->addRelocation(reloc);
    
    const auto& relocs = module->getRelocations();
    EXPECT_EQ(relocs.size(), 1);
    EXPECT_EQ(relocs[0].offset, 0x100);
    EXPECT_EQ(relocs[0].symbol_index, 5);
    
    auto& mutableRelocs = module->getRelocations();
    EXPECT_EQ(mutableRelocs.size(), 1);
}

TEST_F(HoModuleTest, AddAndGetExports) {
    auto module = HoModule::create();
    
    ExportEntry exp;
    exp.name = "_main";
    exp.symbol_index = 0;
    exp.address = 0x1000;
    exp.size = 256;
    
    module->addExport(exp);
    
    const auto& exports = module->getExports();
    EXPECT_EQ(exports.size(), 1);
    EXPECT_EQ(exports[0].name, "_main");
    EXPECT_EQ(exports[0].address, 0x1000);
    
    auto& mutableExports = module->getExports();
    EXPECT_EQ(mutableExports.size(), 1);
}

TEST_F(HoModuleTest, AddAndGetImports) {
    auto module = HoModule::create();
    
    ImportEntry imp;
    imp.name = "printf";
    imp.library = "libc.so.6";
    imp.import_type = ImportEntry::IT_NATIVE;
    imp.version = 1;
    imp.flags = 0;
    imp.resolved_address = 0;
    
    module->addImport(imp);
    
    const auto& imports = module->getImports();
    EXPECT_EQ(imports.size(), 1);
    EXPECT_EQ(imports[0].name, "printf");
    EXPECT_EQ(imports[0].library, "libc.so.6");
    EXPECT_EQ(imports[0].import_type, ImportEntry::IT_NATIVE);
    
    auto& mutableImports = module->getImports();
    EXPECT_EQ(mutableImports.size(), 1);
}

TEST_F(HoModuleTest, AddAndGetFunctionMetadata) {
    auto module = HoModule::create();
    
    FunctionMetadata meta;
    meta.name = "main";
    meta.symbol_index = 0;
    meta.entry_rva = 0x1000;
    meta.code_size = 256;
    meta.local_size = 32;
    meta.param_count = 0;
    meta.param_types_offset = 0;
    meta.return_type_offset = 0;
    meta.flags = 0;
    meta.source_line = 10;
    meta.debug_offset = 0;
    
    module->addFunctionMetadata(meta);
    
    const auto& metadata = module->getFunctionMetadata();
    EXPECT_EQ(metadata.size(), 1);
    EXPECT_EQ(metadata[0].name, "main");
    EXPECT_EQ(metadata[0].code_size, 256);
    
    auto& mutableMeta = module->getFunctionMetadata();
    EXPECT_EQ(mutableMeta.size(), 1);
}

TEST_F(HoModuleTest, FlagsAccessors) {
    auto module = HoModule::create();
    
    module->setFlags(0);
    EXPECT_FALSE(module->hasDebugInfo());
    EXPECT_FALSE(module->hasTypeInfo());
    EXPECT_FALSE(module->isStripped());
    EXPECT_FALSE(module->isPIE());
    EXPECT_EQ(module->getOptimizationLevel(), 0);
    
    module->setDebugInfo(true);
    EXPECT_TRUE(module->hasDebugInfo());
    EXPECT_FALSE(module->hasTypeInfo());
    
    module->setTypeInfo(true);
    EXPECT_TRUE(module->hasTypeInfo());
    
    module->setStripped(true);
    EXPECT_TRUE(module->isStripped());
    
    module->setPIE(true);
    EXPECT_TRUE(module->isPIE());
    
    module->setOptimizationLevel(3);
    EXPECT_EQ(module->getOptimizationLevel(), 3);
}

TEST_F(HoModuleTest, ErrorHandling) {
    auto module = HoModule::create();
    
    EXPECT_FALSE(module->hasError());
    EXPECT_EQ(module->getError(), "");
    
    module->clearError();
    EXPECT_FALSE(module->hasError());
}

TEST_F(HoModuleTest, SerializeBasic) {
    auto module = HoModule::create();
    module->setFileType(FileType::Executable);
    module->setEntryPoint(0x1000);
    
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.virtual_size = 100;
    text.alignment = 16;
    text.data.resize(100, 0xCC);
    module->addSection(std::move(text));
    
    std::vector<uint8_t> output;
    ASSERT_TRUE(module->serialize(output));
    
    EXPECT_GE(output.size(), HoModule::HEADER_SIZE);
    
    EXPECT_EQ(output[0], 'C');
    EXPECT_EQ(output[1], 'O');
    EXPECT_EQ(output[2], 'O');
    EXPECT_EQ(output[3], 'H');
}

TEST_F(HoModuleTest, SerializeAndParse) {
    auto module = HoModule::create();
    module->setFileType(FileType::Executable);
    module->setTargetArch(TargetArch::X86_64);
    module->setEndianness(Endianness::Little);
    module->setPointerSize(8);
    module->setEntryPoint(0x1234);
    module->setBaseAddress(0x400000);

    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.virtual_size = 8;
    text.alignment = 16;
    text.data = {0x10, 0x20, 0x30, 0x40, 0xAA, 0xBB, 0xCC, 0xDD};
    module->addSection(std::move(text));

    Symbol sym;
    sym.name = "entry";
    sym.binding = Symbol::STB_GLOBAL;
    sym.type = Symbol::STT_FUNC;
    sym.visibility = Symbol::STV_DEFAULT;
    sym.reserved = 0;
    sym.value = 0x1234;
    sym.size = 8;
    sym.section_index = 0;
    sym.symbol_index = 0;
    module->addSymbol(sym);

    Relocation reloc;
    reloc.offset = 4;
    reloc.symbol_index = 0;
    reloc.relocation_type = 1;
    reloc.addend = -4;
    module->addRelocation(reloc);

    ExportEntry exp;
    exp.name = "entry";
    exp.symbol_index = 0;
    exp.address = 0x1234;
    exp.size = 8;
    module->addExport(exp);

    ImportEntry imp;
    imp.name = "printf";
    imp.library = "libc";
    imp.import_type = ImportEntry::IT_NATIVE;
    imp.version = 2;
    imp.flags = 7;
    imp.resolved_address = 0;
    module->addImport(imp);

    FunctionMetadata meta;
    meta.name = "entry";
    meta.symbol_index = 0;
    meta.entry_rva = 0x1234;
    meta.code_size = 8;
    meta.local_size = 16;
    meta.param_count = 1;
    meta.param_types_offset = 11;
    meta.return_type_offset = 22;
    meta.flags = 33;
    meta.source_line = 44;
    meta.debug_offset = 55;
    module->addFunctionMetadata(meta);

    std::vector<uint8_t> output;
    ASSERT_TRUE(module->serialize(output));

    auto parsed = HoModule::parse(output);
    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->getFileType(), FileType::Executable);
    EXPECT_EQ(parsed->getTargetArch(), TargetArch::X86_64);
    EXPECT_EQ(parsed->getEntryPoint(), 0x1234U);
    EXPECT_EQ(parsed->getBaseAddress(), 0x400000U);

    const auto* textSec = parsed->getSection(".text");
    ASSERT_NE(textSec, nullptr);
    EXPECT_EQ(textSec->data, std::vector<uint8_t>({0x10, 0x20, 0x30, 0x40, 0xAA, 0xBB, 0xCC, 0xDD}));

    ASSERT_EQ(parsed->getSymbols().size(), 1U);
    EXPECT_EQ(parsed->getSymbols()[0].name, "entry");
    ASSERT_EQ(parsed->getRelocations().size(), 1U);
    EXPECT_EQ(parsed->getRelocations()[0].addend, -4);
    ASSERT_EQ(parsed->getExports().size(), 1U);
    EXPECT_EQ(parsed->getExports()[0].name, "entry");
    ASSERT_EQ(parsed->getImports().size(), 1U);
    EXPECT_EQ(parsed->getImports()[0].library, "libc");
    ASSERT_EQ(parsed->getFunctionMetadata().size(), 1U);
    EXPECT_EQ(parsed->getFunctionMetadata()[0].source_line, 44U);
}

TEST_F(HoModuleTest, ParseWithSymbols) {
    auto module = HoModule::create();
    
    Symbol sym;
    sym.name = "test_func";
    sym.binding = Symbol::STB_GLOBAL;
    sym.type = Symbol::STT_FUNC;
    sym.value = 0x1000;
    sym.size = 100;
    sym.section_index = 1;
    sym.symbol_index = 0;
    module->addSymbol(sym);
    
    const auto& syms = module->getSymbols();
    EXPECT_EQ(syms.size(), 1);
    EXPECT_EQ(syms[0].name, "test_func");
    
    const auto* found = module->getSymbol("test_func");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 0x1000);
}

TEST_F(HoModuleTest, ParseWithExports) {
    auto module = HoModule::create();
    
    ExportEntry exp;
    exp.name = "main";
    exp.symbol_index = 0;
    exp.address = 0x1000;
    exp.size = 256;
    module->addExport(exp);
    
    const auto& exports = module->getExports();
    EXPECT_EQ(exports.size(), 1);
    EXPECT_EQ(exports[0].name, "main");
    EXPECT_EQ(exports[0].address, 0x1000);
}

TEST_F(HoModuleTest, ParseWithImports) {
    auto module = HoModule::create();
    
    ImportEntry imp;
    imp.name = "malloc";
    imp.library = "libc.so";
    imp.import_type = ImportEntry::IT_NATIVE;
    imp.version = 1;
    module->addImport(imp);
    
    const auto& imports = module->getImports();
    EXPECT_EQ(imports.size(), 1);
    EXPECT_EQ(imports[0].name, "malloc");
    EXPECT_EQ(imports[0].library, "libc.so");
}

TEST_F(HoModuleTest, ParseWithFunctionMetadata) {
    auto module = HoModule::create();
    
    FunctionMetadata meta;
    meta.name = "test";
    meta.entry_rva = 0x1000;
    meta.code_size = 256;
    meta.local_size = 32;
    meta.param_count = 2;
    meta.source_line = 10;
    module->addFunctionMetadata(meta);
    
    const auto& metaList = module->getFunctionMetadata();
    EXPECT_EQ(metaList.size(), 1);
    EXPECT_EQ(metaList[0].name, "test");
    EXPECT_EQ(metaList[0].code_size, 256);
}

TEST_F(HoModuleTest, SerializeToFilePath) {
    auto module = HoModule::create();
    module->setFileType(FileType::Executable);
    
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.virtual_size = 16;
    text.data.resize(16, 0);
    module->addSection(std::move(text));
    
    std::string tempPath = "/tmp/hvm_test_module.bin";
    ASSERT_TRUE(module->serialize(tempPath));
    
    auto parsed = HoModule::parse(tempPath);
    ASSERT_NE(parsed, nullptr);
    EXPECT_GE(parsed->getSectionCount(), 1U);
    EXPECT_NE(parsed->getSection(".text"), nullptr);
}

TEST_F(HoModuleTest, ParseInvalidMagic) {
    std::vector<uint8_t> data(64, 0);
    *reinterpret_cast<uint32_t*>(data.data()) = 0xDEADBEEF;
    
    auto result = HoModule::parse(data);
    ASSERT_EQ(result, nullptr);
}

TEST_F(HoModuleTest, ParseTooSmall) {
    std::vector<uint8_t> data(10, 0);
    
    auto result = HoModule::parse(data);
    ASSERT_EQ(result, nullptr);
}

TEST_F(HoModuleTest, SerializeRejectsBigEndian) {
    auto module = HoModule::create();
    module->setEndianness(Endianness::Big);
    std::vector<uint8_t> output;
    EXPECT_FALSE(module->serialize(output));
    EXPECT_TRUE(module->hasError());
}

TEST_F(HoModuleTest, SerializeRejectsUserSuppliedMetadataSectionPayload) {
    auto module = HoModule::create();
    Section symtab;
    symtab.name = ".symtab";
    symtab.type = SectionType::SHT_SYMTAB;
    symtab.virtual_size = 4;
    symtab.data = {0xAA, 0xBB, 0xCC, 0xDD};
    module->addSection(std::move(symtab));

    std::vector<uint8_t> output;
    EXPECT_FALSE(module->serialize(output));
    EXPECT_TRUE(module->hasError());
    EXPECT_NE(module->getError().find("User-supplied metadata section payload"), std::string::npos);
}

TEST_F(HoModuleTest, ParseRejectsBigEndianHeader) {
    auto module = HoModule::create();
    Section sec;
    sec.name = ".text";
    sec.type = SectionType::SHT_TEXT;
    sec.virtual_size = 4;
    sec.data = {1, 2, 3, 4};
    module->addSection(std::move(sec));

    std::vector<uint8_t> output;
    ASSERT_TRUE(module->serialize(output));
    ASSERT_GE(output.size(), HoModule::HEADER_SIZE);
    output[0x0A] = static_cast<uint8_t>(Endianness::Big);

    auto parsed = HoModule::parse(output);
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(HoModuleTest, DeserializePreservesDetailedParseError) {
    auto module = HoModule::create();
    std::vector<uint8_t> data(HoModule::HEADER_SIZE, 0);
    data[0] = 0xEF;
    data[1] = 0xBE;
    data[2] = 0xAD;
    data[3] = 0xDE;

    EXPECT_FALSE(module->deserialize(data));
    EXPECT_TRUE(module->hasError());
    EXPECT_EQ(module->getError(), "Invalid magic number");
}

TEST_F(HoModuleTest, DeserializeRejectsOverflowedSectionTableCount) {
    auto module = HoModule::create();
    std::vector<uint8_t> data(HoModule::HEADER_SIZE, 0);
    data[0] = 'C';
    data[1] = 'O';
    data[2] = 'O';
    data[3] = 'H';
    data[4] = 1;
    data[5] = 0;
    data[6] = 3;
    data[7] = 0;
    data[8] = static_cast<uint8_t>(FileType::ObjectFile);
    data[9] = static_cast<uint8_t>(TargetArch::Any);
    data[10] = static_cast<uint8_t>(Endianness::Little);
    data[11] = 8;

    // section_count = UINT64_MAX (LE)
    for (size_t i = 0; i < 8; ++i) {
        data[0x20 + i] = 0xFF;
    }

    EXPECT_FALSE(module->deserialize(data));
    EXPECT_TRUE(module->hasError());
    EXPECT_EQ(module->getError(), "Invalid section table size");
}

TEST_F(HoModuleTest, SerializePathReportsOpenFailureError) {
    auto module = HoModule::create();
    std::string invalidPath = "/definitely/nonexistent/path/module.ho";
    EXPECT_FALSE(module->serialize(invalidPath));
    EXPECT_TRUE(module->hasError());
    EXPECT_NE(module->getError().find("Cannot open file for writing"), std::string::npos);
}

TEST_F(HoModuleTest, SerializeFilePointerReportsNullError) {
    auto module = HoModule::create();
    EXPECT_FALSE(module->serialize(static_cast<FILE*>(nullptr)));
    EXPECT_TRUE(module->hasError());
    EXPECT_EQ(module->getError(), "Cannot write to null FILE*");
}

TEST_F(HoModuleTest, ParseNullFilePointerReturnsNull) {
    auto parsed = HoModule::parse(static_cast<FILE*>(nullptr));
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(HoModuleTest, DeserializePreservesDeepParseErrorForInvalidSymtabSize) {
    auto source = HoModule::create();
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.virtual_size = 4;
    text.data = {1, 2, 3, 4};
    source->addSection(std::move(text));

    Symbol sym;
    sym.name = "foo";
    sym.binding = Symbol::STB_GLOBAL;
    sym.type = Symbol::STT_FUNC;
    sym.visibility = Symbol::STV_DEFAULT;
    sym.reserved = 0;
    sym.value = 1;
    sym.size = 2;
    sym.section_index = 0;
    sym.symbol_index = 0;
    source->addSymbol(sym);

    std::vector<uint8_t> data;
    ASSERT_TRUE(source->serialize(data));

    const uint64_t sectionCount = readU64LE(data, 0x20);
    const size_t tableStart = HoModule::HEADER_SIZE;
    bool patched = false;
    for (size_t i = 0; i < sectionCount; ++i) {
        const size_t entry = tableStart + i * 40;
        const uint32_t type = static_cast<uint32_t>(data[entry + 0x08]) |
                              (static_cast<uint32_t>(data[entry + 0x09]) << 8U) |
                              (static_cast<uint32_t>(data[entry + 0x0A]) << 16U) |
                              (static_cast<uint32_t>(data[entry + 0x0B]) << 24U);
        if (type == static_cast<uint32_t>(SectionType::SHT_SYMTAB)) {
            writeU32LE(data, entry + 0x10, 31);  // invalid non-multiple of 32
            writeU32LE(data, entry + 0x14, 0);
            patched = true;
            break;
        }
    }
    ASSERT_TRUE(patched);

    auto target = HoModule::create();
    EXPECT_FALSE(target->deserialize(data));
    EXPECT_EQ(target->getError(), "Invalid symbol table size");
}

TEST_F(HoModuleTest, ParseRejectsDuplicateMetadataSectionTypes) {
    auto source = HoModule::create();
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.virtual_size = 4;
    text.data = {1, 2, 3, 4};
    source->addSection(std::move(text));

    Symbol sym;
    sym.name = "foo";
    sym.binding = Symbol::STB_GLOBAL;
    sym.type = Symbol::STT_FUNC;
    sym.visibility = Symbol::STV_DEFAULT;
    sym.reserved = 0;
    sym.value = 1;
    sym.size = 2;
    sym.section_index = 0;
    sym.symbol_index = 0;
    source->addSymbol(sym);

    std::vector<uint8_t> data;
    ASSERT_TRUE(source->serialize(data));

    const uint64_t sectionCount = readU64LE(data, 0x20);
    const size_t tableStart = HoModule::HEADER_SIZE;
    bool patched = false;
    for (size_t i = 0; i < sectionCount; ++i) {
        const size_t entry = tableStart + i * 40;
        const uint32_t type = static_cast<uint32_t>(data[entry + 0x08]) |
                              (static_cast<uint32_t>(data[entry + 0x09]) << 8U) |
                              (static_cast<uint32_t>(data[entry + 0x0A]) << 16U) |
                              (static_cast<uint32_t>(data[entry + 0x0B]) << 24U);
        if (type == static_cast<uint32_t>(SectionType::SHT_TEXT)) {
            writeU32LE(data, entry + 0x08, static_cast<uint32_t>(SectionType::SHT_SYMTAB));
            patched = true;
            break;
        }
    }
    ASSERT_TRUE(patched);

    auto parsed = HoModule::parse(data);
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(HoModuleTest, ParseRejectsTruncatedSectionTable) {
    auto module = HoModule::create();
    Section sec;
    sec.name = ".data";
    sec.type = SectionType::SHT_DATA;
    sec.virtual_size = 4;
    sec.data = {1, 2, 3, 4};
    module->addSection(std::move(sec));

    std::vector<uint8_t> data;
    ASSERT_TRUE(module->serialize(data));
    ASSERT_GT(data.size(), HoModule::HEADER_SIZE);

    data.resize(HoModule::HEADER_SIZE + 1);
    auto parsed = HoModule::parse(data);
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(HoModuleTest, ParseRejectsOutOfRangeSectionData) {
    std::vector<uint8_t> data(HoModule::HEADER_SIZE + 40, 0);

    data[0] = 'C';
    data[1] = 'O';
    data[2] = 'O';
    data[3] = 'H';
    data[4] = 1;
    data[5] = 0;
    data[6] = 3;
    data[7] = 0;
    data[8] = static_cast<uint8_t>(FileType::ObjectFile);
    data[9] = static_cast<uint8_t>(TargetArch::Any);
    data[10] = static_cast<uint8_t>(Endianness::Little);
    data[11] = 8;
    data[0x20] = 1;  // section count

    // section type = SHT_TEXT
    data[HoModule::HEADER_SIZE + 0x08] = static_cast<uint8_t>(SectionType::SHT_TEXT);
    // virtual size = 0x20
    data[HoModule::HEADER_SIZE + 0x10] = 0x20;
    // file offset = past end of file
    data[HoModule::HEADER_SIZE + 0x18] = 0xF0;

    auto parsed = HoModule::parse(data);
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(HoModuleTest, SerializeDoesNotMutateStringPoolAcrossCalls) {
    auto module = HoModule::create();
    const auto initialPool = module->getStringPool();

    Section sec;
    sec.name = ".text";
    sec.type = SectionType::SHT_TEXT;
    sec.virtual_size = 4;
    sec.data = {1, 2, 3, 4};
    module->addSection(std::move(sec));

    std::vector<uint8_t> output1;
    std::vector<uint8_t> output2;
    ASSERT_TRUE(module->serialize(output1));
    ASSERT_TRUE(module->serialize(output2));
    EXPECT_EQ(output1, output2);
    EXPECT_EQ(module->getStringPool(), initialPool);
}

TEST_F(HoModuleTest, MultipleSections) {
    auto module = HoModule::create();
    
    for (int i = 0; i < 5; ++i) {
        Section sec;
        sec.name = "sec" + std::to_string(i);
        sec.type = SectionType::SHT_DATA;
        sec.virtual_size = 32 * (i + 1);
        sec.data.resize(sec.virtual_size, static_cast<uint8_t>(i));
        module->addSection(std::move(sec));
    }
    
    EXPECT_EQ(module->getSectionCount(), 5);
    
    for (int i = 0; i < 5; ++i) {
        auto* sec = module->getSection("sec" + std::to_string(i));
        ASSERT_NE(sec, nullptr);
        EXPECT_EQ(sec->virtual_size, static_cast<uint64_t>(32 * (i + 1)));
    }
}

TEST_F(HoModuleTest, EncodeDecodeInstructions) {
    auto module = HoModule::create();
    
    std::vector<HInstruction> instructions = {
        HInstruction(Opcode::NOP, OperandsR{0, 0, 0, 0}),
        HInstruction(Opcode::MOV, OperandsR{1, 2, 3, 0}),
        HInstruction(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    
    auto encoded = module->encodeInstructions(instructions);
    ASSERT_EQ(encoded.size(), 12);
    
    auto decoded = module->decodeInstructions(encoded);
    EXPECT_EQ(decoded.size(), 3);
    
    EXPECT_EQ(decoded[0].getOpcode(), Opcode::NOP);
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded[0].getOperands()));
    EXPECT_EQ(std::get<OperandsR>(decoded[0].getOperands()).rd, 0);
    
    EXPECT_EQ(decoded[1].getOpcode(), Opcode::MOV);
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded[1].getOperands()));
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rd, 1);
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rs1, 2);
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rs2, 3);
    
    EXPECT_EQ(decoded[2].getOpcode(), Opcode::RET);
}

TEST_F(HoModuleTest, EncodeDecodeExtendedInstructions) {
    auto module = HoModule::create();
    
    std::vector<HInstruction> instructions = {
        HInstruction(Opcode::NOP, OperandsR{0, 0, 0, 0}),
        HInstruction(Opcode::NOP, OperandsR{1, 2, 3, 0x1234}),
    };
    
    for (auto& inst : instructions) {
        inst.setExtended(true);
    }
    
    auto encoded = module->encodeInstructions(instructions);
    ASSERT_EQ(encoded.size(), 14);
    EXPECT_EQ(encoded[0], 0xFE);
    EXPECT_EQ(encoded[7], 0xFE);
    
    auto decoded = module->decodeInstructions(encoded, true);
    EXPECT_EQ(decoded.size(), 2);
    
    EXPECT_EQ(decoded[0].getOpcode(), Opcode::NOP);
    EXPECT_TRUE(decoded[0].isExtended());
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded[0].getOperands()));
    EXPECT_EQ(std::get<OperandsR>(decoded[0].getOperands()).rd, 0);
    
    EXPECT_EQ(decoded[1].getOpcode(), Opcode::NOP);
    EXPECT_TRUE(decoded[1].isExtended());
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded[1].getOperands()));
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rd, 1);
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rs1, 2);
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).rs2, 3);
    EXPECT_EQ(std::get<OperandsR>(decoded[1].getOperands()).func, 0x1234);
}

TEST_F(HoModuleTest, InstructionsToAssembly) {
    auto module = HoModule::create();
    
    std::vector<HInstruction> instructions = {
        HInstruction(Opcode::ADD, OperandsR{5, 10, 15, 0}),
        HInstruction(Opcode::MOVI, OperandsI{3, 5, 100}),
        HInstruction(Opcode::BEQ, OperandsB{1, 2, -50}),
    };
    
    auto assembly = module->instructionsToAssembly(instructions);
    
    EXPECT_TRUE(assembly.find("add r5, r10, r15") != std::string::npos);
    EXPECT_TRUE(assembly.find("movi") != std::string::npos);
    EXPECT_TRUE(assembly.find("beq") != std::string::npos);
}

TEST_F(HoModuleTest, ParseAssembly) {
    auto module = HoModule::create();
    
    std::string assembly = R"(
        nop
        mov r1, r2, r3
        ret
    )";
    
    auto instructions = module->parseAssembly(assembly);
    
    ASSERT_EQ(instructions.size(), 3);
    EXPECT_EQ(instructions[0].getOpcode(), Opcode::NOP);
    EXPECT_EQ(instructions[1].getOpcode(), Opcode::MOV);
    EXPECT_EQ(instructions[2].getOpcode(), Opcode::RET);
}

TEST_F(HoModuleTest, RoundTripInstructions) {
    auto module = HoModule::create();
    
    std::vector<HInstruction> originalInstructions = {
        HInstruction(Opcode::ADDI, OperandsI{1, 0, 100}),
        HInstruction(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    
    auto encoded = module->encodeInstructions(originalInstructions);
    auto decoded = module->decodeInstructions(encoded);
    
    ASSERT_EQ(decoded.size(), 2);
    EXPECT_EQ(decoded[0].getOpcode(), Opcode::ADDI);
    EXPECT_EQ(decoded[1].getOpcode(), Opcode::RET);
}
