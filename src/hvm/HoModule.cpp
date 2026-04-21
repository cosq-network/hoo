#include "hvm/HoModule.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace hvm {

HoModule::HoModule()
    : HoModuleBase(ModuleType::Compiled, "")
    , magic_(MAGIC)
    , version_major_(VERSION_MAJOR)
    , version_minor_(VERSION_MINOR)
    , file_type_(FileType::ObjectFile)
    , target_arch_(TargetArch::Any)
    , endianness_(Endianness::Little)
    , pointer_size_(8)
    , flags_(0)
    , entry_point_(0)
    , base_address_(0)
    , string_pool_("\0")
{
}

HoModule::HoModule(const std::string& name)
    : HoModuleBase(ModuleType::Compiled, name)
    , magic_(MAGIC)
    , version_major_(VERSION_MAJOR)
    , version_minor_(VERSION_MINOR)
    , file_type_(FileType::ObjectFile)
    , target_arch_(TargetArch::Any)
    , endianness_(Endianness::Little)
    , pointer_size_(8)
    , flags_(0)
    , entry_point_(0)
    , base_address_(0)
    , string_pool_("\0")
{
}

HoModule::~HoModule() = default;

std::unique_ptr<HoModule> HoModule::create() {
    return std::unique_ptr<HoModule>(new HoModule());
}

std::unique_ptr<HoModule> HoModule::create(const std::string& name) {
    return std::unique_ptr<HoModule>(new HoModule(name));
}

bool HoModule::serialize(std::vector<uint8_t>& output) const {
    output.clear();
    output.reserve(4096);

    serializeHeader(output);
    serializeSectionTable(output);
    serializeSections(output);

    return true;
}

bool HoModule::serialize(const std::string& file_path) const {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool HoModule::serialize(FILE* file) const {
    if (!file) {
        return false;
    }

    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    return fwrite(data.data(), 1, data.size(), file) == data.size();
}

bool HoModule::parseHeader(const std::vector<uint8_t>& data, size_t& offset) {
    if (data.size() < HEADER_SIZE) {
        error_ = "File too small for header";
        return false;
    }

    magic_ = *reinterpret_cast<const uint32_t*>(data.data() + 0x00);
    if (magic_ != MAGIC) {
        error_ = "Invalid magic number";
        return false;
    }

    version_major_ = *reinterpret_cast<const uint16_t*>(data.data() + 0x04);
    version_minor_ = *reinterpret_cast<const uint16_t*>(data.data() + 0x06);

    file_type_ = static_cast<FileType>(data[0x08]);
    target_arch_ = static_cast<TargetArch>(data[0x09]);
    endianness_ = static_cast<Endianness>(data[0x0A]);
    pointer_size_ = data[0x0B];

    flags_ = *reinterpret_cast<const uint32_t*>(data.data() + 0x0C);
    entry_point_ = *reinterpret_cast<const uint64_t*>(data.data() + 0x10);
    base_address_ = *reinterpret_cast<const uint64_t*>(data.data() + 0x18);

    uint64_t section_count = *reinterpret_cast<const uint64_t*>(data.data() + 0x20);

    uint64_t symtab_offset = *reinterpret_cast<const uint64_t*>(data.data() + 0x28);
    uint32_t symtab_entry_count = *reinterpret_cast<const uint32_t*>(data.data() + 0x30);
    (void)symtab_offset;
    (void)symtab_entry_count;

    offset = HEADER_SIZE + section_count * 40;
    return true;
}

bool HoModule::parseSectionTable(const std::vector<uint8_t>& data, size_t& offset) {
    uint64_t section_count = *reinterpret_cast<const uint64_t*>(data.data() + 0x20);
    sections_.reserve(section_count);

    size_t table_offset = HEADER_SIZE;
    constexpr size_t SECTION_ENTRY_SIZE = 40;

    for (uint64_t i = 0; i < section_count; ++i) {
        Section section;

        uint64_t name_offset = *reinterpret_cast<const uint64_t*>(data.data() + table_offset + 0x00);
        section.name = getString(static_cast<uint32_t>(name_offset));

        section.type = static_cast<SectionType>(*reinterpret_cast<const uint32_t*>(data.data() + table_offset + 0x08));
        section.flags = *reinterpret_cast<const uint32_t*>(data.data() + table_offset + 0x0C);
        section.virtual_size = *reinterpret_cast<const uint64_t*>(data.data() + table_offset + 0x10);
        section.file_offset = *reinterpret_cast<const uint64_t*>(data.data() + table_offset + 0x18);
        section.alignment = *reinterpret_cast<const uint64_t*>(data.data() + table_offset + 0x20);

        if (section.type != SectionType::SHT_BSS &&
            section.type != SectionType::SHT_NULL &&
            section.file_offset > 0 &&
            section.virtual_size > 0 &&
            section.file_offset + section.virtual_size <= data.size()) {
            section.data.resize(section.virtual_size);
            std::memcpy(section.data.data(), data.data() + section.file_offset, section.virtual_size);
        }

        sections_.push_back(section);
        table_offset += SECTION_ENTRY_SIZE;
    }

    offset = table_offset;
    return true;
}

bool HoModule::parseSymbols(const std::vector<uint8_t>& data, const Section& symtab) {
    size_t entry_size = 32;
    size_t count = symtab.data.size() / entry_size;
    symbols_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = symtab.data.data() + i * entry_size;

        Symbol symbol;
        symbol.name = getString(*reinterpret_cast<const uint32_t*>(entry + 0x00));
        symbol.binding = entry[0x04];
        symbol.type = entry[0x05];
        symbol.visibility = entry[0x06];
        symbol.reserved = entry[0x07];
        symbol.value = *reinterpret_cast<const uint64_t*>(entry + 0x08);
        symbol.size = *reinterpret_cast<const uint64_t*>(entry + 0x10);
        symbol.section_index = *reinterpret_cast<const int32_t*>(entry + 0x18);
        symbol.symbol_index = *reinterpret_cast<const uint32_t*>(entry + 0x1C);

        addSymbol(symbol);
    }

    return true;
}

bool HoModule::parseRelocations(const std::vector<uint8_t>& data, const Section& reloc) {
    size_t entry_size = 16;
    size_t count = reloc.data.size() / entry_size;
    relocations_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = reloc.data.data() + i * entry_size;

        Relocation relocation;
        relocation.offset = *reinterpret_cast<const uint64_t*>(entry + 0x00);
        relocation.symbol_index = *reinterpret_cast<const uint32_t*>(entry + 0x08);
        relocation.relocation_type = *reinterpret_cast<const uint16_t*>(entry + 0x0C);
        relocation.addend = *reinterpret_cast<const int16_t*>(entry + 0x0E);

        relocations_.push_back(relocation);
    }

    return true;
}

bool HoModule::parseExports(const std::vector<uint8_t>& data, const Section& exports) {
    size_t entry_size = 24;
    size_t count = exports.data.size() / entry_size;
    exports_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = exports.data.data() + i * entry_size;

        ExportEntry exp;
        exp.name = getString(*reinterpret_cast<const uint32_t*>(entry + 0x00));
        exp.symbol_index = *reinterpret_cast<const uint32_t*>(entry + 0x04);
        exp.address = *reinterpret_cast<const uint64_t*>(entry + 0x08);
        exp.size = *reinterpret_cast<const uint64_t*>(entry + 0x10);

        exports_.push_back(exp);
    }

    return true;
}

bool HoModule::parseImports(const std::vector<uint8_t>& data, const Section& imports) {
    size_t entry_size = 32;
    size_t count = imports.data.size() / entry_size;
    imports_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = imports.data.data() + i * entry_size;

        ImportEntry imp;
        imp.name = getString(*reinterpret_cast<const uint32_t*>(entry + 0x00));
        imp.library = getString(*reinterpret_cast<const uint32_t*>(entry + 0x04));
        imp.import_type = *reinterpret_cast<const uint32_t*>(entry + 0x08);
        imp.version = *reinterpret_cast<const uint32_t*>(entry + 0x0C);
        imp.flags = *reinterpret_cast<const uint64_t*>(entry + 0x10);
        imp.resolved_address = *reinterpret_cast<const uint64_t*>(entry + 0x18);

        imports_.push_back(imp);
    }

    return true;
}

bool HoModule::parseFunctionMetadata(const std::vector<uint8_t>& data, const Section& funcmeta) {
    size_t entry_size = 48;
    size_t count = funcmeta.data.size() / entry_size;
    function_metadata_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = funcmeta.data.data() + i * entry_size;

        FunctionMetadata meta;
        meta.name = getString(*reinterpret_cast<const uint32_t*>(entry + 0x00));
        meta.symbol_index = *reinterpret_cast<const uint32_t*>(entry + 0x04);
        meta.entry_rva = *reinterpret_cast<const uint64_t*>(entry + 0x08);
        meta.code_size = *reinterpret_cast<const uint32_t*>(entry + 0x10);
        meta.local_size = *reinterpret_cast<const uint32_t*>(entry + 0x14);
        meta.param_count = *reinterpret_cast<const uint32_t*>(entry + 0x18);
        meta.param_types_offset = *reinterpret_cast<const uint32_t*>(entry + 0x1C);
        meta.return_type_offset = *reinterpret_cast<const uint32_t*>(entry + 0x20);
        meta.flags = *reinterpret_cast<const uint32_t*>(entry + 0x24);
        meta.source_line = *reinterpret_cast<const uint32_t*>(entry + 0x28);
        meta.debug_offset = *reinterpret_cast<const uint32_t*>(entry + 0x2C);

        function_metadata_.push_back(meta);
    }

    return true;
}

void HoModule::serializeHeader(std::vector<uint8_t>& output) const {
    output.resize(HEADER_SIZE, 0);

    *reinterpret_cast<uint32_t*>(output.data() + 0x00) = magic_;
    *reinterpret_cast<uint16_t*>(output.data() + 0x04) = version_major_;
    *reinterpret_cast<uint16_t*>(output.data() + 0x06) = version_minor_;
    output[0x08] = static_cast<uint8_t>(file_type_);
    output[0x09] = static_cast<uint8_t>(target_arch_);
    output[0x0A] = static_cast<uint8_t>(endianness_);
    output[0x0B] = pointer_size_;
    *reinterpret_cast<uint32_t*>(output.data() + 0x0C) = flags_;
    *reinterpret_cast<uint64_t*>(output.data() + 0x10) = entry_point_;
    *reinterpret_cast<uint64_t*>(output.data() + 0x18) = base_address_;
    *reinterpret_cast<uint64_t*>(output.data() + 0x20) = sections_.size();
    *reinterpret_cast<uint64_t*>(output.data() + 0x28) = sections_.empty() ? 0 : 64 + sections_.size() * 40;
    *reinterpret_cast<uint32_t*>(output.data() + 0x30) = static_cast<uint32_t>(symbols_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x34) = static_cast<uint32_t>(relocations_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x38) = static_cast<uint32_t>(exports_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x3C) = static_cast<uint32_t>(imports_.size());
}

void HoModule::serializeSectionTable(std::vector<uint8_t>& output) const {
    size_t table_start = output.size();
    constexpr size_t SECTION_ENTRY_SIZE = 40;
    output.resize(table_start + sections_.size() * SECTION_ENTRY_SIZE);

    size_t data_offset = HEADER_SIZE + sections_.size() * SECTION_ENTRY_SIZE;

    for (size_t i = 0; i < sections_.size(); ++i) {
        const Section& sec = sections_[i];
        uint64_t name_offset = addString(sec.name);
        size_t entry_offset = table_start + i * SECTION_ENTRY_SIZE;

        *reinterpret_cast<uint64_t*>(output.data() + entry_offset + 0x00) = name_offset;
        *reinterpret_cast<uint32_t*>(output.data() + entry_offset + 0x08) = static_cast<uint32_t>(sec.type);
        *reinterpret_cast<uint32_t*>(output.data() + entry_offset + 0x0C) = sec.flags;
        *reinterpret_cast<uint64_t*>(output.data() + entry_offset + 0x10) = sec.virtual_size;

        if (sec.type != SectionType::SHT_BSS && !sec.data.empty()) {
            *reinterpret_cast<uint64_t*>(output.data() + entry_offset + 0x18) = data_offset;
            data_offset += (sec.virtual_size + 7) & ~7ULL;
        } else {
            *reinterpret_cast<uint64_t*>(output.data() + entry_offset + 0x18) = 0;
        }

        *reinterpret_cast<uint64_t*>(output.data() + entry_offset + 0x20) = sec.alignment;
    }
}

void HoModule::serializeSections(std::vector<uint8_t>& output) const {
    for (const Section& sec : sections_) {
        if (sec.type == SectionType::SHT_BSS || sec.data.empty()) {
            continue;
        }

        size_t current_size = output.size();
        size_t aligned_size = (sec.virtual_size + 7) & ~7ULL;
        output.resize(current_size + aligned_size, 0);
        std::memcpy(output.data() + current_size, sec.data.data(), sec.virtual_size);
    }
}

void HoModule::serializeSymbols(std::vector<uint8_t>& output) const {
    for (const Symbol& sym : symbols_) {
        output.resize(output.size() + 32, 0);
        uint8_t* entry = output.data() + output.size() - 32;

        *reinterpret_cast<uint32_t*>(entry + 0x00) = addString(sym.name);
        entry[0x04] = sym.binding;
        entry[0x05] = sym.type;
        entry[0x06] = sym.visibility;
        entry[0x07] = sym.reserved;
        *reinterpret_cast<uint64_t*>(entry + 0x08) = sym.value;
        *reinterpret_cast<uint64_t*>(entry + 0x10) = sym.size;
        *reinterpret_cast<int32_t*>(entry + 0x18) = sym.section_index;
        *reinterpret_cast<uint32_t*>(entry + 0x1C) = sym.symbol_index;
    }
}

void HoModule::serializeRelocations(std::vector<uint8_t>& output) const {
    for (const Relocation& rel : relocations_) {
        output.resize(output.size() + 16, 0);
        uint8_t* entry = output.data() + output.size() - 16;

        *reinterpret_cast<uint64_t*>(entry + 0x00) = rel.offset;
        *reinterpret_cast<uint32_t*>(entry + 0x08) = rel.symbol_index;
        *reinterpret_cast<uint16_t*>(entry + 0x0C) = rel.relocation_type;
        *reinterpret_cast<int16_t*>(entry + 0x0E) = rel.addend;
    }
}

void HoModule::serializeExports(std::vector<uint8_t>& output) const {
    for (const ExportEntry& exp : exports_) {
        output.resize(output.size() + 24, 0);
        uint8_t* entry = output.data() + output.size() - 24;

        *reinterpret_cast<uint32_t*>(entry + 0x00) = addString(exp.name);
        *reinterpret_cast<uint32_t*>(entry + 0x04) = exp.symbol_index;
        *reinterpret_cast<uint64_t*>(entry + 0x08) = exp.address;
        *reinterpret_cast<uint64_t*>(entry + 0x10) = exp.size;
    }
}

void HoModule::serializeImports(std::vector<uint8_t>& output) const {
    for (const ImportEntry& imp : imports_) {
        output.resize(output.size() + 32, 0);
        uint8_t* entry = output.data() + output.size() - 32;

        *reinterpret_cast<uint32_t*>(entry + 0x00) = addString(imp.name);
        *reinterpret_cast<uint32_t*>(entry + 0x04) = addString(imp.library);
        *reinterpret_cast<uint32_t*>(entry + 0x08) = imp.import_type;
        *reinterpret_cast<uint32_t*>(entry + 0x0C) = imp.version;
        *reinterpret_cast<uint64_t*>(entry + 0x10) = imp.flags;
        *reinterpret_cast<uint64_t*>(entry + 0x18) = imp.resolved_address;
    }
}

void HoModule::serializeFunctionMetadata(std::vector<uint8_t>& output) const {
    for (const FunctionMetadata& meta : function_metadata_) {
        output.resize(output.size() + 48, 0);
        uint8_t* entry = output.data() + output.size() - 48;

        *reinterpret_cast<uint32_t*>(entry + 0x00) = addString(meta.name);
        *reinterpret_cast<uint32_t*>(entry + 0x04) = meta.symbol_index;
        *reinterpret_cast<uint64_t*>(entry + 0x08) = meta.entry_rva;
        *reinterpret_cast<uint32_t*>(entry + 0x10) = meta.code_size;
        *reinterpret_cast<uint32_t*>(entry + 0x14) = meta.local_size;
        *reinterpret_cast<uint32_t*>(entry + 0x18) = meta.param_count;
        *reinterpret_cast<uint32_t*>(entry + 0x1C) = meta.param_types_offset;
        *reinterpret_cast<uint32_t*>(entry + 0x20) = meta.return_type_offset;
        *reinterpret_cast<uint32_t*>(entry + 0x24) = meta.flags;
        *reinterpret_cast<uint32_t*>(entry + 0x28) = meta.source_line;
        *reinterpret_cast<uint32_t*>(entry + 0x2C) = meta.debug_offset;
    }
}

void HoModule::setMagic(uint32_t magic) { magic_ = magic; }
uint32_t HoModule::getMagic() const { return magic_; }

void HoModule::setVersion(uint16_t major, uint16_t minor) {
    version_major_ = major;
    version_minor_ = minor;
}
uint16_t HoModule::getVersionMajor() const { return version_major_; }
uint16_t HoModule::getVersionMinor() const { return version_minor_; }

void HoModule::setFileType(FileType type) { file_type_ = type; }
FileType HoModule::getFileType() const { return file_type_; }

void HoModule::setTargetArch(TargetArch arch) { target_arch_ = arch; }
TargetArch HoModule::getTargetArch() const { return target_arch_; }

void HoModule::setEndianness(Endianness endian) { endianness_ = endian; }
Endianness HoModule::getEndianness() const { return endianness_; }

void HoModule::setPointerSize(uint8_t size) { pointer_size_ = size; }
uint8_t HoModule::getPointerSize() const { return pointer_size_; }

void HoModule::setFlags(uint32_t flags) { flags_ = flags; }
uint32_t HoModule::getFlags() const { return flags_; }

void HoModule::setEntryPoint(uint64_t rva) { entry_point_ = rva; }
uint64_t HoModule::getEntryPoint() const { return entry_point_; }

void HoModule::setBaseAddress(uint64_t addr) { base_address_ = addr; }
uint64_t HoModule::getBaseAddress() const { return base_address_; }

void HoModule::addSection(Section section) { sections_.push_back(std::move(section)); }
Section* HoModule::getSection(const std::string& name) {
    for (auto& sec : sections_) {
        if (sec.name == name) return &sec;
    }
    return nullptr;
}
const Section* HoModule::getSection(const std::string& name) const {
    for (const auto& sec : sections_) {
        if (sec.name == name) return &sec;
    }
    return nullptr;
}
const std::vector<Section>& HoModule::getSections() const { return sections_; }
std::vector<Section>& HoModule::getSections() { return sections_; }
size_t HoModule::getSectionCount() const { return sections_.size(); }

uint32_t HoModule::addString(const std::string& str) const {
    uint32_t offset = static_cast<uint32_t>(string_pool_.size());
    string_pool_ += str;
    string_pool_ += '\0';
    return offset;
}
std::string HoModule::getString(uint32_t offset) const {
    if (offset >= string_pool_.size()) return "";
    return string_pool_.c_str() + offset;
}
const std::string& HoModule::getStringPool() const { return string_pool_; }

void HoModule::addSymbol(const Symbol& symbol) { symbols_.push_back(symbol); }
const std::vector<Symbol>& HoModule::getSymbols() const { return symbols_; }
std::vector<Symbol>& HoModule::getSymbols() { return symbols_; }
const Symbol* HoModule::getSymbol(const std::string& name) const {
    for (const auto& sym : symbols_) {
        if (sym.name == name) return &sym;
    }
    return nullptr;
}

void HoModule::addRelocation(const Relocation& reloc) { relocations_.push_back(reloc); }
const std::vector<Relocation>& HoModule::getRelocations() const { return relocations_; }
std::vector<Relocation>& HoModule::getRelocations() { return relocations_; }

void HoModule::addExport(const ExportEntry& exp) { exports_.push_back(exp); }
const std::vector<ExportEntry>& HoModule::getExports() const { return exports_; }
std::vector<ExportEntry>& HoModule::getExports() { return exports_; }

void HoModule::addImport(const ImportEntry& imp) { imports_.push_back(imp); }
const std::vector<ImportEntry>& HoModule::getImports() const { return imports_; }
std::vector<ImportEntry>& HoModule::getImports() { return imports_; }

void HoModule::addFunctionMetadata(const FunctionMetadata& meta) { function_metadata_.push_back(meta); }
const std::vector<FunctionMetadata>& HoModule::getFunctionMetadata() const { return function_metadata_; }
std::vector<FunctionMetadata>& HoModule::getFunctionMetadata() { return function_metadata_; }

bool HoModule::hasDebugInfo() const { return (flags_ & 0x8000) != 0; }
bool HoModule::hasTypeInfo() const { return (flags_ & 0x4000) != 0; }
bool HoModule::isStripped() const { return (flags_ & 0x2000) != 0; }
bool HoModule::isPIE() const { return (flags_ & 0x1000) != 0; }
uint8_t HoModule::getOptimizationLevel() const { return static_cast<uint8_t>((flags_ >> 8) & 0x0F); }

void HoModule::setDebugInfo(bool has) { flags_ = (flags_ & ~0x8000) | (has ? 0x8000 : 0); }
void HoModule::setTypeInfo(bool has) { flags_ = (flags_ & ~0x4000) | (has ? 0x4000 : 0); }
void HoModule::setStripped(bool stripped) { flags_ = (flags_ & ~0x2000) | (stripped ? 0x2000 : 0); }
void HoModule::setPIE(bool pie) { flags_ = (flags_ & ~0x1000) | (pie ? 0x1000 : 0); }
void HoModule::setOptimizationLevel(uint8_t level) { flags_ = (flags_ & ~0x0F00) | ((level & 0x0F) << 8); }

std::vector<uint8_t> HoModule::encodeInstructions(const std::vector<HInstruction>& instructions) const {
    std::vector<uint8_t> encoded;
    encoded.reserve(instructions.size() * 8);

    for (const auto& inst : instructions) {
        std::vector<uint8_t> bytes;
        if (inst.isExtended()) {
            bytes = inst.encode64();
        } else {
            bytes = inst.encode();
        }
        encoded.insert(encoded.end(), bytes.begin(), bytes.end());
    }

    return encoded;
}

std::vector<HInstruction> HoModule::decodeInstructions(const std::vector<uint8_t>& data, bool extended) const {
    std::vector<HInstruction> instructions;

    if (extended) {
        for (size_t i = 0; i + 8 <= data.size(); i += 8) {
            std::vector<uint8_t> instrBytes(data.begin() + i, data.begin() + i + 8);
            auto inst = HInstruction::decode64(instrBytes);
            if (inst) {
                instructions.push_back(std::move(*inst));
            }
        }
    } else {
        for (size_t i = 0; i + 4 <= data.size(); i += 4) {
            std::vector<uint8_t> instrBytes(data.begin() + i, data.begin() + i + 4);
            auto inst = HInstruction::decode(instrBytes);
            if (inst) {
                instructions.push_back(std::move(*inst));
            }
        }
    }

    return instructions;
}

std::string HoModule::instructionsToAssembly(const std::vector<HInstruction>& instructions) const {
    std::ostringstream oss;

    for (size_t i = 0; i < instructions.size(); ++i) {
        oss << "  " << i << ": " << instructions[i].toAssembly() << "\n";
    }

    return oss.str();
}

std::vector<HInstruction> HoModule::parseAssembly(const std::string& assembly) const {
    std::vector<HInstruction> instructions;
    std::istringstream iss(assembly);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            line = line.substr(colonPos + 1);
        }

        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty()) continue;

        std::istringstream lineIss(line);
        std::string mnemonic;
        lineIss >> mnemonic;

        if (mnemonic.empty()) continue;

        Opcode opcode = HInstruction::stringToOpcode(mnemonic);
        if (opcode == Opcode::UNKNOWN) {
            continue;
        }

        HInstruction inst(opcode);
        instructions.push_back(inst);
    }

    return instructions;
}

std::string HoModule::getError() const { return error_; }
bool HoModule::hasError() const { return !error_.empty(); }
void HoModule::clearError() { error_.clear(); }

bool HoModule::serializeToFile(const std::string& file_path) const {
    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool HoModule::deserializeFromFile(const std::string& file_path) {
    auto parsed = parse(file_path);
    if (!parsed) {
        error_ = "Failed to parse module from file";
        return false;
    }

    std::vector<uint8_t> data;
    parsed->serialize(data);
    return deserialize(data);
}

bool HoModule::deserialize(const std::vector<uint8_t>& input) {
    auto parsed = parse(input);
    if (!parsed) {
        error_ = "Failed to parse module";
        return false;
    }

    magic_ = parsed->magic_;
    version_major_ = parsed->version_major_;
    version_minor_ = parsed->version_minor_;
    file_type_ = parsed->file_type_;
    target_arch_ = parsed->target_arch_;
    endianness_ = parsed->endianness_;
    pointer_size_ = parsed->pointer_size_;
    flags_ = parsed->flags_;
    entry_point_ = parsed->entry_point_;
    base_address_ = parsed->base_address_;
    sections_ = std::move(parsed->sections_);
    string_pool_ = std::move(parsed->string_pool_);
    symbols_ = std::move(parsed->symbols_);
    relocations_ = std::move(parsed->relocations_);
    exports_ = std::move(parsed->exports_);
    imports_ = std::move(parsed->imports_);
    function_metadata_ = std::move(parsed->function_metadata_);

    return true;
}

std::unique_ptr<HoModule> HoModule::parse(const std::vector<uint8_t>& data) {
    auto module = std::unique_ptr<HoModule>(new HoModule());
    size_t offset = 0;

    if (!module->parseHeader(data, offset)) {
        return nullptr;
    }

    if (!module->parseSectionTable(data, offset)) {
        return nullptr;
    }

    const Section* symtab = module->getSection(".symtab");
    if (symtab) {
        module->parseSymbols(data, *symtab);
    }

    const Section* reloc = module->getSection(".reloc");
    if (reloc) {
        module->parseRelocations(data, *reloc);
    }

    const Section* exports = module->getSection(".export");
    if (exports) {
        module->parseExports(data, *exports);
    }

    const Section* imports = module->getSection(".import");
    if (imports) {
        module->parseImports(data, *imports);
    }

    const Section* funcmeta = module->getSection(".funcmeta");
    if (funcmeta) {
        module->parseFunctionMetadata(data, *funcmeta);
    }

    return module;
}

std::unique_ptr<HoModule> HoModule::parse(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return nullptr;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return nullptr;
    }

    return parse(data);
}

std::unique_ptr<HoModule> HoModule::parse(FILE* file) {
    if (!file) {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> data(size);
    if (fread(data.data(), 1, size, file) != size) {
        return nullptr;
    }

    return parse(data);
}

}
