#ifndef HVM_HO_MODULE_H
#define HVM_HO_MODULE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#include "hvm/HInstruction.h"

namespace hvm {

enum class FileType : uint8_t {
    Executable = 0x01,
    SharedObject = 0x02,
    ObjectFile = 0x03
};

enum class TargetArch : uint8_t {
    X86_64 = 0x00,
    ARM64 = 0x01,
    Any = 0xFF
};

enum class Endianness : uint8_t {
    Little = 0x01,
    Big = 0x02
};

enum class SectionType : uint32_t {
    SHT_NULL = 0x01,
    SHT_TEXT = 0x02,
    SHT_RODATA = 0x03,
    SHT_DATA = 0x04,
    SHT_BSS = 0x05,
    SHT_SYMTAB = 0x06,
    SHT_STRTAB = 0x07,
    SHT_RELOC = 0x08,
    SHT_EXPORT = 0x09,
    SHT_IMPORT = 0x0A,
    SHT_FUNCMETA = 0x0B,
    SHT_TYPES = 0x0C,
    SHT_NOTE = 0x0D,
    SHT_TLS = 0x0E,
    SHT_DEBUG_LINE = 0x0F,
    SHT_DEBUG_INFO = 0x10,
    SHT_DEBUG_ABBREV = 0x11,
    SHT_DEBUG_STR = 0x12,
    SHT_DEBUG_FRAME = 0x13,
    SHT_DEBUG_LOC = 0x14,
    SHT_DEBUG_RANGES = 0x15,
    SHT_DEBUG_MACINFO = 0x16,
    SHT_GROUP = 0x17
};

struct SectionFlags {
    static constexpr uint32_t TLS = 0x8000;
    static constexpr uint32_t ALLOC = 0x4000;
    static constexpr uint32_t WRITE = 0x2000;
    static constexpr uint32_t EXECUTE = 0x1000;
    static constexpr uint32_t MERGE = 0x0800;
    static constexpr uint32_t STRINGS = 0x0400;
    static constexpr uint32_t EXCLUDE = 0x0200;
    static constexpr uint32_t COMPRESSED = 0x0100;
};

struct Section {
    std::string name;
    SectionType type;
    uint32_t flags;
    uint64_t virtual_size;
    uint64_t file_offset;
    uint64_t alignment;
    std::vector<uint8_t> data;

    Section() : type(SectionType::SHT_NULL), flags(0), virtual_size(0),
                file_offset(0), alignment(1) {}
};

struct Symbol {
    std::string name;
    uint8_t binding;
    uint8_t type;
    uint8_t visibility;
    uint8_t reserved;
    uint64_t value;
    uint64_t size;
    int32_t section_index;
    uint32_t symbol_index;

    static constexpr uint8_t STB_LOCAL = 0;
    static constexpr uint8_t STB_GLOBAL = 1;
    static constexpr uint8_t STB_WEAK = 2;

    static constexpr uint8_t STT_NOTYPE = 0;
    static constexpr uint8_t STT_FUNC = 1;
    static constexpr uint8_t STT_OBJECT = 2;
    static constexpr uint8_t STT_TYPE = 3;
    static constexpr uint8_t STT_TLS = 4;

    static constexpr uint8_t STV_DEFAULT = 0;
    static constexpr uint8_t STV_INTERNAL = 1;
    static constexpr uint8_t STV_HIDDEN = 2;
    static constexpr uint8_t STV_PROTECTED = 3;
};

struct Relocation {
    uint64_t offset;
    uint32_t symbol_index;
    uint16_t relocation_type;
    int16_t addend;
};

struct ExportEntry {
    std::string name;
    uint32_t symbol_index;
    uint64_t address;
    uint64_t size;
};

struct ImportEntry {
    std::string name;
    std::string library;
    uint32_t import_type;
    uint32_t version;
    uint64_t flags;
    uint64_t resolved_address;

    static constexpr uint32_t IT_HOOC = 0x01;
    static constexpr uint32_t IT_NATIVE = 0x02;
    static constexpr uint32_t IT_RUNTIME = 0x03;
    static constexpr uint32_t IT_INTRINSIC = 0x04;
};

struct FunctionMetadata {
    std::string name;
    uint32_t symbol_index;
    uint64_t entry_rva;
    uint32_t code_size;
    uint32_t local_size;
    uint32_t param_count;
    uint32_t param_types_offset;
    uint32_t return_type_offset;
    uint32_t flags;
    uint32_t source_line;
    uint32_t debug_offset;
};

class HoModule {
public:
    HoModule();
    ~HoModule();

    static std::unique_ptr<HoModule> create();
    static std::unique_ptr<HoModule> parse(const std::vector<uint8_t>& data);
    static std::unique_ptr<HoModule> parse(const std::string& file_path);
    static std::unique_ptr<HoModule> parse(FILE* file);

    bool serialize(std::vector<uint8_t>& output) const;
    bool serialize(const std::string& file_path) const;
    bool serialize(FILE* file) const;

    void setMagic(uint32_t magic);
    uint32_t getMagic() const;

    void setVersion(uint16_t major, uint16_t minor);
    uint16_t getVersionMajor() const;
    uint16_t getVersionMinor() const;

    void setFileType(FileType type);
    FileType getFileType() const;

    void setTargetArch(TargetArch arch);
    TargetArch getTargetArch() const;

    void setEndianness(Endianness endian);
    Endianness getEndianness() const;

    void setPointerSize(uint8_t size);
    uint8_t getPointerSize() const;

    void setFlags(uint32_t flags);
    uint32_t getFlags() const;

    void setEntryPoint(uint64_t rva);
    uint64_t getEntryPoint() const;

    void setBaseAddress(uint64_t addr);
    uint64_t getBaseAddress() const;

    void addSection(Section section);
    Section* getSection(const std::string& name);
    const Section* getSection(const std::string& name) const;
    const std::vector<Section>& getSections() const;
    std::vector<Section>& getSections();
    size_t getSectionCount() const;

    uint32_t addString(const std::string& str) const;
    std::string getString(uint32_t offset) const;
    const std::string& getStringPool() const;

    void addSymbol(const Symbol& symbol);
    const std::vector<Symbol>& getSymbols() const;
    std::vector<Symbol>& getSymbols();
    const Symbol* getSymbol(const std::string& name) const;

    void addRelocation(const Relocation& reloc);
    const std::vector<Relocation>& getRelocations() const;
    std::vector<Relocation>& getRelocations();

    void addExport(const ExportEntry& exp);
    const std::vector<ExportEntry>& getExports() const;
    std::vector<ExportEntry>& getExports();

    void addImport(const ImportEntry& imp);
    const std::vector<ImportEntry>& getImports() const;
    std::vector<ImportEntry>& getImports();

    void addFunctionMetadata(const FunctionMetadata& meta);
    const std::vector<FunctionMetadata>& getFunctionMetadata() const;
    std::vector<FunctionMetadata>& getFunctionMetadata();

    bool hasDebugInfo() const;
    bool hasTypeInfo() const;
    bool isStripped() const;
    bool isPIE() const;
    uint8_t getOptimizationLevel() const;

    void setDebugInfo(bool has);
    void setTypeInfo(bool has);
    void setStripped(bool stripped);
    void setPIE(bool pie);
    void setOptimizationLevel(uint8_t level);

    std::vector<uint8_t> encodeInstructions(const std::vector<HInstruction>& instructions) const;
    std::vector<HInstruction> decodeInstructions(const std::vector<uint8_t>& data, bool extended = false) const;

    std::string instructionsToAssembly(const std::vector<HInstruction>& instructions) const;
    std::vector<HInstruction> parseAssembly(const std::string& assembly) const;

    std::string getError() const;
    bool hasError() const;
    void clearError();

    static constexpr uint32_t MAGIC = 0x484F4F43;
    static constexpr uint16_t VERSION_MAJOR = 1;
    static constexpr uint16_t VERSION_MINOR = 3;
    static constexpr size_t HEADER_SIZE = 64;

private:
    bool parseHeader(const std::vector<uint8_t>& data, size_t& offset);
    bool parseSectionTable(const std::vector<uint8_t>& data, size_t& offset);
    bool parseSymbols(const std::vector<uint8_t>& data, const Section& symtab);
    bool parseRelocations(const std::vector<uint8_t>& data, const Section& reloc);
    bool parseExports(const std::vector<uint8_t>& data, const Section& exports);
    bool parseImports(const std::vector<uint8_t>& data, const Section& imports);
    bool parseFunctionMetadata(const std::vector<uint8_t>& data, const Section& funcmeta);

    void serializeHeader(std::vector<uint8_t>& output) const;
    void serializeSectionTable(std::vector<uint8_t>& output) const;
    void serializeSections(std::vector<uint8_t>& output) const;
    void serializeSymbols(std::vector<uint8_t>& output) const;
    void serializeRelocations(std::vector<uint8_t>& output) const;
    void serializeExports(std::vector<uint8_t>& output) const;
    void serializeImports(std::vector<uint8_t>& output) const;
    void serializeFunctionMetadata(std::vector<uint8_t>& output) const;

    uint32_t magic_;
    uint16_t version_major_;
    uint16_t version_minor_;
    FileType file_type_;
    TargetArch target_arch_;
    Endianness endianness_;
    uint8_t pointer_size_;
    uint32_t flags_;
    uint64_t entry_point_;
    uint64_t base_address_;

    std::vector<Section> sections_;
    mutable std::string string_pool_;
    std::vector<Symbol> symbols_;
    std::vector<Relocation> relocations_;
    std::vector<ExportEntry> exports_;
    std::vector<ImportEntry> imports_;
    std::vector<FunctionMetadata> function_metadata_;

    std::string error_;
};

}

#endif