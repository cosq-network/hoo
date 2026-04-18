#ifndef HVM_H_INSTRUCTION_H
#define HVM_H_INSTRUCTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>

namespace hvm {

enum class Opcode : uint16_t {
    NOP = 0x00,
    MOV = 0x01,
    MOVI = 0x02,
    MOVZ = 0x03,
    LUI = 0x04,
    ADDI = 0x05,
    SUBI = 0x06,
    NEG = 0x07,
    XCHG = 0x08,
    ADD = 0x10,
    MULI = 0x11,
    DIVI = 0x12,
    SHL = 0x13,
    SHLI = 0x14,
    AND = 0x20,
    NOT = 0x21,
    ANDI = 0x22,
    ORI = 0x23,
    XORI = 0x24,
    CLZ = 0x25,
    CTZ = 0x26,
    POPCNT = 0x27,
    FADD = 0x30,
    FSQRT = 0x31,
    FABS = 0x32,
    FNEG = 0x33,
    FADD32 = 0x34,
    FCVT = 0x35,
    CMPEQ = 0x40,
    FCMPEQ = 0x41,
    SET = 0x42,
    BEQ = 0x50,
    BNE = 0x51,
    BLT = 0x52,
    BLE = 0x53,
    BGT = 0x54,
    BGE = 0x55,
    BLTU = 0x56,
    BGEU = 0x57,
    JMP = 0x60,
    JAL = 0x61,
    JALR = 0x62,
    RET = 0x63,
    LD_B = 0x70,
    LD_BU = 0x71,
    LD_H = 0x72,
    LD_HU = 0x73,
    LD_W = 0x74,
    LD_WU = 0x75,
    LD_D = 0x76,
    LD_X = 0x77,
    ST_B = 0x78,
    ST_H = 0x79,
    ST_W = 0x7A,
    ST_D = 0x7B,
    ST_X = 0x7C,
    LDA = 0x7D,
    PUSH = 0x7E,
    POP = 0x7F,
    ENTER = 0x80,
    LEAVE = 0x81,
    ADJSP = 0x82,
    FRAME = 0x83,
    STRNEW = 0x84,
    STRNEWB = 0x85,
    STRLEN = 0x86,
    STREMPTY = 0x87,
    STRGET = 0x88,
    STRSET = 0x89,
    STRAPPEND = 0x8A,
    STRPOP = 0x8B,
    STRCMP = 0x8C,
    STRCMPN = 0x8D,
    STREQUAL = 0x8E,
    STRSTART = 0x8F,
    STREND = 0x90,
    STRCHR = 0x91,
    STRRCHR = 0x92,
    STRFIND = 0x93,
    STRRFIND = 0x94,
    STRCONTAINS = 0x95,
    STRSUB = 0x96,
    STRSLICE = 0x97,
    STRSPLIT = 0x98,
    STRJOIN = 0x99,
    STREPEAT = 0x9A,
    STRREV = 0x9B,
    STRUPPER = 0x9C,
    STRLOWER = 0x9D,
    STRTRIM = 0x9E,
    STRLTRIM = 0x9F,
    STRRTRIM = 0xA0,
    STRPAD = 0xA1,
    STRTOI = 0xA2,
    STRTOD = 0xA3,
    ITOSTR = 0xA4,
    DTOSTR = 0xA5,
    STRENCODE = 0xA6,
    STRDECODE = 0xA7,
    NEW = 0xA8,
    NEWA = 0xA9,
    LDF = 0xAA,
    STF = 0xAB,
    LDELEM = 0xAC,
    STELEM = 0xAD,
    ARRAYLEN = 0xAE,
    INSTANCEOF = 0xAF,
    CHECKCAST = 0xB0,
    MONITORENTER = 0xB1,
    MONITOREXIT = 0xB2,
    GC = 0xB3,
    CALL = 0xB4,
    CALLI = 0xB5,
    TAILCALL = 0xB6,
    CALLVIRT = 0xB7,
    CALLINTF = 0xB8,
    IMPORT = 0xB9,
    LOADMOD = 0xBA,
    RESOLVE = 0xBB,
    THCREATE = 0xC0,
    THJOIN = 0xC1,
    THEXIT = 0xC2,
    THID = 0xC3,
    THYIELD = 0xC4,
    THWAIT = 0xC5,
    MUTEXINI = 0xC6,
    MUTEXLCK = 0xC7,
    MUTEXULK = 0xC8,
    MUTEXDL = 0xC9,
    CONDNWI = 0xCA,
    CONDSIG = 0xCB,
    CONDBRO = 0xCC,
    CONDWT = 0xCD,
    CONDDST = 0xCE,
    SPININIT = 0xCF,
    SPINLCK = 0xD0,
    SPINULK = 0xD1,
    BARRSET = 0xD2,
    BARRWT = 0xD3,
    ATOMADD = 0xE0,
    ATOMSUB = 0xE1,
    ATOMCAS = 0xE2,
    ATOMLD = 0xE3,
    ATOMST = 0xE4,
    TLSALLOC = 0xE5,
    TLSGET = 0xE6,
    TLSSET = 0xE7,
    TLSFREE = 0xE8,
    SEXT_B = 0xF0,
    SEXT_H = 0xF1,
    SEXT_W = 0xF2,
    ZEXT_B = 0xF3,
    ZEXT_H = 0xF4,
    ZEXT_W = 0xF5,
    TRUNC = 0xF6,
    REINTERPRET = 0xF7,
    VADD = 0x100,
    VSUB = 0x100,
    VMUL = 0x100,
    VDOT = 0x101,
    VLOAD = 0x102,
    VSTORE = 0x103,
    VSHUF = 0x104,
    VSPLAT = 0x105,
    VEXTRACT = 0x106,
    VINSERT = 0x107,
    VCMPEQ = 0x108,
    VCMPNE = 0x108,
    VCMPLT = 0x108,
    VCMPLE = 0x108,
    VREDUCE = 0x109,
    VFMA = 0x10A,
    VFMS = 0x10A,
    TRY = 0x110,
    THROW = 0x111,
    THROWV = 0x112,
    CATCH = 0x113,
    FINALLY = 0x114,
    RETHROW = 0x115,
    EXCINFO = 0x116,
    ENDFIN = 0x117,
    DI = 0x118,
    EI = 0x119,
    INT = 0x11A,
    IRET = 0x11B,
    SETINT = 0x11C,
    GETINT = 0x11D,
    MASKINT = 0x11E,
    UNMASKINT = 0x11F,
    CALLHOST = 0x120,
    CALLHOSTV = 0x121,
    CALLNATIVE = 0x122,
    PREPCALL = 0x123,
    FINISHCA = 0x124,
    LOADLIB = 0x125,
    FREELIB = 0x126,
    GETSYM = 0x127,
    GETFUNC = 0x128,
    I2PTR = 0x129,
    PTR2I = 0x12A,
    REINTERP = 0x12B,
    ADDR2FUNC = 0x12C,
    FUNC2ADDR = 0x12D,
    SYSCALL = 0x130,
    TRAP = 0x131,
    DEBUG = 0x132,
    RDCOUNT = 0x133,
    BARRIER = 0x134,
    BREAKPOINT = 0x135,
    SINGLESTEP = 0x136,
    GETREGS = 0x137,
    SETREGS = 0x138,
    GETFPOFF = 0x139,
    UNKNOWN = 0xFF
};

enum class InstructionFormat {
    R,
    I,
    B,
    J,
    RI,
    R_EXT,
    I_EXT,
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
    int32_t offset;
};

struct OperandsRI {
    uint8_t rd;
    uint8_t rd2;
    uint8_t rs;
    uint16_t imm;
};

using Operands = std::variant<OperandsR, OperandsI, OperandsB, OperandsJ, OperandsRI>;

class HInstruction {
public:
    HInstruction();
    explicit HInstruction(Opcode opcode);
    HInstruction(Opcode opcode, const Operands& operands);
    ~HInstruction() = default;

    static std::unique_ptr<HInstruction> decode(const std::vector<uint8_t>& bytes);
    static std::unique_ptr<HInstruction> decode(const uint32_t word);
    static std::unique_ptr<HInstruction> decode64(const std::vector<uint8_t>& bytes);

    std::vector<uint8_t> encode() const;
    uint32_t encode32() const;
    std::vector<uint8_t> encode64() const;

    void setOpcode(Opcode opcode) { opcode_ = opcode; }
    Opcode getOpcode() const { return opcode_; }

    void setOperands(const Operands& operands) { operands_ = operands; }
    const Operands& getOperands() const { return operands_; }

    void setFormat(InstructionFormat format) { format_ = format; }
    InstructionFormat getFormat() const { return format_; }

    void setMnemonic(const std::string& name) { mnemonic_ = name; }
    std::string getMnemonic() const { return mnemonic_; }

    bool isExtended() const { return extended_; }
    void setExtended(bool ext) { extended_ = ext; }

    uint8_t getSize() const { return isExtended() ? 8 : 4; }

    std::string toString() const;
    std::string toAssembly() const;

    static std::string opcodeToString(Opcode opcode);
    static Opcode stringToOpcode(const std::string& name);
    static std::optional<InstructionFormat> getFormatForOpcode(Opcode opcode);

    static bool validateRegister(uint8_t reg);
    static bool validateImmediate(int64_t value, int bits);

    static const std::unordered_map<Opcode, std::string>& getMnemonicMap();
    static const std::unordered_map<std::string, Opcode>& getOpcodeMap();

    static constexpr uint8_t FORMAT_R = 0;
    static constexpr uint8_t FORMAT_I = 1;
    static constexpr uint8_t FORMAT_B = 2;
    static constexpr uint8_t FORMAT_J = 3;
    static constexpr uint8_t FORMAT_RI = 4;

private:
    Opcode opcode_;
    Operands operands_;
    InstructionFormat format_;
    std::string mnemonic_;
    bool extended_;

    static std::string formatOperands(const HInstruction& inst);
};

class InstructionRegistry {
public:
    static InstructionRegistry& instance();

    void registerInstruction(Opcode opcode, const std::string& mnemonic,
                            InstructionFormat format, uint8_t func = 0);
    std::string getMnemonic(Opcode opcode) const;
    Opcode getOpcode(const std::string& mnemonic) const;
    InstructionFormat getFormat(Opcode opcode) const;

    struct InstructionInfo {
        std::string mnemonic;
        InstructionFormat format;
        uint8_t func;
    };

    std::optional<InstructionInfo> getInfo(Opcode opcode) const;
    const std::unordered_map<Opcode, InstructionInfo>& getAllInfo() const { return opcode_to_info_; }

private:
    InstructionRegistry();
    std::unordered_map<Opcode, InstructionInfo> opcode_to_info_;
    std::unordered_map<std::string, Opcode> name_to_opcode_;
};

}

#endif