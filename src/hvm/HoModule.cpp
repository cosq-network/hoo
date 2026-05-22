#include "hvm/HoModule.h"
#include "core/DefaultIOProvider.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>

namespace hvm {
namespace {

constexpr size_t kSectionEntrySize = 40;
constexpr size_t kSymbolEntrySize = 32;
constexpr size_t kRelocationEntrySize = 16;
constexpr size_t kExportEntrySize = 24;
constexpr size_t kImportEntrySize = 32;
constexpr size_t kFunctionMetadataEntrySize = 48;

bool willAddOverflow(size_t a, size_t b) {
    return a > std::numeric_limits<size_t>::max() - b;
}

bool willMulOverflow(size_t a, size_t b) {
    return (a != 0) && (b > std::numeric_limits<size_t>::max() / a);
}

bool readBytes(const std::vector<uint8_t>& data, size_t offset, void* out, size_t size) {
    if (willAddOverflow(offset, size) || offset + size > data.size()) {
        return false;
    }
    std::memcpy(out, data.data() + offset, size);
    return true;
}

bool readU16LE(const std::vector<uint8_t>& data, size_t offset, uint16_t& out) {
    uint8_t bytes[2]{};
    if (!readBytes(data, offset, bytes, sizeof(bytes))) {
        return false;
    }
    out = static_cast<uint16_t>(bytes[0]) |
          (static_cast<uint16_t>(bytes[1]) << 8U);
    return true;
}

bool readU32LE(const std::vector<uint8_t>& data, size_t offset, uint32_t& out) {
    uint8_t bytes[4]{};
    if (!readBytes(data, offset, bytes, sizeof(bytes))) {
        return false;
    }
    out = static_cast<uint32_t>(bytes[0]) |
          (static_cast<uint32_t>(bytes[1]) << 8U) |
          (static_cast<uint32_t>(bytes[2]) << 16U) |
          (static_cast<uint32_t>(bytes[3]) << 24U);
    return true;
}

bool readU64LE(const std::vector<uint8_t>& data, size_t offset, uint64_t& out) {
    uint8_t bytes[8]{};
    if (!readBytes(data, offset, bytes, sizeof(bytes))) {
        return false;
    }
    out = static_cast<uint64_t>(bytes[0]) |
          (static_cast<uint64_t>(bytes[1]) << 8U) |
          (static_cast<uint64_t>(bytes[2]) << 16U) |
          (static_cast<uint64_t>(bytes[3]) << 24U) |
          (static_cast<uint64_t>(bytes[4]) << 32U) |
          (static_cast<uint64_t>(bytes[5]) << 40U) |
          (static_cast<uint64_t>(bytes[6]) << 48U) |
          (static_cast<uint64_t>(bytes[7]) << 56U);
    return true;
}

bool readI16LE(const std::vector<uint8_t>& data, size_t offset, int16_t& out) {
    uint16_t value = 0;
    if (!readU16LE(data, offset, value)) {
        return false;
    }
    out = static_cast<int16_t>(value);
    return true;
}

bool readI32LE(const std::vector<uint8_t>& data, size_t offset, int32_t& out) {
    uint32_t value = 0;
    if (!readU32LE(data, offset, value)) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

void writeU16LE(std::vector<uint8_t>& data, size_t offset, uint16_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFU);
    data[offset + 1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
}

void writeU32LE(std::vector<uint8_t>& data, size_t offset, uint32_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFU);
    data[offset + 1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
}

void writeU64LE(std::vector<uint8_t>& data, size_t offset, uint64_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFU);
    data[offset + 1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 4] = static_cast<uint8_t>((value >> 32U) & 0xFFU);
    data[offset + 5] = static_cast<uint8_t>((value >> 40U) & 0xFFU);
    data[offset + 6] = static_cast<uint8_t>((value >> 48U) & 0xFFU);
    data[offset + 7] = static_cast<uint8_t>((value >> 56U) & 0xFFU);
}

bool alignUpChecked(size_t value, size_t alignment, size_t& out) {
    if (alignment == 0 || alignment == 1) {
        out = value;
        return true;
    }
    const size_t rem = value % alignment;
    if (rem == 0) {
        out = value;
        return true;
    }
    const size_t add = alignment - rem;
    if (willAddOverflow(value, add)) {
        return false;
    }
    out = value + add;
    return true;
}

bool appendString(std::string& pool, const std::string& str, uint32_t& offset) {
    if (pool.size() > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    offset = static_cast<uint32_t>(pool.size());
    pool += str;
    pool.push_back('\0');
    return pool.size() <= std::numeric_limits<uint32_t>::max();
}

std::string readStringFromPool(const std::string& pool, uint32_t offset) {
    if (offset >= pool.size()) {
        return "";
    }
    const char* begin = pool.c_str() + offset;
    return std::string(begin);
}

std::string defaultSectionName(SectionType type) {
    switch (type) {
        case SectionType::SHT_TEXT: return ".text";
        case SectionType::SHT_RODATA: return ".rodata";
        case SectionType::SHT_DATA: return ".data";
        case SectionType::SHT_BSS: return ".bss";
        case SectionType::SHT_SYMTAB: return ".symtab";
        case SectionType::SHT_STRTAB: return ".strtab";
        case SectionType::SHT_RELOC: return ".reloc";
        case SectionType::SHT_EXPORT: return ".export";
        case SectionType::SHT_IMPORT: return ".import";
        case SectionType::SHT_FUNCMETA: return ".funcmeta";
        default: return "";
    }
}

}  // namespace

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
    , string_pool_(1, '\0') {}

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
    , string_pool_(1, '\0') {}

HoModule::~HoModule() = default;

std::unique_ptr<HoModule> HoModule::create() { return std::unique_ptr<HoModule>(new HoModule()); }

std::unique_ptr<HoModule> HoModule::create(const std::string& name) {
    return std::unique_ptr<HoModule>(new HoModule(name));
}

bool HoModule::serialize(std::vector<uint8_t>& output) const {
    output.clear();
    error_.clear();

    if (endianness_ != Endianness::Little) {
        error_ = "Only little-endian serialization is supported";
        return false;
    }

    std::vector<Section> effective_sections;
    effective_sections.reserve(sections_.size() + 5);

    for (const auto& sec : sections_) {
        if (sec.type == SectionType::SHT_STRTAB || sec.type == SectionType::SHT_SYMTAB ||
            sec.type == SectionType::SHT_RELOC || sec.type == SectionType::SHT_EXPORT ||
            sec.type == SectionType::SHT_IMPORT || sec.type == SectionType::SHT_FUNCMETA) {
            if (!sec.data.empty()) {
                error_ = std::string("User-supplied metadata section payload is not supported for: ") + sec.name;
                return false;
            }
            continue;
        }
        effective_sections.push_back(sec);
    }

    auto buildTableSection = [&](const std::string& name,
                                 SectionType type,
                                 uint64_t align,
                                 const std::vector<uint8_t>& data) {
        if (data.empty()) {
            return;
        }
        Section s;
        s.name = name;
        s.type = type;
        s.flags = 0;
        s.virtual_size = data.size();
        s.alignment = align;
        s.data = data;
        effective_sections.push_back(std::move(s));
    };

    auto buildSymbolData = [&]() {
        if (willMulOverflow(symbols_.size(), kSymbolEntrySize)) {
            error_ = "Symbol table size overflow";
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data;
        data.resize(symbols_.size() * kSymbolEntrySize, 0);
        return data;
    };
    auto buildRelocData = [&]() {
        if (willMulOverflow(relocations_.size(), kRelocationEntrySize)) {
            error_ = "Relocation table size overflow";
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data;
        data.resize(relocations_.size() * kRelocationEntrySize, 0);
        return data;
    };
    auto buildExportData = [&]() {
        if (willMulOverflow(exports_.size(), kExportEntrySize)) {
            error_ = "Export table size overflow";
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data;
        data.resize(exports_.size() * kExportEntrySize, 0);
        return data;
    };
    auto buildImportData = [&]() {
        if (willMulOverflow(imports_.size(), kImportEntrySize)) {
            error_ = "Import table size overflow";
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data;
        data.resize(imports_.size() * kImportEntrySize, 0);
        return data;
    };
    auto buildFuncMetaData = [&]() {
        if (willMulOverflow(function_metadata_.size(), kFunctionMetadataEntrySize)) {
            error_ = "Function metadata table size overflow";
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data;
        data.resize(function_metadata_.size() * kFunctionMetadataEntrySize, 0);
        return data;
    };

    std::vector<uint8_t> symtab_data = buildSymbolData();
    std::vector<uint8_t> reloc_data = buildRelocData();
    std::vector<uint8_t> export_data = buildExportData();
    std::vector<uint8_t> import_data = buildImportData();
    std::vector<uint8_t> funcmeta_data = buildFuncMetaData();
    if (!error_.empty()) {
        return false;
    }

    buildTableSection(".symtab", SectionType::SHT_SYMTAB, 8, symtab_data);
    buildTableSection(".reloc", SectionType::SHT_RELOC, 8, reloc_data);
    buildTableSection(".export", SectionType::SHT_EXPORT, 8, export_data);
    buildTableSection(".import", SectionType::SHT_IMPORT, 8, import_data);
    buildTableSection(".funcmeta", SectionType::SHT_FUNCMETA, 8, funcmeta_data);

    std::string local_string_pool(1, '\0');
    std::vector<uint32_t> section_name_offsets;
    section_name_offsets.reserve(effective_sections.size() + 1);

    for (const auto& sec : effective_sections) {
        uint32_t off = 0;
        if (!appendString(local_string_pool, sec.name, off)) {
            return false;
        }
        section_name_offsets.push_back(off);
    }

    for (size_t i = 0; i < symbols_.size(); ++i) {
        uint32_t name_off = 0;
        if (!appendString(local_string_pool, symbols_[i].name, name_off)) {
            return false;
        }
        const size_t base = i * kSymbolEntrySize;
        writeU32LE(symtab_data, base + 0x00, name_off);
        symtab_data[base + 0x04] = symbols_[i].binding;
        symtab_data[base + 0x05] = symbols_[i].type;
        symtab_data[base + 0x06] = symbols_[i].visibility;
        symtab_data[base + 0x07] = symbols_[i].reserved;
        writeU64LE(symtab_data, base + 0x08, symbols_[i].value);
        writeU64LE(symtab_data, base + 0x10, symbols_[i].size);
        writeU32LE(symtab_data, base + 0x18, static_cast<uint32_t>(symbols_[i].section_index));
        writeU32LE(symtab_data, base + 0x1C, symbols_[i].symbol_index);
    }

    for (size_t i = 0; i < relocations_.size(); ++i) {
        const size_t base = i * kRelocationEntrySize;
        writeU64LE(reloc_data, base + 0x00, relocations_[i].offset);
        writeU32LE(reloc_data, base + 0x08, relocations_[i].symbol_index);
        writeU16LE(reloc_data, base + 0x0C, relocations_[i].relocation_type);
        writeU16LE(reloc_data, base + 0x0E, static_cast<uint16_t>(relocations_[i].addend));
    }

    for (size_t i = 0; i < exports_.size(); ++i) {
        uint32_t name_off = 0;
        if (!appendString(local_string_pool, exports_[i].name, name_off)) {
            return false;
        }
        const size_t base = i * kExportEntrySize;
        writeU32LE(export_data, base + 0x00, name_off);
        writeU32LE(export_data, base + 0x04, exports_[i].symbol_index);
        writeU64LE(export_data, base + 0x08, exports_[i].address);
        writeU64LE(export_data, base + 0x10, exports_[i].size);
    }

    for (size_t i = 0; i < imports_.size(); ++i) {
        uint32_t name_off = 0;
        uint32_t lib_off = 0;
        if (!appendString(local_string_pool, imports_[i].name, name_off) ||
            !appendString(local_string_pool, imports_[i].library, lib_off)) {
            return false;
        }
        const size_t base = i * kImportEntrySize;
        writeU32LE(import_data, base + 0x00, name_off);
        writeU32LE(import_data, base + 0x04, lib_off);
        writeU32LE(import_data, base + 0x08, imports_[i].import_type);
        writeU32LE(import_data, base + 0x0C, imports_[i].version);
        writeU64LE(import_data, base + 0x10, imports_[i].flags);
        writeU64LE(import_data, base + 0x18, imports_[i].resolved_address);
    }

    for (size_t i = 0; i < function_metadata_.size(); ++i) {
        uint32_t name_off = 0;
        if (!appendString(local_string_pool, function_metadata_[i].name, name_off)) {
            return false;
        }
        const size_t base = i * kFunctionMetadataEntrySize;
        writeU32LE(funcmeta_data, base + 0x00, name_off);
        writeU32LE(funcmeta_data, base + 0x04, function_metadata_[i].symbol_index);
        writeU64LE(funcmeta_data, base + 0x08, function_metadata_[i].entry_rva);
        writeU32LE(funcmeta_data, base + 0x10, function_metadata_[i].code_size);
        writeU32LE(funcmeta_data, base + 0x14, function_metadata_[i].local_size);
        writeU32LE(funcmeta_data, base + 0x18, function_metadata_[i].param_count);
        writeU32LE(funcmeta_data, base + 0x1C, function_metadata_[i].param_types_offset);
        writeU32LE(funcmeta_data, base + 0x20, function_metadata_[i].return_type_offset);
        writeU32LE(funcmeta_data, base + 0x24, function_metadata_[i].flags);
        writeU32LE(funcmeta_data, base + 0x28, function_metadata_[i].source_line);
        writeU32LE(funcmeta_data, base + 0x2C, function_metadata_[i].debug_offset);
    }

    for (auto& sec : effective_sections) {
        switch (sec.type) {
            case SectionType::SHT_SYMTAB: sec.data = symtab_data; sec.virtual_size = sec.data.size(); break;
            case SectionType::SHT_RELOC: sec.data = reloc_data; sec.virtual_size = sec.data.size(); break;
            case SectionType::SHT_EXPORT: sec.data = export_data; sec.virtual_size = sec.data.size(); break;
            case SectionType::SHT_IMPORT: sec.data = import_data; sec.virtual_size = sec.data.size(); break;
            case SectionType::SHT_FUNCMETA: sec.data = funcmeta_data; sec.virtual_size = sec.data.size(); break;
            default: break;
        }
    }

    Section strtab;
    strtab.name = ".strtab";
    strtab.type = SectionType::SHT_STRTAB;
    strtab.flags = SectionFlags::ALLOC | SectionFlags::STRINGS;
    strtab.alignment = 1;
    strtab.data.assign(local_string_pool.begin(), local_string_pool.end());
    strtab.virtual_size = strtab.data.size();

    uint32_t strtab_name_offset = 0;
    if (!appendString(local_string_pool, strtab.name, strtab_name_offset)) {
        return false;
    }
    strtab.data.assign(local_string_pool.begin(), local_string_pool.end());
    strtab.virtual_size = strtab.data.size();

    effective_sections.push_back(std::move(strtab));
    section_name_offsets.push_back(strtab_name_offset);

    output.resize(HEADER_SIZE, 0);
    writeU32LE(output, 0x00, magic_);
    writeU16LE(output, 0x04, version_major_);
    writeU16LE(output, 0x06, version_minor_);
    output[0x08] = static_cast<uint8_t>(file_type_);
    output[0x09] = static_cast<uint8_t>(target_arch_);
    output[0x0A] = static_cast<uint8_t>(endianness_);
    output[0x0B] = pointer_size_;
    writeU32LE(output, 0x0C, flags_);
    writeU64LE(output, 0x10, entry_point_);
    writeU64LE(output, 0x18, base_address_);
    writeU64LE(output, 0x20, static_cast<uint64_t>(effective_sections.size()));
    writeU64LE(output, 0x28, effective_sections.empty() ? 0U : HEADER_SIZE + (effective_sections.size() * kSectionEntrySize));
    writeU32LE(output, 0x30, static_cast<uint32_t>(symbols_.size()));
    writeU32LE(output, 0x34, static_cast<uint32_t>(relocations_.size()));
    writeU32LE(output, 0x38, static_cast<uint32_t>(exports_.size()));
    writeU32LE(output, 0x3C, static_cast<uint32_t>(imports_.size()));

    const size_t table_start = output.size();
    output.resize(table_start + (effective_sections.size() * kSectionEntrySize), 0);

    size_t data_offset = table_start + (effective_sections.size() * kSectionEntrySize);
    for (size_t i = 0; i < effective_sections.size(); ++i) {
        auto& sec = effective_sections[i];
        const size_t entry_offset = table_start + (i * kSectionEntrySize);

        writeU64LE(output, entry_offset + 0x00, section_name_offsets[i]);
        writeU32LE(output, entry_offset + 0x08, static_cast<uint32_t>(sec.type));
        writeU32LE(output, entry_offset + 0x0C, sec.flags);
        writeU64LE(output, entry_offset + 0x10, sec.virtual_size);

        if (sec.type != SectionType::SHT_BSS && sec.virtual_size > 0 && !sec.data.empty()) {
            size_t aligned_offset = 0;
            if (!alignUpChecked(data_offset, static_cast<size_t>(std::max<uint64_t>(1, sec.alignment)), aligned_offset)) {
                error_ = "Section alignment overflow during serialization";
                return false;
            }
            data_offset = aligned_offset;
            writeU64LE(output, entry_offset + 0x18, data_offset);
            sec.file_offset = data_offset;

            if (output.size() < data_offset) {
                output.resize(data_offset, 0);
            }
            if (sec.data.size() > std::numeric_limits<size_t>::max() - output.size()) {
                error_ = "Section payload size overflow during serialization";
                return false;
            }
            output.insert(output.end(), sec.data.begin(), sec.data.end());
            data_offset = output.size();
        } else {
            writeU64LE(output, entry_offset + 0x18, 0);
        }

        writeU64LE(output, entry_offset + 0x20, sec.alignment);
    }

    return true;
}

bool HoModule::serialize(const std::string& file_path) const {
    error_.clear();
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = std::string("Cannot open file for writing: ") + file_path;
        return false;
    }

    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return file.good();
}

bool HoModule::serialize(FILE* file) const {
    error_.clear();
    if (!file) {
        error_ = "Cannot write to null FILE*";
        return false;
    }

    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    if (fwrite(data.data(), 1, data.size(), file) != data.size()) {
        error_ = "Failed to write serialized module to FILE*";
        return false;
    }
    return true;
}

bool HoModule::parseHeader(const std::vector<uint8_t>& data, size_t& offset) {
    uint32_t magic = 0;
    if (data.size() < HEADER_SIZE || !readU32LE(data, 0x00, magic)) {
        error_ = "File too small for header";
        return false;
    }

    magic_ = magic;
    if (magic_ != MAGIC) {
        error_ = "Invalid magic number";
        return false;
    }

    if (!readU16LE(data, 0x04, version_major_) ||
        !readU16LE(data, 0x06, version_minor_)) {
        error_ = "Failed to read version";
        return false;
    }

    file_type_ = static_cast<FileType>(data[0x08]);
    target_arch_ = static_cast<TargetArch>(data[0x09]);
    endianness_ = static_cast<Endianness>(data[0x0A]);
    pointer_size_ = data[0x0B];
    if (endianness_ != Endianness::Little) {
        error_ = "Only little-endian modules are supported";
        return false;
    }

    uint64_t section_count = 0;
    if (!readU32LE(data, 0x0C, flags_) ||
        !readU64LE(data, 0x10, entry_point_) ||
        !readU64LE(data, 0x18, base_address_) ||
        !readU64LE(data, 0x20, section_count)) {
        error_ = "Failed to read header fields";
        return false;
    }

    if (section_count > (std::numeric_limits<size_t>::max() / kSectionEntrySize)) {
        error_ = "Invalid section table size";
        return false;
    }
    const size_t table_size = static_cast<size_t>(section_count) * kSectionEntrySize;
    if (willAddOverflow(HEADER_SIZE, table_size) || HEADER_SIZE + table_size > data.size()) {
        error_ = "Invalid section table size";
        return false;
    }

    offset = HEADER_SIZE + table_size;
    return true;
}

bool HoModule::parseSectionTable(const std::vector<uint8_t>& data, size_t& offset) {
    uint64_t section_count64 = 0;
    if (!readU64LE(data, 0x20, section_count64)) {
        error_ = "Failed to read section count";
        return false;
    }

    if (section_count64 > std::numeric_limits<size_t>::max()) {
        error_ = "Section count overflow";
        return false;
    }
    const size_t section_count = static_cast<size_t>(section_count64);

    if (willMulOverflow(section_count, kSectionEntrySize)) {
        error_ = "Section table exceeds file size";
        return false;
    }
    const size_t table_bytes = section_count * kSectionEntrySize;
    if (willAddOverflow(HEADER_SIZE, table_bytes) || HEADER_SIZE + table_bytes > data.size()) {
        error_ = "Section table exceeds file size";
        return false;
    }

    struct RawSection {
        uint64_t name_offset = 0;
        Section sec;
    };

    std::vector<RawSection> raw_sections;
    raw_sections.reserve(section_count);

    for (size_t i = 0; i < section_count; ++i) {
        const size_t table_offset = HEADER_SIZE + i * kSectionEntrySize;
        RawSection raw;
        uint32_t type = 0;

        if (!readU64LE(data, table_offset + 0x00, raw.name_offset) ||
            !readU32LE(data, table_offset + 0x08, type) ||
            !readU32LE(data, table_offset + 0x0C, raw.sec.flags) ||
            !readU64LE(data, table_offset + 0x10, raw.sec.virtual_size) ||
            !readU64LE(data, table_offset + 0x18, raw.sec.file_offset) ||
            !readU64LE(data, table_offset + 0x20, raw.sec.alignment)) {
            error_ = "Failed to read section table entry";
            return false;
        }

        raw.sec.type = static_cast<SectionType>(type);

        if (raw.sec.type != SectionType::SHT_BSS && raw.sec.type != SectionType::SHT_NULL &&
            raw.sec.virtual_size > 0) {
            if (raw.sec.file_offset > data.size() ||
                raw.sec.virtual_size > (data.size() - raw.sec.file_offset)) {
                error_ = "Section payload exceeds file size";
                return false;
            }

            raw.sec.data.resize(static_cast<size_t>(raw.sec.virtual_size));
            std::memcpy(raw.sec.data.data(), data.data() + raw.sec.file_offset, raw.sec.data.size());
        }

        raw_sections.push_back(std::move(raw));
    }

    string_pool_ = std::string(1, '\0');
    for (const auto& raw : raw_sections) {
        if (raw.sec.type == SectionType::SHT_STRTAB && !raw.sec.data.empty()) {
            string_pool_.assign(raw.sec.data.begin(), raw.sec.data.end());
            break;
        }
    }

    if (string_pool_.empty() || string_pool_[0] != '\0') {
        string_pool_.insert(string_pool_.begin(), '\0');
    }

    sections_.clear();
    sections_.reserve(raw_sections.size());

    for (const auto& raw : raw_sections) {
        Section sec = raw.sec;
        sec.name = readStringFromPool(string_pool_, static_cast<uint32_t>(raw.name_offset));
        if (sec.name.empty()) {
            sec.name = defaultSectionName(sec.type);
        }
        sections_.push_back(std::move(sec));
    }

    offset = HEADER_SIZE + table_bytes;
    return true;
}

bool HoModule::parseSymbols(const std::vector<uint8_t>& /*data*/, const Section& symtab) {
    if (symtab.data.size() % kSymbolEntrySize != 0) {
        error_ = "Invalid symbol table size";
        return false;
    }

    const size_t count = symtab.data.size() / kSymbolEntrySize;
    symbols_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& d = symtab.data;
        const size_t base = i * kSymbolEntrySize;
        uint32_t name_offset = 0;
        int32_t section_index = 0;

        Symbol symbol;
        if (!readU32LE(d, base + 0x00, name_offset) ||
            !readU64LE(d, base + 0x08, symbol.value) ||
            !readU64LE(d, base + 0x10, symbol.size) ||
            !readI32LE(d, base + 0x18, section_index) ||
            !readU32LE(d, base + 0x1C, symbol.symbol_index)) {
            error_ = "Failed to decode symbol entry";
            return false;
        }

        symbol.name = readStringFromPool(string_pool_, name_offset);
        symbol.binding = d[base + 0x04];
        symbol.type = d[base + 0x05];
        symbol.visibility = d[base + 0x06];
        symbol.reserved = d[base + 0x07];
        symbol.section_index = section_index;
        addSymbol(symbol);
    }

    return true;
}

bool HoModule::parseRelocations(const std::vector<uint8_t>& /*data*/, const Section& reloc) {
    if (reloc.data.size() % kRelocationEntrySize != 0) {
        error_ = "Invalid relocation table size";
        return false;
    }

    const size_t count = reloc.data.size() / kRelocationEntrySize;
    relocations_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& d = reloc.data;
        const size_t base = i * kRelocationEntrySize;
        Relocation relocation;

        if (!readU64LE(d, base + 0x00, relocation.offset) ||
            !readU32LE(d, base + 0x08, relocation.symbol_index) ||
            !readU16LE(d, base + 0x0C, relocation.relocation_type) ||
            !readI16LE(d, base + 0x0E, relocation.addend)) {
            error_ = "Failed to decode relocation entry";
            return false;
        }

        relocations_.push_back(relocation);
    }

    return true;
}

bool HoModule::parseExports(const std::vector<uint8_t>& /*data*/, const Section& exports) {
    if (exports.data.size() % kExportEntrySize != 0) {
        error_ = "Invalid export table size";
        return false;
    }

    const size_t count = exports.data.size() / kExportEntrySize;
    exports_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& d = exports.data;
        const size_t base = i * kExportEntrySize;
        uint32_t name_offset = 0;
        ExportEntry exp;

        if (!readU32LE(d, base + 0x00, name_offset) ||
            !readU32LE(d, base + 0x04, exp.symbol_index) ||
            !readU64LE(d, base + 0x08, exp.address) ||
            !readU64LE(d, base + 0x10, exp.size)) {
            error_ = "Failed to decode export entry";
            return false;
        }

        exp.name = readStringFromPool(string_pool_, name_offset);
        exports_.push_back(exp);
    }

    return true;
}

bool HoModule::parseImports(const std::vector<uint8_t>& /*data*/, const Section& imports) {
    if (imports.data.size() % kImportEntrySize != 0) {
        error_ = "Invalid import table size";
        return false;
    }

    const size_t count = imports.data.size() / kImportEntrySize;
    imports_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& d = imports.data;
        const size_t base = i * kImportEntrySize;
        uint32_t name_offset = 0;
        uint32_t lib_offset = 0;
        ImportEntry imp;

        if (!readU32LE(d, base + 0x00, name_offset) ||
            !readU32LE(d, base + 0x04, lib_offset) ||
            !readU32LE(d, base + 0x08, imp.import_type) ||
            !readU32LE(d, base + 0x0C, imp.version) ||
            !readU64LE(d, base + 0x10, imp.flags) ||
            !readU64LE(d, base + 0x18, imp.resolved_address)) {
            error_ = "Failed to decode import entry";
            return false;
        }

        imp.name = readStringFromPool(string_pool_, name_offset);
        imp.library = readStringFromPool(string_pool_, lib_offset);
        imports_.push_back(imp);
    }

    return true;
}

bool HoModule::parseFunctionMetadata(const std::vector<uint8_t>& /*data*/, const Section& funcmeta) {
    if (funcmeta.data.size() % kFunctionMetadataEntrySize != 0) {
        error_ = "Invalid function metadata table size";
        return false;
    }

    const size_t count = funcmeta.data.size() / kFunctionMetadataEntrySize;
    function_metadata_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& d = funcmeta.data;
        const size_t base = i * kFunctionMetadataEntrySize;
        uint32_t name_offset = 0;
        FunctionMetadata meta;

        if (!readU32LE(d, base + 0x00, name_offset) ||
            !readU32LE(d, base + 0x04, meta.symbol_index) ||
            !readU64LE(d, base + 0x08, meta.entry_rva) ||
            !readU32LE(d, base + 0x10, meta.code_size) ||
            !readU32LE(d, base + 0x14, meta.local_size) ||
            !readU32LE(d, base + 0x18, meta.param_count) ||
            !readU32LE(d, base + 0x1C, meta.param_types_offset) ||
            !readU32LE(d, base + 0x20, meta.return_type_offset) ||
            !readU32LE(d, base + 0x24, meta.flags) ||
            !readU32LE(d, base + 0x28, meta.source_line) ||
            !readU32LE(d, base + 0x2C, meta.debug_offset)) {
            error_ = "Failed to decode function metadata entry";
            return false;
        }

        meta.name = readStringFromPool(string_pool_, name_offset);
        function_metadata_.push_back(meta);
    }

    return true;
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

std::optional<uint32_t> HoModule::addString(const std::string& str) {
    uint32_t offset = 0;
    if (!appendString(string_pool_, str, offset)) {
        return std::nullopt;
    }
    return offset;
}
std::string HoModule::getString(uint32_t offset) const { return readStringFromPool(string_pool_, offset); }
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
        const auto bytes = inst.encode();
        encoded.insert(encoded.end(), bytes.begin(), bytes.end());
    }

    return encoded;
}

std::vector<HInstruction> HoModule::decodeInstructions(const std::vector<uint8_t>& data, bool /*extended*/) const {
    std::vector<HInstruction> instructions;

    for (size_t i = 0; i < data.size(); ) {
        std::vector<uint8_t> remaining(data.begin() + static_cast<std::ptrdiff_t>(i), data.end());
        size_t bytesUsed = 0;
        auto inst = HInstruction::decode(remaining, bytesUsed);
        if (inst && bytesUsed > 0) {
            instructions.push_back(std::move(*inst));
            i += bytesUsed;
        } else {
            // Failed to decode or used 0 bytes (avoid infinite loop)
            break;
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

        const size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            line = line.substr(colonPos + 1);
        }

        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty()) continue;

        std::istringstream lineIss(line);
        std::string mnemonic;
        lineIss >> mnemonic;

        if (mnemonic.empty()) continue;

        const Opcode opcode = HInstruction::stringToOpcode(mnemonic);
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

    auto provider = getIOProvider() ? getIOProvider() : std::make_shared<hooc::DefaultIOProvider>();
    if (!provider->writeBinaryFile(file_path, data)) {
        error_ = std::string("Cannot write to file: ") + file_path;
        return false;
    }

    return true;
}

bool HoModule::deserializeFromFile(const std::string& file_path) {
    auto provider = getIOProvider() ? getIOProvider() : std::make_shared<hooc::DefaultIOProvider>();
    auto data = provider->readBinaryFile(file_path);
    if (!data) {
        error_ = std::string("Cannot read file: ") + file_path;
        return false;
    }

    return deserialize(*data);
}

bool HoModule::deserialize(const std::vector<uint8_t>& input) {
    auto parsed = parse(input);
    if (!parsed) {
        HoModule tmp;
        size_t off = 0;
        if (!tmp.parseHeader(input, off) || !tmp.parseSectionTable(input, off)) {
            error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
        } else {
            auto countByType = [&](SectionType type) {
                size_t count = 0;
                for (const auto& sec : tmp.sections_) {
                    if (sec.type == type) {
                        ++count;
                    }
                }
                return count;
            };
            auto rejectDuplicate = [&](SectionType type, const char* name) -> bool {
                if (countByType(type) > 1) {
                    error_ = std::string("Duplicate section type: ") + name;
                    return false;
                }
                return true;
            };
            if (!rejectDuplicate(SectionType::SHT_SYMTAB, ".symtab") ||
                !rejectDuplicate(SectionType::SHT_RELOC, ".reloc") ||
                !rejectDuplicate(SectionType::SHT_EXPORT, ".export") ||
                !rejectDuplicate(SectionType::SHT_IMPORT, ".import") ||
                !rejectDuplicate(SectionType::SHT_FUNCMETA, ".funcmeta")) {
                return false;
            }

            auto findByType = [&](SectionType type) -> const Section* {
                for (const auto& sec : tmp.sections_) {
                    if (sec.type == type) {
                        return &sec;
                    }
                }
                return nullptr;
            };

            if (const Section* symtab = findByType(SectionType::SHT_SYMTAB)) {
                if (!tmp.parseSymbols(input, *symtab)) {
                    error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
                    return false;
                }
            }
            if (const Section* reloc = findByType(SectionType::SHT_RELOC)) {
                if (!tmp.parseRelocations(input, *reloc)) {
                    error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
                    return false;
                }
            }
            if (const Section* exports = findByType(SectionType::SHT_EXPORT)) {
                if (!tmp.parseExports(input, *exports)) {
                    error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
                    return false;
                }
            }
            if (const Section* imports = findByType(SectionType::SHT_IMPORT)) {
                if (!tmp.parseImports(input, *imports)) {
                    error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
                    return false;
                }
            }
            if (const Section* funcmeta = findByType(SectionType::SHT_FUNCMETA)) {
                if (!tmp.parseFunctionMetadata(input, *funcmeta)) {
                    error_ = tmp.getError().empty() ? "Failed to parse module" : tmp.getError();
                    return false;
                }
            }

            error_ = "Failed to parse module";
        }
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

    auto findByType = [&](SectionType type) -> const Section* {
        for (const auto& sec : module->sections_) {
            if (sec.type == type) {
                return &sec;
            }
        }
        return nullptr;
    };

    module->symbols_.clear();
    module->relocations_.clear();
    module->exports_.clear();
    module->imports_.clear();
    module->function_metadata_.clear();

    auto countByType = [&](SectionType type) {
        size_t count = 0;
        for (const auto& sec : module->sections_) {
            if (sec.type == type) {
                ++count;
            }
        }
        return count;
    };
    auto rejectDuplicate = [&](SectionType type, const char* name) -> bool {
        if (countByType(type) > 1) {
            module->error_ = std::string("Duplicate section type: ") + name;
            return false;
        }
        return true;
    };
    if (!rejectDuplicate(SectionType::SHT_SYMTAB, ".symtab") ||
        !rejectDuplicate(SectionType::SHT_RELOC, ".reloc") ||
        !rejectDuplicate(SectionType::SHT_EXPORT, ".export") ||
        !rejectDuplicate(SectionType::SHT_IMPORT, ".import") ||
        !rejectDuplicate(SectionType::SHT_FUNCMETA, ".funcmeta")) {
        return nullptr;
    }

    if (const Section* symtab = findByType(SectionType::SHT_SYMTAB)) {
        if (!module->parseSymbols(data, *symtab)) {
            return nullptr;
        }
    }

    if (const Section* reloc = findByType(SectionType::SHT_RELOC)) {
        if (!module->parseRelocations(data, *reloc)) {
            return nullptr;
        }
    }

    if (const Section* exports = findByType(SectionType::SHT_EXPORT)) {
        if (!module->parseExports(data, *exports)) {
            return nullptr;
        }
    }

    if (const Section* imports = findByType(SectionType::SHT_IMPORT)) {
        if (!module->parseImports(data, *imports)) {
            return nullptr;
        }
    }

    if (const Section* funcmeta = findByType(SectionType::SHT_FUNCMETA)) {
        if (!module->parseFunctionMetadata(data, *funcmeta)) {
            return nullptr;
        }
    }

    return module;
}

std::unique_ptr<HoModule> HoModule::parse(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return nullptr;
    }

    const std::streampos end_pos = file.tellg();
    if (end_pos < 0) {
        return nullptr;
    }

    const size_t size = static_cast<size_t>(end_pos);
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size))) {
        return nullptr;
    }

    return parse(data);
}

std::unique_ptr<HoModule> HoModule::parse(FILE* file) {
    if (!file) {
        return nullptr;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        return nullptr;
    }

    const long file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        return nullptr;
    }
    if (static_cast<unsigned long>(file_size) > static_cast<unsigned long>(std::numeric_limits<size_t>::max())) {
        return nullptr;
    }

    const size_t size = static_cast<size_t>(file_size);
    std::vector<uint8_t> data(size);
    if (fread(data.data(), 1, size, file) != size) {
        return nullptr;
    }

    return parse(data);
}

}  // namespace hvm
