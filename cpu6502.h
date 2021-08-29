#ifndef NES_CPU6502_H_
#define NES_CPU6502_H_

#include <unordered_map>

#include "common.h"
#include "mapper.h"
#include "ppu.h"
#include "memory_view.h"

// Implements the NES's MOS 6502 CPU.
class Cpu6502 {
  public:
    Cpu6502(const std::string& file_path);

    // Executes the next instruction.
    void RunCycle();

  private:
    // Resets the CPU state, loads the cartridge,
    // sets the next instruction baded on reset vector.
    void Reset(const std::string& file_path);
    void BuildInstructionSet();

    void LoadCartrtidgeFile(const std::string& file_path);
    // Loads an iNES 1.0 file
    void LoadNes1File(std::vector<uint8_t> bytes);

    // D has no effect. 
    enum class Flag {
      C = 0, Z = 1, I = 2, D = 3, V = 6, N = 7
    };
    bool GetFlag(Flag flag);
    void SetFlag(Flag flag, bool val);
    void SetPIgnoreB(uint8_t new_p);

    struct AddrVal {
      uint16_t addr = 0;  // unused for kImmediate
      uint8_t val = 0;    // value at addr
      bool page_crossed = false;
    };
    /// Addressing modes -- affects program_counter
    // These all return addresses except NextImmediate()
    uint8_t NextImmediate();
    uint16_t NextZeroPage();
    uint16_t NextZeroPageX();
    uint16_t NextZeroPageY();
    uint16_t NextAbsolute();
    uint16_t NextAbsoluteX(bool* page_crossed);
    uint16_t NextAbsoluteY(bool* page_crossed);
    uint16_t NextIndirectX();
    uint16_t NextIndirectY(bool* page_crossed);
    // Only for JMP. This doesn't appear to be documented in very many places, but the indirection
    // here cannot cross pages. So JMP ($2FF) should read $2FF and $200.
    uint16_t NextAbsoluteIndirect(); // only JMP
    // For branching, decodes next offset into an address.
    uint16_t NextRelativeAddr(bool* page_crossed);

    enum class AddressingMode {
      kImmediate,
      kZeroPage,
      kZeroPageX,
      kZeroPageY,
      kAbsolute,
      kAbsoluteX,
      kAbsoluteY,
      kIndirectX,
      kIndirectY,
      kAbsoluteIndirect,
      kRelative,
      kAccumulator,
      kNone // needed?
    };
    uint16_t NextAddr(AddressingMode mode, bool* page_crossed);
    AddrVal NextAddrVal(AddressingMode mode, bool unofficial=false);

    void PushStack(uint8_t val);
    void PushStack16(uint16_t val);
    uint8_t PopStack();
    uint16_t PopStack16();

    std::string AddrValString(AddrVal addrval, AddressingMode mode, bool is_jmp=false);
    void DbgMem();
    void DbgStack();
    std::string Status();
    std::string PC();

    /// INSTRUCTIONS
    #define DEF_INSTR(name) void name(AddressingMode mode)
    DEF_INSTR(ADC);
    DEF_INSTR(JMP);
    DEF_INSTR(BRK);
    DEF_INSTR(RTI);
    DEF_INSTR(LDX);
    DEF_INSTR(STX);
    DEF_INSTR(JSR);
    DEF_INSTR(SEC);
    DEF_INSTR(BCS);
    DEF_INSTR(CLC);
    DEF_INSTR(BCC);
    DEF_INSTR(LDA);
    DEF_INSTR(BEQ);
    DEF_INSTR(BNE);
    DEF_INSTR(STA);
    DEF_INSTR(BIT);
    DEF_INSTR(BVS);
    DEF_INSTR(BVC);
    DEF_INSTR(BPL);
    DEF_INSTR(RTS);
    DEF_INSTR(NOP);
    DEF_INSTR(SEI);
    DEF_INSTR(SED);
    DEF_INSTR(PHP);
    DEF_INSTR(PLA);
    DEF_INSTR(AND);
    DEF_INSTR(CMP);
    DEF_INSTR(CLD);
    DEF_INSTR(PHA);
    DEF_INSTR(PLP);
    DEF_INSTR(BMI);
    DEF_INSTR(ORA);
    DEF_INSTR(CLV);
    DEF_INSTR(EOR);
    DEF_INSTR(LDY);
    DEF_INSTR(CPX);
    DEF_INSTR(CPY);
    DEF_INSTR(SBC);
    DEF_INSTR(INX);
    DEF_INSTR(INY);
    DEF_INSTR(DEX);
    DEF_INSTR(DEY);
    DEF_INSTR(TAX);
    DEF_INSTR(TAY);
    DEF_INSTR(TXA);
    DEF_INSTR(TYA);
    DEF_INSTR(TSX);
    DEF_INSTR(TXS);
    DEF_INSTR(LSR);
    DEF_INSTR(ASL);
    DEF_INSTR(ROR);
    DEF_INSTR(ROL);
    DEF_INSTR(STY);
    DEF_INSTR(INC);
    DEF_INSTR(DEC);
    // Unofficial instructions
    DEF_INSTR(UN_NOP);

    // Instruction set keyed on opcode.
    struct Instruction {
      std::string name = "";
      std::function<void(void)> impl = nullptr; // addressing mode was bound
      // Base number of cycles. Impl can add more (ex. page crossing)
      uint8_t cycles = 0;
    };
    std::unordered_map<uint8_t, Instruction> instructions_;

    // Points to next address to execute
    uint16_t program_counter_ = 0;

    // 2kb of internal memory -- nestest wants this 0'd
    uint8_t internal_ram_[2048] = {};

    std::unique_ptr<Ppu> ppu_;

    // Todo pass ptr to this and internal ram into a MemoryView class
    std::unique_ptr<Mapper> mapper_;

    // NOTE: This needs to be last
    std::unique_ptr<MemoryView> memory_view_;

    /// Registers
    uint8_t a_;
    uint8_t x_;
    uint8_t y_;
    // Bit order MSb (NVxx DIZC) LSb -> Bits 4 and 5 only set when copied to stack.
    uint8_t p_;
    uint8_t stack_pointer_;

    // Current cycle number. Cycle 7 means 7 cycles have elapsed.
    uint64_t cycle_ = 0;
};

// notes:
// NES lacks decimal mode.
// little-endian - lsbyte first in memory
// registers:
// A: byte-wide, supports using the status register for carrying, overflow detection, etc
// X,Y: byte-wide indexes for several addressing modes.
//  only X can be used to get a copy of the stack pointer or change its value 
// PC: 2-bytes. Points to next instr. PC is modified automatically during
//  instr execution. Interrupts, jumps, branches also modify.
// S: byte-wide stack pointer. Points to the low 8 bits (00-FF) of the next free location
//  on the stack at $0100 - $01FF. Decr on push, incr on pull. No overflow detection.
// P: 6 bits but byte-wide status register. C,Z,I,D (no effect),V,N,B

// mem notes:
// $0000-$07FF SIZE $0800 : 2KB internal RAM 
// $0800-$0FFF SIZE $0800 : mirror of $0000 - $07FF
// $1000-$17FF SIZE $0800 : mirror of $0000 - $07FF
// $1800-$1FFF SIZE $0800 : mirror of $0000 - $07FF
// $2000-$2007 SIZE $0008 : NES PPU registers
// $2008-$3FFF SIZE $1FF8 : mirrors of $2000 - $2007 (repeats every 8 bytes)
// $4000-$4017 SIZE $0018 : NES APU and I/O registers
// $4018-$401F SIZE $0008 : APU and I/o functionality that is normally disabled (CPU test mode)
// $4020-$FFFF SIZE $BFE0 : Cartidge space: PRG ROM, PRG RAM, and mapper registers

// Think of 256-byte "page". So internal RAM $0000-$07FF has 8 pages. 
// The optional PRG RAM chip on some carts is 32 pages of SRAM at $6000-$7FFF
// Indirect addressing relies on zero/direct page at $0000-$00FF.
// Stack instructions always access stack page at $0100-$01FF.

// All addressing 16-bit unless noted.
// Indexed Addressing: 
// Uses the X or Y register to help determine address. 6 main indexed addr modes:
//  +: add a cycle for writes or for page wrapping on reads.
// d,x - zero page indexed - 4 cycles
//  val = PEEK((arg + X) % 256)
// d,y - zero page indexed - 4 cycles
//  val = PEEK((arg + Y) % 256)
// a,x - absolute indexed - 4+ cycles
//  val = PEEK(arg + X)
// a,y - absolute indexed - 4+ cycles
//  val = PEEK(arg + Y)
// (d,x) - indirect indexed - 6 cycles
//  val = PEEK(PEEK((arg + X) % 256) + PEEK((arg + X + 1) % 256) * 256)
// (d),y - indirect indexed - 5+ cycles
//  val = PEEK(PEEK(arg) + PEEK((arg + 1) % 256) * 256 + Y)
// 
// Non-indexed:
// A - Use the accumulator directly
// #v - immediate byte
// d - zero page. Fetch 8-bit val from zero page.
// a - absolute. Full 16-bit addr to identify location.
// label - relative. branch uses signed 8-bit relative offset.
// (a) - indirect. JMP can fetch 16-bit val from addr.


// PPU has a $0000 - $3FFF addr space. Can access via CPU registers 2006 and 2007
// PPU/$0000 - PPU/$1FFF is usually CHR-ROM or CHR-RAM, maybe with bank switching

#endif