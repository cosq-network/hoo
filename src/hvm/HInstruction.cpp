#include "hvm/HInstruction.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace hvm {

HInstruction::HInstruction()
    : opcode_(Opcode::NOP)
    , operands_(OperandsR{0, 0, 0, 0})
    , format_(InstructionFormat::R)
    , mnemonic_("nop")
    , extended_(false) {
}

HInstruction::HInstruction(Opcode opcode)
    : opcode_(opcode)
    , format_(getFormatForOpcode(opcode).value_or(InstructionFormat::R))
    , extended_(false) {
    
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

HInstruction::HInstruction(Opcode opcode, const Operands& operands)
    : opcode_(opcode)
    , operands_(operands)
    , format_(getFormatForOpcode(opcode).value_or(InstructionFormat::R))
    , extended_(false) {
    mnemonic_ = opcodeToString(opcode);
}

std::unique_ptr<HInstruction> HInstruction::decode(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 4) {
        return nullptr;
    }
    
    auto inst = std::make_unique<HInstruction>();
    
    uint8_t opcode_byte = bytes[0];
    
    if (opcode_byte == 0xFF) {
        inst->opcode_ = Opcode::UNKNOWN;
        inst->format_ = InstructionFormat::UNKNOWN;
        inst->extended_ = false;
        return inst;
    }
    
    inst->opcode_ = static_cast<Opcode>(opcode_byte);
    inst->format_ = getFormatForOpcode(inst->opcode_).value_or(InstructionFormat::R);
    inst->mnemonic_ = opcodeToString(inst->opcode_);
    
    if (inst->format_ == InstructionFormat::R || inst->format_ == InstructionFormat::R_EXT) {
        OperandsR ops;
        ops.rd = bytes[1];
        ops.rs1 = bytes[2];
        ops.rs2 = bytes[3];
        ops.func = (bytes[2] << 8) | bytes[3];
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::I || inst->format_ == InstructionFormat::I_EXT) {
        OperandsI ops;
        ops.rd = bytes[1];
        ops.rs = bytes[2];
        int16_t imm = (static_cast<int16_t>(bytes[3]) << 8) | bytes[2];
        ops.imm15 = imm;
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::B) {
        OperandsB ops;
        ops.rs1 = bytes[1];
        ops.rs2 = bytes[2];
        int16_t imm = (static_cast<int16_t>(bytes[3]) << 8) | bytes[2];
        ops.imm15 = imm;
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::J) {
        OperandsJ ops;
        ops.rd = bytes[1];
        ops.offset = (static_cast<int32_t>(bytes[3]) << 16) | (static_cast<int32_t>(bytes[2]) << 8) | bytes[1];
        inst->operands_ = ops;
    } else {
        inst->operands_ = OperandsR{0, 0, 0, 0};
    }
    
    return inst;
}

std::unique_ptr<HInstruction> HInstruction::decode(const uint32_t word) {
    auto inst = std::make_unique<HInstruction>();
    
    uint8_t opcode_byte = (word >> 24) & 0xFF;
    
    if (opcode_byte == 0xFF) {
        inst->opcode_ = Opcode::UNKNOWN;
        inst->format_ = InstructionFormat::UNKNOWN;
        inst->extended_ = false;
        return inst;
    }
    
    inst->opcode_ = static_cast<Opcode>(opcode_byte);
    inst->format_ = getFormatForOpcode(inst->opcode_).value_or(InstructionFormat::R);
    inst->mnemonic_ = opcodeToString(inst->opcode_);
    
    if (inst->format_ == InstructionFormat::R || inst->format_ == InstructionFormat::R_EXT) {
        OperandsR ops;
        ops.rd = (word >> 16) & 0xFF;
        ops.rs1 = (word >> 8) & 0xFF;
        ops.rs2 = word & 0xFF;
        ops.func = (word >> 8) & 0xFFF;
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::I || inst->format_ == InstructionFormat::I_EXT) {
        OperandsI ops;
        ops.rd = (word >> 16) & 0xFF;
        ops.rs = (word >> 8) & 0xFF;
        ops.imm15 = static_cast<int16_t>((word & 0xFFFF) | ((word & 0x8000) ? 0xFFFF0000 : 0));
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::B) {
        OperandsB ops;
        ops.rs1 = (word >> 16) & 0xFF;
        ops.rs2 = (word >> 8) & 0xFF;
        ops.imm15 = static_cast<int16_t>((word & 0xFFFF) | ((word & 0x8000) ? 0xFFFF0000 : 0));
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::J) {
        OperandsJ ops;
        ops.rd = (word >> 16) & 0xFF;
        ops.offset = static_cast<int32_t>(word & 0xFFFFFF);
        inst->operands_ = ops;
    } else {
        inst->operands_ = OperandsR{0, 0, 0, 0};
    }
    
    return inst;
}

std::unique_ptr<HInstruction> HInstruction::decode64(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 8) {
        return nullptr;
    }
    
    auto inst = std::make_unique<HInstruction>();
    inst->extended_ = true;
    
    uint8_t firstByte = bytes[0];
    uint16_t opcodeVal;
    
    if (firstByte == 0xFF) {
        opcodeVal = *reinterpret_cast<const uint16_t*>(bytes.data() + 1);
    } else {
        opcodeVal = firstByte;
    }
    
    if (opcodeVal == 0xFFFF || opcodeVal == 0xFF) {
        inst->opcode_ = Opcode::UNKNOWN;
        inst->format_ = InstructionFormat::UNKNOWN;
        return inst;
    }
    
    inst->opcode_ = static_cast<Opcode>(opcodeVal);
    inst->format_ = getFormatForOpcode(inst->opcode_).value_or(InstructionFormat::R_EXT);
    inst->mnemonic_ = opcodeToString(inst->opcode_);
    
    if (inst->format_ == InstructionFormat::R || inst->format_ == InstructionFormat::R_EXT) {
        OperandsR ops;
        ops.rd = bytes[3];
        ops.rs1 = bytes[4];
        ops.rs2 = bytes[5];
        ops.func = *reinterpret_cast<const uint16_t*>(bytes.data() + 6);
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::I || inst->format_ == InstructionFormat::I_EXT) {
        OperandsI ops;
        ops.rd = bytes[3];
        ops.rs = bytes[4];
        ops.imm15 = *reinterpret_cast<const int16_t*>(bytes.data() + 6);
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::B) {
        OperandsB ops;
        ops.rs1 = bytes[3];
        ops.rs2 = bytes[4];
        ops.imm15 = *reinterpret_cast<const int16_t*>(bytes.data() + 6);
        inst->operands_ = ops;
    } else if (inst->format_ == InstructionFormat::J || inst->format_ == InstructionFormat::RI) {
        if (inst->format_ == InstructionFormat::RI) {
            OperandsRI ops;
            ops.rd = bytes[3];
            ops.rd2 = bytes[4];
            ops.rs = bytes[5];
            ops.imm = *reinterpret_cast<const uint16_t*>(bytes.data() + 6);
            inst->operands_ = ops;
        } else {
            OperandsJ ops;
            ops.rd = bytes[3];
            ops.offset = *reinterpret_cast<const int32_t*>(bytes.data() + 4);
            inst->operands_ = ops;
        }
    } else {
        inst->operands_ = OperandsR{0, 0, 0, 0};
    }
    
    return inst;
}

std::vector<uint8_t> HInstruction::encode() const {
    std::vector<uint8_t> bytes(4, 0);
    bytes[0] = static_cast<uint8_t>(opcode_);
    
    if (std::holds_alternative<OperandsR>(operands_)) {
        const auto& ops = std::get<OperandsR>(operands_);
        bytes[1] = ops.rd;
        bytes[2] = ops.rs1;
        bytes[3] = ops.rs2;
} else if (std::holds_alternative<OperandsI>(operands_)) {
        const auto& ops = std::get<OperandsI>(operands_);
        bytes[1] = ops.rd;
        bytes[2] = ops.rs;
        *reinterpret_cast<int16_t*>(bytes.data() + 2) = ops.imm15;
    } else if (std::holds_alternative<OperandsB>(operands_)) {
        const auto& ops = std::get<OperandsB>(operands_);
        bytes[1] = ops.rs1;
        bytes[2] = ops.rs2;
        *reinterpret_cast<int16_t*>(bytes.data() + 2) = ops.imm15;
    } else if (std::holds_alternative<OperandsJ>(operands_)) {
        const auto& ops = std::get<OperandsJ>(operands_);
        bytes[1] = ops.rd;
        *reinterpret_cast<int32_t*>(bytes.data() + 1) = ops.offset;
    } else if (std::holds_alternative<OperandsRI>(operands_)) {
        const auto& ops = std::get<OperandsRI>(operands_);
        bytes[1] = ops.rd;
        bytes[2] = ops.rd2;
        bytes[3] = ops.rs;
    }
    
    return bytes;
}

uint32_t HInstruction::encode32() const {
    uint32_t word = static_cast<uint32_t>(opcode_) << 24;
    
    if (std::holds_alternative<OperandsR>(operands_)) {
        const auto& ops = std::get<OperandsR>(operands_);
        word |= (static_cast<uint32_t>(ops.rd) << 16);
        word |= (static_cast<uint32_t>(ops.rs1) << 8);
        word |= ops.rs2;
    } else if (std::holds_alternative<OperandsI>(operands_)) {
        const auto& ops = std::get<OperandsI>(operands_);
        word |= (static_cast<uint32_t>(ops.rd) << 16);
        word |= (static_cast<uint32_t>(ops.rs) << 8);
        word |= (static_cast<uint32_t>(ops.imm15) & 0xFFFF);
    } else if (std::holds_alternative<OperandsB>(operands_)) {
        const auto& ops = std::get<OperandsB>(operands_);
        word |= (static_cast<uint32_t>(ops.rs1) << 16);
        word |= (static_cast<uint32_t>(ops.rs2) << 8);
        word |= (static_cast<uint32_t>(ops.imm15) & 0xFFFF);
    } else if (std::holds_alternative<OperandsJ>(operands_)) {
        const auto& ops = std::get<OperandsJ>(operands_);
        word |= (static_cast<uint32_t>(ops.rd) << 16);
        word |= (static_cast<uint32_t>(ops.offset) & 0xFFFFFF);
    } else if (std::holds_alternative<OperandsRI>(operands_)) {
        const auto& ops = std::get<OperandsRI>(operands_);
        word |= (static_cast<uint32_t>(ops.rd) << 16);
        word |= (static_cast<uint32_t>(ops.rd2) << 8);
        word |= ops.rs;
    }
    
    return word;
}

std::vector<uint8_t> HInstruction::encode64() const {
    std::vector<uint8_t> bytes(8, 0);
    
    uint16_t opcodeVal = static_cast<uint16_t>(opcode_);
    if (opcodeVal > 0xFF) {
        bytes[0] = 0xFF;
        *reinterpret_cast<uint16_t*>(bytes.data() + 1) = opcodeVal;
    } else {
        bytes[0] = static_cast<uint8_t>(opcode_);
    }
    bytes[2] = 0xFF;
    
    if (std::holds_alternative<OperandsR>(operands_)) {
        const auto& ops = std::get<OperandsR>(operands_);
        bytes[3] = ops.rd;
        bytes[4] = ops.rs1;
        bytes[5] = ops.rs2;
        *reinterpret_cast<uint16_t*>(bytes.data() + 6) = ops.func;
    } else if (std::holds_alternative<OperandsI>(operands_)) {
        const auto& ops = std::get<OperandsI>(operands_);
        bytes[3] = ops.rd;
        bytes[4] = ops.rs;
        *reinterpret_cast<int16_t*>(bytes.data() + 6) = ops.imm15;
    } else if (std::holds_alternative<OperandsB>(operands_)) {
        const auto& ops = std::get<OperandsB>(operands_);
        bytes[3] = ops.rs1;
        bytes[4] = ops.rs2;
        *reinterpret_cast<int16_t*>(bytes.data() + 6) = ops.imm15;
    } else if (std::holds_alternative<OperandsJ>(operands_)) {
        const auto& ops = std::get<OperandsJ>(operands_);
        bytes[3] = ops.rd;
        *reinterpret_cast<int32_t*>(bytes.data() + 4) = ops.offset;
    } else if (std::holds_alternative<OperandsRI>(operands_)) {
        const auto& ops = std::get<OperandsRI>(operands_);
        bytes[3] = ops.rd;
        bytes[4] = ops.rd2;
        bytes[5] = ops.rs;
        *reinterpret_cast<uint16_t*>(bytes.data() + 6) = ops.imm;
    }
    
    return bytes;
}

std::string HInstruction::toString() const {
    std::ostringstream oss;
    oss << "HInstruction(" << mnemonic_ << ", " << toAssembly() << ")";
    return oss.str();
}

std::string HInstruction::toAssembly() const {
    std::ostringstream oss;
    oss << mnemonic_ << " ";
    
    switch (format_) {
        case InstructionFormat::R:
        case InstructionFormat::R_EXT:
            if (std::holds_alternative<OperandsR>(operands_)) {
                const auto& ops = std::get<OperandsR>(operands_);
                oss << "r" << static_cast<int>(ops.rd) 
                   << ", r" << static_cast<int>(ops.rs1)
                   << ", r" << static_cast<int>(ops.rs2);
            }
            break;
        case InstructionFormat::I:
        case InstructionFormat::I_EXT:
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
                oss << "r" << static_cast<int>(ops.rd)
                   << ", " << ops.offset;
            }
            break;
        case InstructionFormat::RI:
            if (std::holds_alternative<OperandsRI>(operands_)) {
                const auto& ops = std::get<OperandsRI>(operands_);
                oss << "r" << static_cast<int>(ops.rd)
                   << ", r" << static_cast<int>(ops.rd2)
                   << ", r" << static_cast<int>(ops.rs)
                   << ", " << ops.imm;
            }
            break;
        default:
            oss << "(invalid operands)";
            break;
    }
    
    return oss.str();
}

std::string HInstruction::opcodeToString(Opcode opcode) {
    return getMnemonicMap().at(opcode);
}

Opcode HInstruction::stringToOpcode(const std::string& name) {
    auto it = getOpcodeMap().find(name);
    if (it != getOpcodeMap().end()) {
        return it->second;
    }
    return Opcode::UNKNOWN;
}

std::optional<InstructionFormat> HInstruction::getFormatForOpcode(Opcode opcode) {
    return InstructionRegistry::instance().getFormat(opcode);
}

bool HInstruction::validateRegister(uint8_t reg) {
    return reg < 32;
}

bool HInstruction::validateImmediate(int64_t value, int bits) {
    int64_t min = -(1LL << (bits - 1));
    int64_t max = (1LL << (bits - 1)) - 1;
    return value >= min && value <= max;
}

const std::unordered_map<Opcode, std::string>& HInstruction::getMnemonicMap() {
    static std::unordered_map<Opcode, std::string> map = []() {
        std::unordered_map<Opcode, std::string> m;
        for (const auto& [opcode, info] : InstructionRegistry::instance().getAllInfo()) {
            m[opcode] = info.mnemonic;
        }
        return m;
    }();
    return map;
}

const std::unordered_map<std::string, Opcode>& HInstruction::getOpcodeMap() {
    static std::unordered_map<std::string, Opcode> map = []() {
        std::unordered_map<std::string, Opcode> m;
        for (const auto& [opcode, info] : InstructionRegistry::instance().getAllInfo()) {
            m[info.mnemonic] = opcode;
        }
        return m;
    }();
    return map;
}

std::string HInstruction::formatOperands(const HInstruction& inst) {
    return inst.toAssembly();
}

InstructionRegistry& InstructionRegistry::instance() {
    static InstructionRegistry registry;
    return registry;
}

InstructionRegistry::InstructionRegistry() {
    auto reg = [&](Opcode opcode, const std::string& mnemonic, InstructionFormat format, uint8_t func = 0) {
        opcode_to_info_[opcode] = {mnemonic, format, func};
        name_to_opcode_[mnemonic] = opcode;
    };
    
    reg(Opcode::NOP, "nop", InstructionFormat::R);
    reg(Opcode::MOV, "mov", InstructionFormat::R);
    reg(Opcode::MOVI, "movi", InstructionFormat::I);
    reg(Opcode::MOVZ, "movz", InstructionFormat::R);
    reg(Opcode::LUI, "lui", InstructionFormat::I);
    reg(Opcode::ADDI, "addi", InstructionFormat::I);
    reg(Opcode::SUBI, "subi", InstructionFormat::I);
    reg(Opcode::NEG, "neg", InstructionFormat::R);
    reg(Opcode::XCHG, "xchg", InstructionFormat::R);
    reg(Opcode::ADD, "add", InstructionFormat::R);
    reg(Opcode::MULI, "muli", InstructionFormat::I);
    reg(Opcode::DIVI, "divi", InstructionFormat::I);
    reg(Opcode::SHL, "shl", InstructionFormat::R);
    reg(Opcode::SHLI, "shli", InstructionFormat::I);
    reg(Opcode::AND, "and", InstructionFormat::R);
    reg(Opcode::NOT, "not", InstructionFormat::R);
    reg(Opcode::ANDI, "andi", InstructionFormat::I);
    reg(Opcode::ORI, "ori", InstructionFormat::I);
    reg(Opcode::XORI, "xori", InstructionFormat::I);
    reg(Opcode::CLZ, "clz", InstructionFormat::R);
    reg(Opcode::CTZ, "ctz", InstructionFormat::R);
    reg(Opcode::POPCNT, "popcnt", InstructionFormat::R);
    reg(Opcode::FADD, "fadd", InstructionFormat::R);
    reg(Opcode::FSQRT, "fsqrt", InstructionFormat::R);
    reg(Opcode::FABS, "fabs", InstructionFormat::R);
    reg(Opcode::FNEG, "fneg", InstructionFormat::R);
    reg(Opcode::FADD32, "fadd32", InstructionFormat::R);
    reg(Opcode::FCVT, "fcvt", InstructionFormat::R);
    reg(Opcode::CMPEQ, "cmpeq", InstructionFormat::R);
    reg(Opcode::FCMPEQ, "fcmpeq", InstructionFormat::R);
    reg(Opcode::SET, "set", InstructionFormat::R);
    reg(Opcode::BEQ, "beq", InstructionFormat::B);
    reg(Opcode::BNE, "bne", InstructionFormat::B);
    reg(Opcode::BLT, "blt", InstructionFormat::B);
    reg(Opcode::BLE, "ble", InstructionFormat::B);
    reg(Opcode::BGT, "bgt", InstructionFormat::B);
    reg(Opcode::BGE, "bge", InstructionFormat::B);
    reg(Opcode::BLTU, "bltu", InstructionFormat::B);
    reg(Opcode::BGEU, "bgeu", InstructionFormat::B);
    reg(Opcode::JMP, "jmp", InstructionFormat::J);
    reg(Opcode::JAL, "jal", InstructionFormat::J);
    reg(Opcode::JALR, "jalr", InstructionFormat::I);
    reg(Opcode::RET, "ret", InstructionFormat::R);
    reg(Opcode::LD_B, "ld.b", InstructionFormat::I);
    reg(Opcode::LD_BU, "ld.bu", InstructionFormat::I);
    reg(Opcode::LD_H, "ld.h", InstructionFormat::I);
    reg(Opcode::LD_HU, "ld.hu", InstructionFormat::I);
    reg(Opcode::LD_W, "ld.w", InstructionFormat::I);
    reg(Opcode::LD_WU, "ld.wu", InstructionFormat::I);
    reg(Opcode::LD_D, "ld.d", InstructionFormat::I);
    reg(Opcode::LD_X, "ld.x", InstructionFormat::I);
    reg(Opcode::ST_B, "st.b", InstructionFormat::I);
    reg(Opcode::ST_H, "st.h", InstructionFormat::I);
    reg(Opcode::ST_W, "st.w", InstructionFormat::I);
    reg(Opcode::ST_D, "st.d", InstructionFormat::I);
    reg(Opcode::ST_X, "st.x", InstructionFormat::I);
    reg(Opcode::LDA, "lda", InstructionFormat::I);
    reg(Opcode::PUSH, "push", InstructionFormat::I);
    reg(Opcode::POP, "pop", InstructionFormat::I);
    reg(Opcode::ENTER, "enter", InstructionFormat::I);
    reg(Opcode::LEAVE, "leave", InstructionFormat::R);
    reg(Opcode::ADJSP, "adjsp", InstructionFormat::I);
    reg(Opcode::FRAME, "frame", InstructionFormat::I);
    reg(Opcode::STRNEW, "strnew", InstructionFormat::R);
    reg(Opcode::STRNEWB, "strnewb", InstructionFormat::R);
    reg(Opcode::STRLEN, "strlen", InstructionFormat::R);
    reg(Opcode::STREMPTY, "strempty", InstructionFormat::R);
    reg(Opcode::STRGET, "strget", InstructionFormat::R);
    reg(Opcode::STRSET, "strset", InstructionFormat::R);
    reg(Opcode::STRAPPEND, "strappend", InstructionFormat::R);
    reg(Opcode::STRPOP, "strpop", InstructionFormat::R);
    reg(Opcode::STRCMP, "strcmp", InstructionFormat::R);
    reg(Opcode::STRCMPN, "strcmpn", InstructionFormat::R);
    reg(Opcode::STREQUAL, "strequal", InstructionFormat::R);
    reg(Opcode::STRSTART, "strstart", InstructionFormat::R);
    reg(Opcode::STREND, "strend", InstructionFormat::R);
    reg(Opcode::STRCHR, "strchr", InstructionFormat::R);
    reg(Opcode::STRRCHR, "strrchr", InstructionFormat::R);
    reg(Opcode::STRFIND, "strfind", InstructionFormat::R);
    reg(Opcode::STRRFIND, "strrfind", InstructionFormat::R);
    reg(Opcode::STRCONTAINS, "strcontains", InstructionFormat::R);
    reg(Opcode::STRSUB, "strsub", InstructionFormat::R);
    reg(Opcode::STRSLICE, "strslice", InstructionFormat::R);
    reg(Opcode::STRSPLIT, "strsplit", InstructionFormat::R);
    reg(Opcode::STRJOIN, "strjoin", InstructionFormat::R);
    reg(Opcode::STREPEAT, "strepeat", InstructionFormat::R);
    reg(Opcode::STRREV, "strrev", InstructionFormat::R);
    reg(Opcode::STRUPPER, "strupper", InstructionFormat::R);
    reg(Opcode::STRLOWER, "strlower", InstructionFormat::R);
    reg(Opcode::STRTRIM, "strtrim", InstructionFormat::R);
    reg(Opcode::STRLTRIM, "strltrim", InstructionFormat::R);
    reg(Opcode::STRRTRIM, "strrtrim", InstructionFormat::R);
    reg(Opcode::STRPAD, "strpad", InstructionFormat::R);
    reg(Opcode::STRTOI, "strtoi", InstructionFormat::R);
    reg(Opcode::STRTOD, "strtod", InstructionFormat::R);
    reg(Opcode::ITOSTR, "itostr", InstructionFormat::R);
    reg(Opcode::DTOSTR, "dtostr", InstructionFormat::R);
    reg(Opcode::STRENCODE, "strencode", InstructionFormat::R);
    reg(Opcode::STRDECODE, "strdecode", InstructionFormat::R);
    reg(Opcode::NEW, "new", InstructionFormat::I);
    reg(Opcode::NEWA, "newa", InstructionFormat::I);
    reg(Opcode::LDF, "ldf", InstructionFormat::I);
    reg(Opcode::STF, "stf", InstructionFormat::I);
    reg(Opcode::LDELEM, "ldelem", InstructionFormat::R);
    reg(Opcode::STELEM, "stelem", InstructionFormat::R);
    reg(Opcode::ARRAYLEN, "arraylen", InstructionFormat::R);
    reg(Opcode::INSTANCEOF, "instanceof", InstructionFormat::I);
    reg(Opcode::CHECKCAST, "checkcast", InstructionFormat::I);
    reg(Opcode::MONITORENTER, "monitorenter", InstructionFormat::R);
    reg(Opcode::MONITOREXIT, "monitorexit", InstructionFormat::R);
    reg(Opcode::GC, "gc", InstructionFormat::R);
    reg(Opcode::CALL, "call", InstructionFormat::J);
    reg(Opcode::CALLI, "calli", InstructionFormat::I);
    reg(Opcode::TAILCALL, "tailcall", InstructionFormat::J);
    reg(Opcode::CALLVIRT, "callvirt", InstructionFormat::R);
    reg(Opcode::CALLINTF, "callintf", InstructionFormat::R);
    reg(Opcode::IMPORT, "import", InstructionFormat::I);
    reg(Opcode::LOADMOD, "loadmod", InstructionFormat::R);
    reg(Opcode::RESOLVE, "resolve", InstructionFormat::I);
    reg(Opcode::THCREATE, "thcreate", InstructionFormat::R);
    reg(Opcode::THJOIN, "thjoin", InstructionFormat::R);
    reg(Opcode::THEXIT, "thexit", InstructionFormat::I);
    reg(Opcode::THID, "thid", InstructionFormat::R);
    reg(Opcode::THYIELD, "thyield", InstructionFormat::R);
    reg(Opcode::THWAIT, "thwait", InstructionFormat::R);
    reg(Opcode::MUTEXINI, "mutexini", InstructionFormat::R);
    reg(Opcode::MUTEXLCK, "mutexlck", InstructionFormat::R);
    reg(Opcode::MUTEXULK, "mutexulk", InstructionFormat::R);
    reg(Opcode::MUTEXDL, "mutexdl", InstructionFormat::R);
    reg(Opcode::CONDNWI, "condnwi", InstructionFormat::R);
    reg(Opcode::CONDSIG, "condsig", InstructionFormat::R);
    reg(Opcode::CONDBRO, "condbro", InstructionFormat::R);
    reg(Opcode::CONDWT, "condwt", InstructionFormat::R);
    reg(Opcode::CONDDST, "conddst", InstructionFormat::R);
    reg(Opcode::SPININIT, "spininit", InstructionFormat::R);
    reg(Opcode::SPINLCK, "spinlck", InstructionFormat::R);
    reg(Opcode::SPINULK, "spinulk", InstructionFormat::R);
    reg(Opcode::BARRSET, "barrset", InstructionFormat::R);
    reg(Opcode::BARRWT, "barrwt", InstructionFormat::R);
    reg(Opcode::ATOMADD, "atomadd", InstructionFormat::R);
    reg(Opcode::ATOMSUB, "atomsub", InstructionFormat::R);
    reg(Opcode::ATOMCAS, "atomcas", InstructionFormat::R);
    reg(Opcode::ATOMLD, "atomld", InstructionFormat::R);
    reg(Opcode::ATOMST, "atomst", InstructionFormat::R);
    reg(Opcode::TLSALLOC, "tlsalloc", InstructionFormat::R);
    reg(Opcode::TLSGET, "tlsget", InstructionFormat::R);
    reg(Opcode::TLSSET, "tlsset", InstructionFormat::R);
    reg(Opcode::TLSFREE, "tlsfree", InstructionFormat::R);
    reg(Opcode::SEXT_B, "sext.b", InstructionFormat::R);
    reg(Opcode::SEXT_H, "sext.h", InstructionFormat::R);
    reg(Opcode::SEXT_W, "sext.w", InstructionFormat::R);
    reg(Opcode::ZEXT_B, "zext.b", InstructionFormat::R);
    reg(Opcode::ZEXT_H, "zext.h", InstructionFormat::R);
    reg(Opcode::ZEXT_W, "zext.w", InstructionFormat::R);
    reg(Opcode::TRUNC, "trunc", InstructionFormat::R);
    reg(Opcode::REINTERPRET, "reinterpret", InstructionFormat::R);
    reg(Opcode::VADD, "vadd", InstructionFormat::R);
    reg(Opcode::VSUB, "vsub", InstructionFormat::R);
    reg(Opcode::VMUL, "vmul", InstructionFormat::R);
    reg(Opcode::VDOT, "vdot", InstructionFormat::R);
    reg(Opcode::VLOAD, "vload", InstructionFormat::I);
    reg(Opcode::VSTORE, "vstore", InstructionFormat::I);
    reg(Opcode::VSHUF, "vshuf", InstructionFormat::R);
    reg(Opcode::VSPLAT, "vsplat", InstructionFormat::R);
    reg(Opcode::VEXTRACT, "vextract", InstructionFormat::R);
    reg(Opcode::VINSERT, "vinsert", InstructionFormat::RI);
    reg(Opcode::VCMPEQ, "vcmpeq", InstructionFormat::R);
    reg(Opcode::VCMPNE, "vcmpne", InstructionFormat::R);
    reg(Opcode::VCMPLT, "vcmplt", InstructionFormat::R);
    reg(Opcode::VCMPLE, "vcmple", InstructionFormat::R);
    reg(Opcode::VREDUCE, "vreduce", InstructionFormat::R);
    reg(Opcode::VFMA, "vfma", InstructionFormat::R);
    reg(Opcode::VFMS, "vfms", InstructionFormat::R);
    reg(Opcode::TRY, "try", InstructionFormat::I);
    reg(Opcode::THROW, "throw", InstructionFormat::I);
    reg(Opcode::THROWV, "throwv", InstructionFormat::R);
    reg(Opcode::CATCH, "catch", InstructionFormat::J);
    reg(Opcode::FINALLY, "finally", InstructionFormat::I);
    reg(Opcode::RETHROW, "rethrow", InstructionFormat::R);
    reg(Opcode::EXCINFO, "excinfo", InstructionFormat::R);
    reg(Opcode::ENDFIN, "endfin", InstructionFormat::R);
    reg(Opcode::DI, "di", InstructionFormat::R);
    reg(Opcode::EI, "ei", InstructionFormat::R);
    reg(Opcode::INT, "int", InstructionFormat::I);
    reg(Opcode::IRET, "iret", InstructionFormat::R);
    reg(Opcode::SETINT, "setint", InstructionFormat::I);
    reg(Opcode::GETINT, "getint", InstructionFormat::R);
    reg(Opcode::MASKINT, "maskint", InstructionFormat::R);
    reg(Opcode::UNMASKINT, "unmaskint", InstructionFormat::R);
    reg(Opcode::CALLHOST, "callhost", InstructionFormat::R);
    reg(Opcode::CALLHOSTV, "callhostv", InstructionFormat::R);
    reg(Opcode::CALLNATIVE, "callnative", InstructionFormat::R);
    reg(Opcode::PREPCALL, "prepcall", InstructionFormat::I);
    reg(Opcode::FINISHCA, "finishca", InstructionFormat::R);
    reg(Opcode::LOADLIB, "loadlib", InstructionFormat::I);
    reg(Opcode::FREELIB, "freelib", InstructionFormat::R);
    reg(Opcode::GETSYM, "getsym", InstructionFormat::R);
    reg(Opcode::GETFUNC, "getfunc", InstructionFormat::R);
    reg(Opcode::I2PTR, "i2ptr", InstructionFormat::R);
    reg(Opcode::PTR2I, "ptr2i", InstructionFormat::R);
    reg(Opcode::REINTERP, "reinterp", InstructionFormat::R);
    reg(Opcode::ADDR2FUNC, "addr2func", InstructionFormat::R);
    reg(Opcode::FUNC2ADDR, "func2addr", InstructionFormat::R);
    reg(Opcode::SYSCALL, "syscall", InstructionFormat::R);
    reg(Opcode::TRAP, "trap", InstructionFormat::R);
    reg(Opcode::DEBUG, "debug", InstructionFormat::I);
    reg(Opcode::RDCOUNT, "rdcount", InstructionFormat::R);
    reg(Opcode::BARRIER, "barrier", InstructionFormat::R);
    reg(Opcode::BREAKPOINT, "breakpoint", InstructionFormat::R);
    reg(Opcode::SINGLESTEP, "singlestep", InstructionFormat::R);
    reg(Opcode::GETREGS, "getregs", InstructionFormat::R);
    reg(Opcode::SETREGS, "setregs", InstructionFormat::R);
    reg(Opcode::GETFPOFF, "getfpoff", InstructionFormat::R);
    reg(Opcode::UNKNOWN, "unknown", InstructionFormat::UNKNOWN);
}

void InstructionRegistry::registerInstruction(Opcode opcode, const std::string& mnemonic,
                                    InstructionFormat format, uint8_t func) {
    opcode_to_info_[opcode] = {mnemonic, format, func};
    name_to_opcode_[mnemonic] = opcode;
}

std::string InstructionRegistry::getMnemonic(Opcode opcode) const {
    auto it = opcode_to_info_.find(opcode);
    if (it != opcode_to_info_.end()) {
        return it->second.mnemonic;
    }
    return "";
}

Opcode InstructionRegistry::getOpcode(const std::string& mnemonic) const {
    auto it = name_to_opcode_.find(mnemonic);
    if (it != name_to_opcode_.end()) {
        return it->second;
    }
    return Opcode::UNKNOWN;
}

InstructionFormat InstructionRegistry::getFormat(Opcode opcode) const {
    auto it = opcode_to_info_.find(opcode);
    if (it != opcode_to_info_.end()) {
        return it->second.format;
    }
    return InstructionFormat::UNKNOWN;
}

std::optional<InstructionRegistry::InstructionInfo> InstructionRegistry::getInfo(Opcode opcode) const {
    auto it = opcode_to_info_.find(opcode);
    if (it != opcode_to_info_.end()) {
        return it->second;
    }
    return std::nullopt;
}

}