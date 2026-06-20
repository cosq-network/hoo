#ifndef HVM_HVM_INSTRUCTION_H
#define HVM_HVM_INSTRUCTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>

namespace hvm {

/**
 * @brief HVM opcodes as defined by the HVM64 core/system profile.
 * Note: Multiple instructions may share the same opcode value (e.g., Arithmetic family 0x10),
 * being differentiated by the 'func' field in R-type instructions.
 */
enum class Opcode : uint16_t {
    NOP      = 0x00,
    MOV      = 0x01,
    MOVZ     = 0x03,
    LUI      = 0x04,
    ADDI     = 0x05,
    RETAIN   = 0x06,
    RELEASE  = 0x07,
    ICACHE_RNG = 0x0B,
    ARITH    = 0x10, // ADD, SUB, MUL, DIV, DIVU, REM
    SHIFT    = 0x13, // SHL, SHR, SAR
    LOGIC    = 0x20, // AND, OR, XOR
    NOT      = 0x21,
    FLOAT_ARITH = 0x30, // FADD, FSUB, FMUL, FDIV
    CMP      = 0x40, // CMPEQ, CMPNE, CMPLT, CMPLE
    FCMP     = 0x41, // FCMPEQ, FCMPLT, FCMPLE
    BEQ      = 0x50,
    BNE      = 0x51,
    BLT      = 0x52,
    BLE      = 0x53,
    JMP      = 0x60,
    JAL      = 0x61,
    JALR     = 0x62,
    RET      = 0x63,
    LD_B     = 0x70,
    LD_BU    = 0x71,
    LD_H     = 0x72,
    LD_HU    = 0x73,
    LD_W     = 0x74,
    LD_WU    = 0x75,
    LD_D     = 0x76,
    LD_P     = 0x77,
    ST_B     = 0x78,
    ST_H     = 0x79,
    ST_W     = 0x7A,
    ST_D     = 0x7B,
    ST_P     = 0x7C,
    LDA      = 0x7D,
    PUSH     = 0x7E,
    POP      = 0x7F,
    ENTER    = 0x80,
    LEAVE    = 0x81,
    ADJSP    = 0x82,
    FRAME    = 0x83,
    CALL     = 0xB4,
    TAILCALL = 0xB6,
    SYSCALL  = 0xC0,
    BREAK    = 0xC1,
    ECALL    = 0xC2,
    TRAPRET  = 0xC3,
    LR_D     = 0xC4,
    SC_D     = 0xC5,
    CSRRW    = 0xC6,
    SFENCE_VMA = 0xC8,
    LOOP_SET = 0xD0,
    LOOP_DECBR = 0xD1,
    PREFETCH_R = 0xD2,
    PREFETCH_W = 0xD3,
    PREFETCH_NTA = 0xD4,
    MEMZERO_HINT = 0xD5,
    ALLOC_BUMP = 0xD6,
    RDPROF = 0xD7,
    CHK_B = 0xD8,
    LD_D_NZ = 0xD9,
    BR_HINT = 0xDA,
    DOORBELL = 0xDB,
    VSETVL = 0xE0,
    VECTOR_MEM = 0xE1,
    VECTOR_ARITH = 0xE2,
    VECTOR_FMA = 0xE3,
    VECTOR_MASK = 0xE4,
    VECTOR_REDUCE = 0xE5,
    VECTOR_SHIFT = 0xE6,
    VECTOR_BITWISE = 0xE7,
    UNKNOWN  = 0xFFFF
};

enum class InstructionFormat {
    R,
    I,
    B,
    J,
    RI,
    UNKNOWN
};

enum class InstructionEncoding {
    Base32,
    Escape32,
    Compressed16,
    UNKNOWN
};

struct OperandsR {
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint16_t func;
};

struct OperandsI {
    uint8_t rd;
    uint8_t rs;
    int16_t imm15;
};

struct OperandsB {
    uint8_t rs1;
    uint8_t rs2;
    int16_t imm15;
};

struct OperandsJ {
    uint8_t rd;
    int32_t offset; // 20-bit in spec
};

struct OperandsRI {
    uint8_t rd;
    uint8_t rs1; // renamed from rs for consistency
    uint8_t rs2; // renamed from rd2 for consistency
    uint16_t imm; // 10-bit in spec
};

using Operands = std::variant<OperandsR, OperandsI, OperandsB, OperandsJ, OperandsRI>;

class HVMInstruction {
public:
    HVMInstruction();
    explicit HVMInstruction(Opcode opcode);
    HVMInstruction(Opcode opcode, const Operands& operands);
    ~HVMInstruction() = default;

    static std::unique_ptr<HVMInstruction> decode(const std::vector<uint8_t>& bytes, size_t& bytesUsed, bool allowCompressed = false);
    static std::unique_ptr<HVMInstruction> decode(const std::vector<uint8_t>& bytes); // convenience wrapper
    static std::unique_ptr<HVMInstruction> decode(const uint32_t word);

    std::vector<uint8_t> encode() const;
    uint32_t encode32() const;

    void setOpcode(Opcode opcode) { opcode_ = opcode; }
    Opcode getOpcode() const { return opcode_; }

    void setOperands(const Operands& operands) { operands_ = operands; }
    const Operands& getOperands() const { return operands_; }

    void setFormat(InstructionFormat format) { format_ = format; }
    InstructionFormat getFormat() const { return format_; }

    void setMnemonic(const std::string& name) { mnemonic_ = name; }
    std::string getMnemonic() const { return mnemonic_; }

    bool isExtended() const { return static_cast<uint32_t>(opcode_) >= 0x80; }

    uint8_t getSize() const { return isExtended() ? 8 : 4; }

    std::string toString() const;
    std::string toAssembly() const;

    static std::string opcodeToString(Opcode opcode, uint16_t func = 0);
    static Opcode stringToOpcode(const std::string& name);
    static std::optional<InstructionFormat> getFormatForOpcode(Opcode opcode);

    static bool validateRegister(uint8_t reg);
    static bool validateImmediate(int64_t value, int bits);

private:
    Opcode opcode_;
    Operands operands_;
    InstructionFormat format_;
    std::string mnemonic_;
};

class InstructionRegistry {
public:
    static InstructionRegistry& instance();

    struct InstructionInfo {
        std::string mnemonic;
        Opcode opcode;
        InstructionEncoding encoding;
        InstructionFormat format;
        uint16_t func;
    };

    void registerInstruction(const std::string& mnemonic, Opcode opcode,
    InstructionFormat format, uint16_t func = 0);
    // Overload that allows explicit encoding specification (e.g., for Compressed16)
    void registerInstruction(const std::string& mnemonic, Opcode opcode,
    InstructionFormat format, uint16_t func, InstructionEncoding encoding);
    
    std::optional<InstructionInfo> getInfoByMnemonic(const std::string& mnemonic) const;
    std::optional<InstructionInfo> getInfoByOpcode(Opcode opcode, uint16_t func = 0) const;
    std::optional<InstructionInfo> getCompressedInfoByOpcode(Opcode opcode, uint16_t func = 0) const;
    
    const std::unordered_map<std::string, InstructionInfo>& getAllInfo() const { return mnemonic_to_info_; }

private:
    InstructionRegistry();
    std::unordered_map<std::string, InstructionInfo> mnemonic_to_info_;
    // Secondary index for decoding: opcode -> (func -> info)
    std::unordered_map<uint16_t, std::unordered_map<uint16_t, InstructionInfo>> opcode_func_to_info_;
    std::unordered_map<uint16_t, std::unordered_map<uint16_t, InstructionInfo>> opcode_func_to_compressed_info_;
};

}

#endif
