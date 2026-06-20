#include "hvm/HVMInstruction.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace hvm {
namespace {

constexpr uint8_t kExtendedOpcodeEscape = 0xFE;

InstructionEncoding encodingForOpcode(Opcode opcode) {
    const uint16_t opcodeVal = static_cast<uint16_t>(opcode);
    if (opcodeVal >= 0x80U) {
        return InstructionEncoding::Escape32;
    }
    return InstructionEncoding::Base32;
}

void writeU16LE(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void writeU32LE(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
}

bool readU16LE(const std::vector<uint8_t>& in, size_t off, uint16_t& out) {
    if (off + 2 > in.size()) return false;
    out = static_cast<uint16_t>(in[off]) | (static_cast<uint16_t>(in[off + 1]) << 8U);
    return true;
}

bool readU32LE(const std::vector<uint8_t>& in, size_t off, uint32_t& out) {
    if (off + 4 > in.size()) return false;
    out = static_cast<uint32_t>(in[off]) |
          (static_cast<uint32_t>(in[off + 1]) << 8U) |
          (static_cast<uint32_t>(in[off + 2]) << 16U) |
          (static_cast<uint32_t>(in[off + 3]) << 24U);
    return true;
}

void encodeULEB128(uint32_t value, std::vector<uint8_t>& out) {
    do {
        uint8_t byte = static_cast<uint8_t>(value & 0x7FU);
        value >>= 7U;
        if (value != 0) {
            byte |= 0x80U;
        }
        out.push_back(byte);
    } while (value != 0);
}

bool decodeULEB128(const std::vector<uint8_t>& in, size_t start, uint32_t& value, size_t& bytesUsed) {
    value = 0;
    bytesUsed = 0;
    uint32_t shift = 0;

    while (start + bytesUsed < in.size() && bytesUsed < 5) {
        const uint8_t byte = in[start + bytesUsed];
        value |= (static_cast<uint32_t>(byte & 0x7FU) << shift);
        ++bytesUsed;
        if ((byte & 0x80U) == 0) {
            return true;
        }
        shift += 7U;
    }
    return false;
}

} // namespace

HVMInstruction::HVMInstruction()
    : opcode_(Opcode::NOP)
    , operands_(OperandsR{0, 0, 0, 0})
    , format_(InstructionFormat::R)
    , mnemonic_("nop") {
}

HVMInstruction::HVMInstruction(Opcode opcode)
    : opcode_(opcode)
    , format_(getFormatForOpcode(opcode).value_or(InstructionFormat::R)) {
    
    switch (format_) {
        case InstructionFormat::R:
            operands_ = OperandsR{0, 0, 0, 0};
            break;
        case InstructionFormat::I:
            operands_ = OperandsI{0, 0, 0};
            break;
        case InstructionFormat::B:
            operands_ = OperandsB{0, 0, 0};
            break;
        case InstructionFormat::J:
            operands_ = OperandsJ{0, 0};
            break;
        case InstructionFormat::RI:
            operands_ = OperandsRI{0, 0, 0, 0};
            break;
        default:
            operands_ = OperandsR{0, 0, 0, 0};
            break;
    }
    
    mnemonic_ = opcodeToString(opcode);
}

HVMInstruction::HVMInstruction(Opcode opcode, const Operands& operands)
    : opcode_(opcode)
    , operands_(operands)
    , format_(getFormatForOpcode(opcode).value_or(InstructionFormat::R)) {
    
    uint16_t func = 0;
    if (std::holds_alternative<OperandsR>(operands)) {
        func = std::get<OperandsR>(operands).func;
    }
    mnemonic_ = opcodeToString(opcode, func);
}

std::unique_ptr<HVMInstruction> HVMInstruction::decode(const std::vector<uint8_t>& bytes, size_t& bytesUsed, bool allowCompressed) {
    bytesUsed = 0;
    if (bytes.empty()) return nullptr;

    const uint8_t firstByte = bytes[0];
    // Check for 16‑bit compressed instructions (HVM‑C)
    if (allowCompressed && firstByte != kExtendedOpcodeEscape && (firstByte & 0xF0) == 0xF0) {
        // Compressed format: 4‑bit opcode, 4‑bit rd, 4‑bit rs1, 4‑bit imm4
        if (bytes.size() < 2) return nullptr;
        uint8_t secondByte = bytes[1];
        uint8_t opcode4 = (firstByte >> 0) & 0x0F; // lower 4 bits hold opcode
        uint8_t rd = (secondByte >> 4) & 0x0F;
        uint8_t rs1 = secondByte & 0x0F;
        uint8_t imm4 = (firstByte >> 4) & 0x0F; // upper 4 bits are immediate
        // Map 4‑bit opcode to full 8‑bit opcode space (simple identity for demo)
        Opcode opcode = static_cast<Opcode>(opcode4);
        auto info = InstructionRegistry::instance().getInfoByOpcode(opcode, 0);
        if (!info || info->encoding != InstructionEncoding::Compressed16) return nullptr;
        auto inst = std::make_unique<HVMInstruction>();
        inst->opcode_ = opcode;
        inst->format_ = info->format;
        inst->mnemonic_ = info->mnemonic;
        // For simplicity we treat this as an R‑type with immediate in func field
        inst->operands_ = OperandsR{rd, rs1, 0, static_cast<uint16_t>(imm4)};
        bytesUsed = 2;
        return inst;
    }

    if (firstByte == kExtendedOpcodeEscape) {
        uint32_t opcodeVal = 0;
        size_t opcodeBytes = 0;
        if (!decodeULEB128(bytes, 1, opcodeVal, opcodeBytes)) return nullptr;
        if (opcodeVal < 0x80U) {
            return nullptr;
        }
        
        // Payload always starts at offset 4 due to alignment padding
        if (8 > bytes.size()) return nullptr;

        uint32_t word = 0;
        if (!readU32LE(bytes, 4, word)) return nullptr;

        Opcode opcode = static_cast<Opcode>(opcodeVal);
        auto info = InstructionRegistry::instance().getInfoByOpcode(opcode, word & 0x3FF);
        if (!info) {
            // Fallback: try with func 0
            info = InstructionRegistry::instance().getInfoByOpcode(opcode, 0);
        }

        if (!info) return nullptr;
        if (info->encoding != InstructionEncoding::Escape32) return nullptr;

        auto inst = std::make_unique<HVMInstruction>();
        inst->opcode_ = opcode;
        inst->format_ = info->format;
        inst->mnemonic_ = info->mnemonic;

        switch (inst->format_) {
            case InstructionFormat::R:
                inst->operands_ = OperandsR{
                    static_cast<uint8_t>((word >> 20) & 0x1F),
                    static_cast<uint8_t>((word >> 15) & 0x1F),
                    static_cast<uint8_t>((word >> 10) & 0x1F),
                    static_cast<uint16_t>(word & 0x3FF)
                };
                break;
            case InstructionFormat::I: {
                int16_t imm = static_cast<int16_t>(word & 0x7FFF);
                if (imm & 0x4000) imm |= 0x8000;
                inst->operands_ = OperandsI{
                    static_cast<uint8_t>((word >> 20) & 0x1F),
                    static_cast<uint8_t>((word >> 15) & 0x1F),
                    imm
                };
                break;
            }
            case InstructionFormat::B: {
                int16_t imm = static_cast<int16_t>(word & 0x7FFF);
                if (imm & 0x4000) imm |= 0x8000;
                inst->operands_ = OperandsB{
                    static_cast<uint8_t>((word >> 20) & 0x1F),
                    static_cast<uint8_t>((word >> 15) & 0x1F),
                    imm
                };
                break;
            }
            case InstructionFormat::J: {
                int32_t imm = static_cast<int32_t>(word & 0xFFFFF);
                if (imm & 0x80000) imm |= 0xFFF00000;
                inst->operands_ = OperandsJ{
                    static_cast<uint8_t>((word >> 20) & 0x1F),
                    imm
                };
                break;
            }
            case InstructionFormat::RI:
                inst->operands_ = OperandsRI{
                    static_cast<uint8_t>((word >> 20) & 0x1F),
                    static_cast<uint8_t>((word >> 15) & 0x1F),
                    static_cast<uint8_t>((word >> 10) & 0x1F),
                    static_cast<uint16_t>(word & 0x3FF)
                };
                break;
            default:
                return nullptr;
        }

        bytesUsed = 8;
        return inst;
    }

    if (bytes.size() < 4) return nullptr;
    uint32_t word = 0;
    if (!readU32LE(bytes, 0, word)) return nullptr;
    auto inst = decode(word);
    if (inst) {
        bytesUsed = 4;
    }
    return inst;
}

std::unique_ptr<HVMInstruction> HVMInstruction::decode(const std::vector<uint8_t>& bytes) {
    size_t ignored;
    return decode(bytes, ignored);
}

std::unique_ptr<HVMInstruction> HVMInstruction::decode(const uint32_t word) {
    uint8_t opVal = (word >> 25) & 0x7F;
    Opcode opcode = static_cast<Opcode>(opVal);

    // Try to find the exact instruction info using opcode and potential func
    auto info = InstructionRegistry::instance().getInfoByOpcode(opcode, word & 0x3FF);
    if (!info) {
        // Fallback: try with func 0
        info = InstructionRegistry::instance().getInfoByOpcode(opcode, 0);
    }

    if (!info) {
        auto inst = std::make_unique<HVMInstruction>();
        inst->opcode_ = Opcode::UNKNOWN;
        inst->format_ = InstructionFormat::UNKNOWN;
        inst->mnemonic_ = "unknown";
        return inst;
    }

    auto inst = std::make_unique<HVMInstruction>();
    inst->opcode_ = opcode;
    inst->format_ = info->format;
    inst->mnemonic_ = info->mnemonic;

    switch (inst->format_) {
        case InstructionFormat::R:
            inst->operands_ = OperandsR{
                static_cast<uint8_t>((word >> 20) & 0x1F),
                static_cast<uint8_t>((word >> 15) & 0x1F),
                static_cast<uint8_t>((word >> 10) & 0x1F),
                static_cast<uint16_t>(word & 0x3FF)
            };
            // Re-sync mnemonic for R-format based on func
            inst->mnemonic_ = opcodeToString(opcode, std::get<OperandsR>(inst->operands_).func);
            break;
        case InstructionFormat::I: {
            int16_t imm = static_cast<int16_t>(word & 0x7FFF);
            if (imm & 0x4000) imm |= 0x8000;
            inst->operands_ = OperandsI{
                static_cast<uint8_t>((word >> 20) & 0x1F),
                static_cast<uint8_t>((word >> 15) & 0x1F),
                imm
            };
            break;
        }
        case InstructionFormat::B: {
            int16_t imm = static_cast<int16_t>(word & 0x7FFF);
            if (imm & 0x4000) imm |= 0x8000;
            inst->operands_ = OperandsB{
                static_cast<uint8_t>((word >> 20) & 0x1F),
                static_cast<uint8_t>((word >> 15) & 0x1F),
                imm
            };
            break;
        }
        case InstructionFormat::J: {
            int32_t imm = static_cast<int32_t>(word & 0xFFFFF);
            if (imm & 0x80000) imm |= 0xFFF00000;
            inst->operands_ = OperandsJ{
                static_cast<uint8_t>((word >> 20) & 0x1F),
                imm
            };
            break;
        }
        case InstructionFormat::RI:
            inst->operands_ = OperandsRI{
                static_cast<uint8_t>((word >> 20) & 0x1F),
                static_cast<uint8_t>((word >> 15) & 0x1F),
                static_cast<uint8_t>((word >> 10) & 0x1F),
                static_cast<uint16_t>(word & 0x3FF)
            };
            break;
        default:
            inst->format_ = InstructionFormat::UNKNOWN;
            break;
    }

    return inst;
}

std::vector<uint8_t> HVMInstruction::encode() const {
    const uint16_t opcodeVal = static_cast<uint16_t>(opcode_);
    // Opcodes >= 0x80 must be escaped because the base formats only have 7 bits for opcode.
    const bool forceExtended = opcodeVal >= 0x80;

    if (!forceExtended) {
        std::vector<uint8_t> bytes;
        writeU32LE(bytes, encode32());
        return bytes;
    }

    std::vector<uint8_t> bytes;
    bytes.reserve(8); // Always 8 bytes for aligned extended instructions
    bytes.push_back(kExtendedOpcodeEscape);
    encodeULEB128(opcodeVal, bytes);
    
    // Pad with zeros so the 32-bit payload starts at offset 4
    while (bytes.size() < 4) {
        bytes.push_back(0);
    }
    
    writeU32LE(bytes, encode32());
    return bytes;
}

uint32_t HVMInstruction::encode32() const {
    uint32_t opVal = static_cast<uint32_t>(opcode_) & 0x7F;
    uint32_t word = (opVal << 25);

    switch (format_) {
        case InstructionFormat::R: {
            const auto& ops = std::get<OperandsR>(operands_);
            word |= ((ops.rd & 0x1F) << 20);
            word |= ((ops.rs1 & 0x1F) << 15);
            word |= ((ops.rs2 & 0x1F) << 10);
            word |= (ops.func & 0x3FF);
            break;
        }
        case InstructionFormat::I: {
            const auto& ops = std::get<OperandsI>(operands_);
            word |= ((ops.rd & 0x1F) << 20);
            word |= ((ops.rs & 0x1F) << 15);
            word |= (static_cast<uint32_t>(ops.imm15) & 0x7FFF);
            break;
        }
        case InstructionFormat::B: {
            const auto& ops = std::get<OperandsB>(operands_);
            word |= ((ops.rs1 & 0x1F) << 20);
            word |= ((ops.rs2 & 0x1F) << 15);
            word |= (static_cast<uint32_t>(ops.imm15) & 0x7FFF);
            break;
        }
        case InstructionFormat::J: {
            const auto& ops = std::get<OperandsJ>(operands_);
            word |= ((ops.rd & 0x1F) << 20);
            word |= (static_cast<uint32_t>(ops.offset) & 0xFFFFF);
            break;
        }
        case InstructionFormat::RI: {
            const auto& ops = std::get<OperandsRI>(operands_);
            word |= ((ops.rd & 0x1F) << 20);
            word |= ((ops.rs1 & 0x1F) << 15);
            word |= ((ops.rs2 & 0x1F) << 10);
            word |= (ops.imm & 0x3FF);
            break;
        }
        default:
            break;
    }

    return word;
}

std::string HVMInstruction::toString() const {
    std::ostringstream oss;
    oss << "HVMInstruction(" << mnemonic_ << ", " << toAssembly() << ")";
    return oss.str();
}

std::string HVMInstruction::toAssembly() const {
    std::ostringstream oss;
    oss << mnemonic_ << " ";
    
    switch (format_) {
        case InstructionFormat::R:
            if (std::holds_alternative<OperandsR>(operands_)) {
                const auto& ops = std::get<OperandsR>(operands_);
                oss << "r" << static_cast<int>(ops.rd) 
                   << ", r" << static_cast<int>(ops.rs1)
                   << ", r" << static_cast<int>(ops.rs2);
            }
            break;
        case InstructionFormat::I:
            if (std::holds_alternative<OperandsI>(operands_)) {
                const auto& ops = std::get<OperandsI>(operands_);
                oss << "r" << static_cast<int>(ops.rd) 
                   << ", r" << static_cast<int>(ops.rs)
                   << ", " << ops.imm15;
            }
            break;
        case InstructionFormat::B:
            if (std::holds_alternative<OperandsB>(operands_)) {
                const auto& ops = std::get<OperandsB>(operands_);
                oss << "r" << static_cast<int>(ops.rs1)
                   << ", r" << static_cast<int>(ops.rs2)
                   << ", " << ops.imm15;
            }
            break;
        case InstructionFormat::J:
            if (std::holds_alternative<OperandsJ>(operands_)) {
                const auto& ops = std::get<OperandsJ>(operands_);
                if (mnemonic_ == "jmp" || mnemonic_ == "tailcall") {
                    oss << ops.offset;
                } else {
                    oss << "r" << static_cast<int>(ops.rd) << ", " << ops.offset;
                }
            }
            break;
        case InstructionFormat::RI:
            if (std::holds_alternative<OperandsRI>(operands_)) {
                const auto& ops = std::get<OperandsRI>(operands_);
                oss << "r" << static_cast<int>(ops.rd)
                   << ", r" << static_cast<int>(ops.rs1)
                   << ", r" << static_cast<int>(ops.rs2)
                   << ", " << ops.imm;
            }
            break;
        default:
            oss << "(invalid operands)";
            break;
    }
    
    return oss.str();
}

std::string HVMInstruction::opcodeToString(Opcode opcode, uint16_t func) {
    auto info = InstructionRegistry::instance().getInfoByOpcode(opcode, func);
    if (info) return info->mnemonic;
    auto compInfo = InstructionRegistry::instance().getCompressedInfoByOpcode(opcode, func);
    if (compInfo) return compInfo->mnemonic;
    return "unknown";
}

Opcode HVMInstruction::stringToOpcode(const std::string& name) {
    auto info = InstructionRegistry::instance().getInfoByMnemonic(name);
    if (info) return info->opcode;
    return Opcode::UNKNOWN;
}

std::optional<InstructionFormat> HVMInstruction::getFormatForOpcode(Opcode opcode) {
    auto info = InstructionRegistry::instance().getInfoByOpcode(opcode, 0);
    if (info) return info->format;
    return std::nullopt;
}

bool HVMInstruction::validateRegister(uint8_t reg) {
    return reg < 32;
}

bool HVMInstruction::validateImmediate(int64_t value, int bits) {
    int64_t min = -(1LL << (bits - 1));
    int64_t max = (1LL << (bits - 1)) - 1;
    return value >= min && value <= max;
}

InstructionRegistry& InstructionRegistry::instance() {
    static InstructionRegistry registry;
    return registry;
}

InstructionRegistry::InstructionRegistry() {
    auto reg = [&](const std::string& mnemonic, Opcode opcode, InstructionFormat format, uint16_t func = 0) {
        registerInstruction(mnemonic, opcode, format, func);
    };
    // Helper for compressed registration
    auto regCompressed = [&](const std::string& mnemonic, Opcode opcode, InstructionFormat format, uint16_t func = 0) {
        registerInstruction(mnemonic, opcode, format, func, InstructionEncoding::Compressed16);
    };
    
    // Core families from hvm_instruction_set.csv
    reg("nop",   Opcode::NOP,   InstructionFormat::R);
    reg("mov",   Opcode::MOV,   InstructionFormat::R);
    reg("retain", Opcode::RETAIN, InstructionFormat::R);
    reg("release", Opcode::RELEASE, InstructionFormat::R);
    reg("movz",  Opcode::MOVZ,  InstructionFormat::I);
    reg("lui",   Opcode::LUI,   InstructionFormat::I);
    reg("addi",  Opcode::ADDI,  InstructionFormat::I);
    
    // Arithmetic (0x10)
    reg("add",   Opcode::ARITH, InstructionFormat::R, 0);
    reg("sub",   Opcode::ARITH, InstructionFormat::R, 1);
    reg("mul",   Opcode::ARITH, InstructionFormat::R, 2);
    reg("div",   Opcode::ARITH, InstructionFormat::R, 5);
    reg("divu",  Opcode::ARITH, InstructionFormat::R, 6);
    reg("rem",   Opcode::ARITH, InstructionFormat::R, 7);
    
    // Compressed 16‑bit instructions (HVM‑C) – only those that satisfy the 4‑bit register/immediate constraint
    regCompressed("add.c", Opcode::ARITH, InstructionFormat::R, 0); // rd = rs1 + imm4
    regCompressed("sub.c", Opcode::ARITH, InstructionFormat::R, 1); // rd = rs1 - imm4
    regCompressed("ld.p.c", Opcode::LD_P, InstructionFormat::R);
    regCompressed("st.p.c", Opcode::ST_P, InstructionFormat::R);
    regCompressed("ret.c", Opcode::RET, InstructionFormat::R);

    // Shift (0x13)
    reg("shl",   Opcode::SHIFT, InstructionFormat::R, 0);
    reg("shr",   Opcode::SHIFT, InstructionFormat::R, 1);
    reg("sar",   Opcode::SHIFT, InstructionFormat::R, 2);
    
    // Logic (0x20)
    reg("and",   Opcode::LOGIC, InstructionFormat::R, 0);
    reg("or",    Opcode::LOGIC, InstructionFormat::R, 1);
    reg("xor",   Opcode::LOGIC, InstructionFormat::R, 2);
    reg("not",   Opcode::NOT,   InstructionFormat::R);
    
    // Float Arith (0x30)
    reg("fadd",  Opcode::FLOAT_ARITH, InstructionFormat::R, 0);
    reg("fsub",  Opcode::FLOAT_ARITH, InstructionFormat::R, 1);
    reg("fmul",  Opcode::FLOAT_ARITH, InstructionFormat::R, 2);
    reg("fdiv",  Opcode::FLOAT_ARITH, InstructionFormat::R, 3);
    
    // Comparisons (0x40, 0x41)
    reg("cmpeq", Opcode::CMP,   InstructionFormat::R, 0);
    reg("cmpne", Opcode::CMP,   InstructionFormat::R, 1);
    reg("cmplt", Opcode::CMP,   InstructionFormat::R, 2);
    reg("cmple", Opcode::CMP,   InstructionFormat::R, 3);
    reg("fcmpeq", Opcode::FCMP, InstructionFormat::R, 0);
    reg("fcmplt", Opcode::FCMP, InstructionFormat::R, 1);
    reg("fcmple", Opcode::FCMP, InstructionFormat::R, 2);
    
    // Branch/Jump
    reg("beq",   Opcode::BEQ,   InstructionFormat::B);
    reg("bne",   Opcode::BNE,   InstructionFormat::B);
    reg("blt",   Opcode::BLT,   InstructionFormat::B);
    reg("ble",   Opcode::BLE,   InstructionFormat::B);
    reg("jmp",   Opcode::JMP,   InstructionFormat::J);
    reg("jal",   Opcode::JAL,   InstructionFormat::J);
    reg("jalr",  Opcode::JALR,  InstructionFormat::I);
    reg("ret",   Opcode::RET,   InstructionFormat::R);
    
    // Memory
    reg("ld.b",  Opcode::LD_B,  InstructionFormat::I);
    reg("ld.bu", Opcode::LD_BU, InstructionFormat::I);
    reg("ld.h",  Opcode::LD_H,  InstructionFormat::I);
    reg("ld.hu", Opcode::LD_HU, InstructionFormat::I);
    reg("ld.w",  Opcode::LD_W,  InstructionFormat::I);
    reg("ld.wu", Opcode::LD_WU, InstructionFormat::I);
    reg("ld.d",  Opcode::LD_D,  InstructionFormat::I);
    reg("st.b",  Opcode::ST_B,  InstructionFormat::I);
    reg("st.h",  Opcode::ST_H,  InstructionFormat::I);
    reg("st.w",  Opcode::ST_W,  InstructionFormat::I);
    reg("st.d",  Opcode::ST_D,  InstructionFormat::I);
    reg("lda",   Opcode::LDA,   InstructionFormat::I);
    
    // Stack
    reg("push",  Opcode::PUSH,  InstructionFormat::R);
    reg("pop",   Opcode::POP,   InstructionFormat::R);
    reg("enter", Opcode::ENTER, InstructionFormat::I);
    reg("leave", Opcode::LEAVE, InstructionFormat::R);
    reg("adjsp", Opcode::ADJSP, InstructionFormat::I);
    reg("frame", Opcode::FRAME, InstructionFormat::I);
    
    // Calls
    reg("call",     Opcode::CALL,     InstructionFormat::J);
    reg("tailcall", Opcode::TAILCALL, InstructionFormat::J);
    
    // Hardware/System
    reg("syscall",  Opcode::SYSCALL,  InstructionFormat::I);
    reg("break",    Opcode::BREAK,    InstructionFormat::R);
    
    // Hardware loop
    reg("loop.set", Opcode::LOOP_SET, InstructionFormat::I);
    reg("loop.decbr", Opcode::LOOP_DECBR, InstructionFormat::B);

    // New HVM instruction sets
    reg("alloc.bump", Opcode::ALLOC_BUMP, InstructionFormat::I);
    reg("chk.b", Opcode::CHK_B, InstructionFormat::R);
    reg("ld.d.nz", Opcode::LD_D_NZ, InstructionFormat::I);
    reg("vsetvl", Opcode::VSETVL, InstructionFormat::R);
    reg("vld.v", Opcode::VECTOR_MEM, InstructionFormat::R, 0);
    reg("vst.v", Opcode::VECTOR_MEM, InstructionFormat::R, 1);
    reg("vadd.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 0);
    reg("vsub.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 2);
    reg("vmul.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 4);
    reg("vdiv.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 6);
}

void InstructionRegistry::registerInstruction(const std::string& mnemonic, Opcode opcode,
                                    InstructionFormat format, uint16_t func) {
    InstructionInfo info = {mnemonic, opcode, encodingForOpcode(opcode), format, func};
    mnemonic_to_info_[mnemonic] = info;
    opcode_func_to_info_[static_cast<uint16_t>(opcode)][func] = info;
}

// Overload allowing explicit encoding (e.g., for compressed 16‑bit instructions)
void InstructionRegistry::registerInstruction(const std::string& mnemonic, Opcode opcode,
                                    InstructionFormat format, uint16_t func, InstructionEncoding encoding) {
    InstructionInfo info = {mnemonic, opcode, encoding, format, func};
    mnemonic_to_info_[mnemonic] = info;
    if (encoding == InstructionEncoding::Compressed16) {
        opcode_func_to_compressed_info_[static_cast<uint16_t>(opcode)][func] = info;
    } else {
        opcode_func_to_info_[static_cast<uint16_t>(opcode)][func] = info;
    }
}

std::optional<InstructionRegistry::InstructionInfo> InstructionRegistry::getInfoByMnemonic(const std::string& mnemonic) const {
    auto it = mnemonic_to_info_.find(mnemonic);
    if (it != mnemonic_to_info_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<InstructionRegistry::InstructionInfo> InstructionRegistry::getInfoByOpcode(Opcode opcode, uint16_t func) const {
    auto it = opcode_func_to_info_.find(static_cast<uint16_t>(opcode));
    if (it != opcode_func_to_info_.end()) {
        auto it2 = it->second.find(func);
        if (it2 != it->second.end()) {
            return it2->second;
        }
    }
    return std::nullopt;
}

std::optional<InstructionRegistry::InstructionInfo> InstructionRegistry::getCompressedInfoByOpcode(Opcode opcode, uint16_t func) const {
    auto it = opcode_func_to_compressed_info_.find(static_cast<uint16_t>(opcode));
    if (it != opcode_func_to_compressed_info_.end()) {
        auto it2 = it->second.find(func);
        if (it2 != it->second.end()) {
            return it2->second;
        }
    }
    return std::nullopt;
}

} // namespace hvm
