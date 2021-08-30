#include "cpu6502.h"

#include <fstream>
#include <functional>

#include "common.h"
#include "mappers/nrom_mapper.h"
#include "mapper_id.h"
#include "ppu.h"
#include "memory_view.h"

namespace { 

#define VAL(a) memory_view_->Get(a)
#define DBGPAD(...) DBG("%-32s", string_format(__VA_ARGS__).c_str())
#define DBGPADSINGLE(x) DBG("       "); DBGPAD("%s", x);

// Used for nestest log goldens
void DBG(const char* str, ...) {
  #ifdef DEBUG
  va_list arglist;
  va_start(arglist, str);
  vprintf(str, arglist);
  va_end(arglist);
  #endif
}
// used for everything else
void VDBG(const char* str, ...) {
  #ifdef VDEBUG
  va_list arglist;
  va_start(arglist, str);
  vprintf(str, arglist);
  va_end(arglist);
  #endif
}

uint16_t StackAddr(uint8_t sp) {
  return ((0x01 << 8) | sp);
}

}

Cpu6502::Cpu6502(const std::string& file_path) {
  Reset(file_path);
  VDBG("NES ready. PC: %#04x\n", program_counter_);
}

// TODO: Rename to RunInstruction?
void Cpu6502::RunCycle() {
  std::string prev_flags = string_format("A:%02X X:%02X Y:%02X P:%02X SP:%02X",
    a_, x_, y_, p_, stack_pointer_);
  uint64_t prev_cycle = cycle_;
  uint8_t opcode = memory_view_->Get(program_counter_);
  DBG("%04X  %02X ", program_counter_, opcode);
  program_counter_++;
  Instruction& instr = instructions_.at(opcode);
  instr.impl();
  cycle_ += instr.cycles;
  DBG("%s PPU:  0,  0 CYC:%d\n", prev_flags.c_str(), prev_cycle);
}

void Cpu6502::Reset(const std::string& file_path) {
  LoadCartrtidgeFile(file_path);
  memory_view_ = std::make_unique<MemoryView>(internal_ram_, ppu_.get(), mapper_.get());
  assert(ppu_);
  assert(mapper_);
  assert(memory_view_);
  // DbgMem();

  // http://webcache.googleusercontent.com/search?q=cache:knntPlSpFnQJ:forums.nesdev.com/viewtopic.php%3Ff%3D3%26t%3D14231+&cd=4&hl=en&ct=clnk&gl=us
  // The following 7 cycles happens at reset:
  // 1. [READ] read (PC)
  // 2. [READ] read (PC)
  // 3. [READ] read (S), decrement S
  // 4. [READ] read (S), decrement S
  // 5. [READ] read (S), decrement S
  // 6. [WRITE] PC_low = ($FFFC), set interrupt flag
  // 7. [WRITE] PC_high = ($FFFD)

  // nestest should start at 0xC000 till I get input working
  // C000 is start of PRG_ROM's mirror (so 0x10 in .nes)
  if (file_path == "/Users/river/code/nes/roms/nestest.nes") {
    program_counter_ = 0xC000;
  } else {
    program_counter_ =  memory_view_->Get16(0xFFFC);
  }

  // not realistic -- programs should set these
  a_ = x_ = y_ = 0;

  p_ = 0x24;  // for nestest golden
  stack_pointer_ = 0xFD;
  cycle_ = 7;

  // TODO: Do the rest: https://wiki.nesdev.com/w/index.php?title=Init_code

  BuildInstructionSet();
}

void Cpu6502::LoadCartrtidgeFile(const std::string& file_path) {
  std::ifstream input(file_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
  if (bytes.size() <= 0) {
    throw std::runtime_error("No file or empty file.");
  } else if (bytes.size() < 8) {
    throw std::runtime_error("Invalid file format.");
  }

  bool is_ines = false;
  if (bytes[0] == 'N' && bytes[1] == 'E' && bytes[2] == 'S' && bytes[3] == 0x1A) {
    is_ines = true;
  }

  bool is_nes2 = false;
  if (is_ines == true && (bytes[7]&0x0C)==0x08) {
    is_nes2 = true;
  }

  if (is_nes2) {
    VDBG("Found %d byte NES 2.0 file.\n", bytes.size());
    // TODO: Load NES 2.0 specific
    // Back-compat with ines 1.0
    LoadNes1File(bytes);
  } else if (is_ines) {
    VDBG("Found %d byte iNES 1.0 file.\n", bytes.size());
    LoadNes1File(bytes);
  } else {
    throw std::runtime_error("Rom file is not iNES format.");
  }
}

void Cpu6502::LoadNes1File(std::vector<uint8_t> bytes) {
  if (bytes.size() < 16) {
    throw std::runtime_error("Incomplete iNes header.");
  }

  uint32_t prg_rom_size = static_cast<uint32_t>(bytes[4]) * 0x4000;
  uint32_t chr_rom_size = static_cast<uint32_t>(bytes[5]) * 0x2000;

  if (bytes.size() < (prg_rom_size + chr_rom_size + 16)) {
    throw std::runtime_error("Rom file size less than header suggests.");
  }

  // TODO: Handle flags as needed.
  uint8_t flags6 = bytes[6];  // msb are lower nybble of mapper num
  if (flags6 & 0b0010'0000) {
    throw std::runtime_error("Rom has a trainer!");
  }
  uint8_t flags7 = bytes[7];  // lsb are upper nybble of mapper num
  uint8_t mapper_number = ((flags7 >> 4) << 4) | (flags6 >> 4);

  uint8_t prg_ram_size = bytes[8] == 0x0 ? static_cast<uint8_t>(0x2000) : bytes[8] * 0x2000;
  VDBG("Mapper ID %d PRG_ROM sz %d CHAR_ROM sz %d PRG_RAM sz %d\n",
      mapper_number, prg_rom_size, chr_rom_size, prg_ram_size);
  
  if (chr_rom_size > 0) {
    ppu_ = std::make_unique<Ppu>(bytes.data() + 16 + prg_rom_size, chr_rom_size);
  } else {
    ppu_ = std::make_unique<Ppu>(nullptr, 0);
  }

  mapper_ = std::make_unique<NromMapper>(bytes.data() + 16, prg_rom_size);
}

bool Cpu6502::GetFlag(Cpu6502::Flag flag) {
  return Bit(static_cast<uint8_t>(flag), p_) == 1;
}

void Cpu6502::SetFlag(Cpu6502::Flag flag, bool val) {
  p_ = SetBit(static_cast<uint8_t>(flag), p_, val);
}

void Cpu6502::SetPIgnoreB(uint8_t new_p) {
  uint8_t old_p = p_;

  // Need to preserve bits 4 and 5 from original p
  // There's probably a better way to do this...
  if (Bit(4, old_p)) {
    new_p |= (1 << 4);
  } else {
    new_p &= ~(1 << 4);
  }

  if (Bit(5, old_p)) {
    new_p |= (1 << 5);
  } else {
    new_p &= ~(1 << 5);
  }
  p_ = new_p;
}

uint8_t Cpu6502::NextImmediate() {
  uint8_t val = memory_view_->Get(program_counter_++);
  DBG("%02X    ", val);
  return val;
}
uint16_t Cpu6502::NextZeroPage() {
  uint16_t addr = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(addr));
  return addr;
}
uint16_t Cpu6502::NextZeroPageX() {
  uint16_t addr = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(addr));
  addr = (addr + x_) % 0x100;  // Add X to LSB of ZP
  return addr;
}
uint16_t Cpu6502::NextZeroPageY() {
  uint16_t addr = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(addr));
  addr = (addr + y_) % 0x100;  // Add Y to LSB of ZP
  return addr;
}
uint16_t Cpu6502::NextAbsolute() {
  uint16_t addr = memory_view_->Get16(program_counter_);
  DBG("%02X %02X ", static_cast<uint8_t>(addr), static_cast<uint8_t>(addr >> 8)); // low first
  program_counter_ += 2;
  return addr;
}
uint16_t Cpu6502::NextAbsoluteX(bool* page_crossed) {
  uint16_t addr = memory_view_->Get16(program_counter_);
  DBG("%02X %02X ", static_cast<uint8_t>(addr), static_cast<uint8_t>(addr >> 8));
  program_counter_ += 2;
  *page_crossed = CrossedPage(addr, addr + x_);
  return addr + x_;
}
uint16_t Cpu6502::NextAbsoluteY(bool* page_crossed) {
  uint16_t addr = memory_view_->Get16(program_counter_);
  DBG("%02X %02X ", static_cast<uint8_t>(addr), static_cast<uint8_t>(addr >> 8));
  program_counter_ += 2;
  *page_crossed = CrossedPage(addr, addr + y_);
  return addr + y_;
}
uint16_t Cpu6502::NextIndirectX() {
  // Get ZP, add X_ to LSB, then read full addr
  uint16_t zero_addr = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(zero_addr));
  zero_addr = (zero_addr + x_) % 0x100;
  return memory_view_->Get16(zero_addr, /*page_wrap=*/true);
}
uint16_t Cpu6502::NextIndirectY(bool* page_crossed) {
  // get ZP addr, then read full addr from it and add Y
  uint16_t zero_addr = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(zero_addr));
  uint16_t addr = memory_view_->Get16(zero_addr, /*page_wrap=*/true);
  *page_crossed = CrossedPage(addr, addr + y_);
  return addr + y_;
}

uint16_t Cpu6502::NextAbsoluteIndirect() {
  uint16_t indirect = memory_view_->Get16(program_counter_);
  DBG("%02X %02X ", static_cast<uint8_t>(indirect), static_cast<uint8_t>(indirect >> 8));
  program_counter_ += 2;

  return memory_view_->Get16(indirect, /*page_wrap=*/true);
}

uint16_t Cpu6502::NextRelativeAddr(bool* page_crossed) {
  uint8_t offset_uint = memory_view_->Get(program_counter_++);
  DBG("%02X    ", static_cast<uint8_t>(offset_uint));
  // https://stackoverflow.com/questions/14623266/why-cant-i-reinterpret-cast-uint-to-int
  int8_t tmp;
  std::memcpy(&tmp, &offset_uint, sizeof(tmp));
  const int8_t offset = tmp;

  *page_crossed = CrossedPage(program_counter_, program_counter_ + offset);
  return program_counter_ + offset;
}

void Cpu6502::PushStack(uint8_t val) {
  memory_view_->Set(StackAddr(stack_pointer_--), val);
}
void Cpu6502::PushStack16(uint16_t val) {
  // Store MSB then LSB so that we can read back little-endian.
  PushStack(val >> 8);
  PushStack(static_cast<uint8_t>(val));
}
uint8_t Cpu6502::PopStack() {
  return memory_view_->Get(StackAddr(++stack_pointer_));
}
uint16_t Cpu6502::PopStack16() {
  // Value was stored little-endian in top-down stack, so get LSB then MSB
  return (PopStack() | (PopStack() << 8));
}

void Cpu6502::DbgMem() {
  for (int i = 0x6000; i <= 0xFFFF; i += 0x10) {
    DBG("\nCPU[%03X]: ", i);
    for (int j = 0; j < 0x10; j++) {
      DBG("%#04x ", mapper_->Get(i + j));
    }
  }
  DBG("\n");
}

void Cpu6502::DbgStack() {
  for (int i = 0x0100; i <= 0x01FF; i += 0x10) {
    DBG("\nCPU[%03X]: ", i);
    for (int j = 0; j < 0x10; j++) {
      DBG("%#04x ", memory_view_->Get(i + j));
    }
  }
  DBG("\n");
}

std::string Cpu6502::Status() {
  return string_format("[PC: %#06x, A: %#04x, X: %#04x, Y: %#04x, P: %#04x, SP: %#04x]",
    program_counter_, a_, x_, y_, p_, stack_pointer_);
}

void Cpu6502::ADC(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  uint8_t val = addrval.val;
  DBGPAD("ADC %s", AddrValString(addrval, mode).c_str());
  uint16_t new_a = a_ + val + GetFlag(Flag::C);
  SetFlag(Flag::C, new_a > 0xFF);
  SetFlag(Flag::V, Pos(a_) && Pos(val) && !Pos(new_a));
  a_ = new_a;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

void Cpu6502::JMP(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("JMP %s", AddrValString(addrval, mode, /*is_jmp=*/true).c_str());
  program_counter_ = addr;
}

void Cpu6502::BRK(AddressingMode mode) {
  PushStack16(program_counter_);  // PC is already +1 from reading instr.
  PushStack(p_ | 0b0011'0000);  // B=0b11
  program_counter_ = memory_view_->Get16(0xFFFE);
  SetFlag(Flag::I, true);
  DBGPADSINGLE("BRK");
}

void Cpu6502::RTI(AddressingMode mode) {
  SetPIgnoreB(PopStack());
  program_counter_ = PopStack16();
  DBGPADSINGLE("RTI");
}

void Cpu6502::LDX(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  uint8_t val = addrval.val;
  DBGPAD("LDX %s", AddrValString(addrval, mode).c_str());
  SetFlag(Flag::Z, val == 0);
  SetFlag(Flag::N, !Pos(val));
  x_ = val;
}

void Cpu6502::STX(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("STX %s", AddrValString(addrval, mode).c_str());
  memory_view_->Set(addr, x_);
}

void Cpu6502::JSR(AddressingMode mode) {
  uint16_t pc_to_return_to = program_counter_ + 1;
  AddrVal addrval = NextAddrVal(mode);
  uint16_t new_pc = addrval.addr;
  DBGPAD("JSR %s", AddrValString(addrval, mode, /*is_jmp=*/true).c_str());
  PushStack16(pc_to_return_to);
  program_counter_ = new_pc;
}

void Cpu6502::SEC(AddressingMode mode) {
  SetFlag(Flag::C, true);
  DBGPADSINGLE("SEC");
}

void Cpu6502::BCS(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BCS %s", AddrValString(addrval, mode).c_str());
  if (GetFlag(Flag::C)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::CLC(AddressingMode mode) {
  SetFlag(Flag::C, false);
  DBGPADSINGLE("CLC");
}

void Cpu6502::BCC(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BCC %s", AddrValString(addrval, mode).c_str());
  if (!GetFlag(Flag::C)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::LDA(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  uint8_t val = addrval.val;
  DBGPAD("LDA %s", AddrValString(addrval, mode).c_str());
  SetFlag(Flag::Z, val == 0);
  SetFlag(Flag::N, !Pos(val));
  a_ = val;
}

void Cpu6502::BEQ(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BEQ %s", AddrValString(addrval, mode).c_str());
  if (GetFlag(Flag::Z)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::BNE(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BNE %s", AddrValString(addrval, mode).c_str());
  if (!GetFlag(Flag::Z)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::STA(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("STA %s", AddrValString(addrval, mode).c_str());
  memory_view_->Set(addr, a_);
}

void Cpu6502::BIT(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint8_t val = addrval.val;
  DBGPAD("BIT %s", AddrValString(addrval, mode).c_str());
  uint8_t res = val & a_;
  SetFlag(Flag::Z, res == 0);
  SetFlag(Flag::V, Bit(6, val) == 1);
  SetFlag(Flag::N, Bit(7, val) == 1);
}

void Cpu6502::BVS(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BVS %s", AddrValString(addrval, mode).c_str());
  if (GetFlag(Flag::V)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::BVC(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BVC %s", AddrValString(addrval, mode).c_str());
  if (!GetFlag(Flag::V)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::BPL(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BPL %s", AddrValString(addrval, mode).c_str());
  if (!GetFlag(Flag::N)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::RTS(AddressingMode mode) {
  program_counter_ = PopStack16() + 1;
  DBGPADSINGLE("RTS");
}

void Cpu6502::NOP(AddressingMode mode) {
  DBGPADSINGLE("NOP");
 }

void Cpu6502::SEI(AddressingMode mode) {
  SetFlag(Flag::I, true);
  DBGPADSINGLE("SEI");
 }

 void Cpu6502::SED(AddressingMode mode) {
  SetFlag(Flag::D, true);
  DBGPADSINGLE("SED");
 }

 void Cpu6502::PHP(AddressingMode mode) {
  PushStack(p_ | 0b0011'0000);  // B=0b11
  DBGPADSINGLE("PHP");
 }

void Cpu6502::PLA(AddressingMode mode) {
  DBGPADSINGLE("PLA");
  a_ = PopStack();
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
 }

void Cpu6502::AND(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  a_ &= addrval.val;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
  DBGPAD("AND %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::CMP(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  SetFlag(Flag::C, a_ >= addrval.val);
  SetFlag(Flag::Z, a_ == addrval.val);
  SetFlag(Flag::N, !Pos(a_ - addrval.val));
  DBGPAD("CMP %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::CLD(AddressingMode mode) {
  SetFlag(Flag::D, false);
  DBGPADSINGLE("CLD");
}

void Cpu6502::PHA(AddressingMode mode) {
  PushStack(a_);
  DBGPADSINGLE("PHA");
}

void Cpu6502::PLP(AddressingMode mode) {
  SetPIgnoreB(PopStack());
  DBGPADSINGLE("PLP");
}

void Cpu6502::BMI(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("BMI %s", AddrValString(addrval, mode).c_str());
  if (GetFlag(Flag::N)) {
    program_counter_ = addr;
    cycle_ += addrval.page_crossed + 1;
  }
}

void Cpu6502::ORA(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  a_ |= addrval.val;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
  DBGPAD("ORA %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::CLV(AddressingMode mode) {
  SetFlag(Flag::V, false);
  DBGPADSINGLE("CLV");
}

void Cpu6502::EOR(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  a_ ^= addrval.val;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
  DBGPAD("EOR %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::LDY(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  uint8_t val = addrval.val;
  DBGPAD("LDY %s", AddrValString(addrval, mode).c_str());
  SetFlag(Flag::Z, val == 0);
  SetFlag(Flag::N, !Pos(val));
  y_ = val;
}

void Cpu6502::CPX(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  SetFlag(Flag::C, x_ >= addrval.val);
  SetFlag(Flag::Z, x_ == addrval.val);
  SetFlag(Flag::N, !Pos(x_ - addrval.val));
  DBGPAD("CPX %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::CPY(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  cycle_ += addrval.page_crossed;
  SetFlag(Flag::C, y_ >= addrval.val);
  SetFlag(Flag::Z, y_ == addrval.val);
  SetFlag(Flag::N, !Pos(y_ - addrval.val));
  DBGPAD("CPY %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::SBC(AddressingMode mode, bool unofficial) {
  AddrVal addrval = NextAddrVal(mode, unofficial);
  uint8_t val = addrval.val;
  val = ~val;
  DBGPAD("SBC %s", AddrValString(addrval, mode).c_str());
  uint16_t new_a = a_ + val + GetFlag(Flag::C);
  SetFlag(Flag::C, new_a > 0xFF);
  if (Pos(a_) && !Pos(addrval.val) && !Pos(new_a)) {
    SetFlag(Flag::V, true);
  } else if (!Pos(a_) && Pos(addrval.val) && Pos(new_a)) {
    SetFlag(Flag::V, true);
  } else {
    SetFlag(Flag::V, false);
  }
  a_ = new_a;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

void Cpu6502::INX(AddressingMode mode) {
  DBGPADSINGLE("INX");
  x_ += 1;
  SetFlag(Flag::Z, x_ == 0);
  SetFlag(Flag::N, !Pos(x_));
}

void Cpu6502::INY(AddressingMode mode) {
  DBGPADSINGLE("INY");
  y_ += 1;
  SetFlag(Flag::Z, y_ == 0);
  SetFlag(Flag::N, !Pos(y_));
}

void Cpu6502::DEX(AddressingMode mode) {
  DBGPADSINGLE("DEX");
  x_ -= 1;
  SetFlag(Flag::Z, x_ == 0);
  SetFlag(Flag::N, !Pos(x_));
}

void Cpu6502::DEY(AddressingMode mode) {
  DBGPADSINGLE("DEY");
  y_ -= 1;
  SetFlag(Flag::Z, y_ == 0);
  SetFlag(Flag::N, !Pos(y_));
}

void Cpu6502::TAX(AddressingMode mode) {
  DBGPADSINGLE("TAX");
  x_ = a_;
  SetFlag(Flag::Z, x_ == 0);
  SetFlag(Flag::N, !Pos(x_));
}

void Cpu6502::TAY(AddressingMode mode) {
  DBGPADSINGLE("TAY");
  y_ = a_;
  SetFlag(Flag::Z, y_ == 0);
  SetFlag(Flag::N, !Pos(y_));
}

void Cpu6502::TXA(AddressingMode mode) {
  DBGPADSINGLE("TXA");
  a_ = x_;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

void Cpu6502::TYA(AddressingMode mode) {
  DBGPADSINGLE("TYA");
  a_ = y_;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

void Cpu6502::TSX(AddressingMode mode) {
  DBGPADSINGLE("TSX");
  x_ = stack_pointer_;
  SetFlag(Flag::Z, x_ == 0);
  SetFlag(Flag::N, !Pos(x_));
}

void Cpu6502::TXS(AddressingMode mode) {
  // Weirdly enough this doesn't set flags.
  DBGPADSINGLE("TXS");
  stack_pointer_ = x_;
}

void Cpu6502::LSR(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  DBGPAD("LSR %s", AddrValString(addrval, mode).c_str());
  // Accumulator needs to be set directly
  uint8_t result = 0;
  uint8_t initial_val = 0;
  if (mode == AddressingMode::kAccumulator) {
    initial_val = a_;
    result = initial_val >> 1;
    a_ = result;
  } else {
    initial_val = addrval.val;
    result = initial_val >> 1;
    memory_view_->Set(addrval.addr, result);
  }
  SetFlag(Flag::C, Bit(0, initial_val));
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

void Cpu6502::ASL(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  DBGPAD("ASL %s", AddrValString(addrval, mode).c_str());
  // Accumulator needs to be set directly
  uint8_t result = 0;
  uint8_t initial_val = 0;
  if (mode == AddressingMode::kAccumulator) {
    initial_val = a_;
    result = initial_val << 1;
    a_ = result;
  } else {
    initial_val = addrval.val;
    result = initial_val << 1;
    memory_view_->Set(addrval.addr, result);
  }
  SetFlag(Flag::C, Bit(7, initial_val));
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

void Cpu6502::ROR(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  DBGPAD("ROR %s", AddrValString(addrval, mode).c_str());
  // Accumulator needs to be set directly
  uint8_t result = 0;
  uint8_t initial_val = 0;
  if (mode == AddressingMode::kAccumulator) {
    initial_val = a_;
    result = initial_val >> 1;
    result = SetBit(7, result, GetFlag(Flag::C));
    a_ = result;
  } else {
    initial_val = addrval.val;
    result = initial_val >> 1;
    result = SetBit(7, result, GetFlag(Flag::C));
    memory_view_->Set(addrval.addr, result);
  }
  SetFlag(Flag::C, Bit(0, initial_val));
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

void Cpu6502::ROL(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  DBGPAD("ROL %s", AddrValString(addrval, mode).c_str());
  // Accumulator needs to be set directly
  uint8_t result = 0;
  uint8_t initial_val = 0;
  if (mode == AddressingMode::kAccumulator) {
    initial_val = a_;
    result = initial_val << 1;
    result = SetBit(0, result, GetFlag(Flag::C));
    a_ = result;
  } else {
    initial_val = addrval.val;
    result = initial_val << 1;
    result = SetBit(0, result, GetFlag(Flag::C));
    memory_view_->Set(addrval.addr, result);
  }
  SetFlag(Flag::C, Bit(7, initial_val));
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

void Cpu6502::STY(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("STY %s", AddrValString(addrval, mode).c_str());
  memory_view_->Set(addr, y_);
}

void Cpu6502::INC(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("INC %s", AddrValString(addrval, mode).c_str());
  uint8_t result = memory_view_->Get(addr) + 1;
  memory_view_->Set(addr, result);
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

void Cpu6502::DEC(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode);
  uint16_t addr = addrval.addr;
  DBGPAD("DEC %s", AddrValString(addrval, mode).c_str());
  uint8_t result = memory_view_->Get(addr) - 1;
  memory_view_->Set(addr, result);
  SetFlag(Flag::Z, result == 0);
  SetFlag(Flag::N, !Pos(result));
}

/// Unoficial Opcodes

void Cpu6502::UN_NOP(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unoficial=*/true);
  cycle_ += addrval.page_crossed;
  DBGPAD("NOP %s", AddrValString(addrval, mode).c_str());
}

void Cpu6502::UN_LAX(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unoficial=*/true);
  cycle_ += addrval.page_crossed;
  DBGPAD("LAX %s", AddrValString(addrval, mode).c_str());
  // LDA then TAX. So just load into both.
  uint8_t val = addrval.val;
  SetFlag(Flag::Z, val == 0);
  SetFlag(Flag::N, !Pos(val));
  x_ = val;
  a_ = val;
}

void Cpu6502::UN_SAX(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unofficial=*/true);
  uint16_t addr = addrval.addr;
  cycle_ += addrval.page_crossed;
  DBGPAD("SAX %s", AddrValString(addrval, mode).c_str());
  memory_view_->Set(addr, a_ & x_);
}

void Cpu6502::UN_SBC(AddressingMode mode) {
  SBC(mode, /*unofficial=*/true);
}

void Cpu6502::UN_DCP(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unofficial=*/true);
  uint16_t addr = addrval.addr;
  cycle_ += addrval.page_crossed;
  DBGPAD("DCP %s", AddrValString(addrval, mode).c_str());
  // DEC then CMP the value.
  uint8_t result = memory_view_->Get(addr) - 1;
  memory_view_->Set(addr, result);
  SetFlag(Flag::C, a_ >= result);
  SetFlag(Flag::Z, a_ == result);
  SetFlag(Flag::N, !Pos(a_ - result));
}

void Cpu6502::UN_ISB(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unofficial=*/true);
  uint16_t addr = addrval.addr;
  cycle_ += addrval.page_crossed;
  DBGPAD("ISB %s", AddrValString(addrval, mode).c_str());

  // INC then SBC the value.
  uint8_t val = memory_view_->Get(addr) + 1;
  memory_view_->Set(addr, val);

  val = ~val;
  uint16_t new_a = a_ + val + GetFlag(Flag::C);
  SetFlag(Flag::C, new_a > 0xFF);
  if (Pos(a_) && !Pos(addrval.val) && !Pos(new_a)) {
    SetFlag(Flag::V, true);
  } else if (!Pos(a_) && Pos(addrval.val) && Pos(new_a)) {
    SetFlag(Flag::V, true);
  } else {
    SetFlag(Flag::V, false);
  }
  a_ = new_a;
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

void Cpu6502::UN_SLO(AddressingMode mode) {
  AddrVal addrval = NextAddrVal(mode, /*unofficial=*/true);
  cycle_ += addrval.page_crossed;
  DBGPAD("SLO %s", AddrValString(addrval, mode).c_str());
  // ASL val then ORA it into A.
  uint8_t initial_val = addrval.val;
  uint8_t result = initial_val << 1;
  memory_view_->Set(addrval.addr, result);

  a_ |= result;
  SetFlag(Flag::C, Bit(7, initial_val));
  SetFlag(Flag::Z, a_ == 0);
  SetFlag(Flag::N, !Pos(a_));
}

uint16_t Cpu6502::NextAddr(AddressingMode mode, bool* page_crossed) {
  switch (mode) {
    case AddressingMode::kZeroPage:
      return NextZeroPage();
    case AddressingMode::kZeroPageX:
      return NextZeroPageX();
    case AddressingMode::kZeroPageY:
      return NextZeroPageY();
    case AddressingMode::kAbsolute:
      return NextAbsolute();
    case AddressingMode::kAbsoluteX:
      return NextAbsoluteX(page_crossed);
    case AddressingMode::kAbsoluteY:
      return NextAbsoluteY(page_crossed);
    case AddressingMode::kIndirectX:
      return NextIndirectX();
    case AddressingMode::kIndirectY:
      return NextIndirectY(page_crossed);
    case AddressingMode::kAbsoluteIndirect:
      return NextAbsoluteIndirect();
    case AddressingMode::kRelative:
      return NextRelativeAddr(page_crossed);
    default:
      throw std::runtime_error("Undefined addressing mode in NextAddr.");
  }
}

Cpu6502::AddrVal Cpu6502::NextAddrVal(AddressingMode mode, bool unofficial) {
  if (mode == AddressingMode::kImmediate) {
    uint8_t imm = NextImmediate();
    if (unofficial) DBG("*"); else DBG(" ");
    return {0, imm};
  } else if (mode == AddressingMode::kAccumulator) {
    DBG("      "); if (unofficial) DBG("*"); else DBG(" ");
    return {0, a_};
  } else if (mode == AddressingMode::kNone) {
    DBG("      "); if (unofficial) DBG("*"); else DBG(" ");
    return {0, a_};
  }
  AddrVal addrval;
  addrval.addr = NextAddr(mode, &addrval.page_crossed);
  addrval.val = VAL(addrval.addr);
  if (unofficial) DBG("*"); else DBG(" ");
  return addrval;
}

std::string Cpu6502::AddrValString(AddrVal addrval, AddressingMode mode, bool is_jmp)  {
  switch (mode) {
    case AddressingMode::kImmediate:
      return string_format("#$%02X", addrval.val);
    case AddressingMode::kZeroPage:
      return string_format("$%02X = %02X", addrval.addr, addrval.val);
    case AddressingMode::kZeroPageX: {
      uint8_t target = memory_view_->Get(program_counter_ - 1);
      return string_format("$%02X,X @ %02X = %02X", target, addrval.addr, addrval.val);
    }
    case AddressingMode::kZeroPageY: {
      uint8_t target = memory_view_->Get(program_counter_ - 1);
      return string_format("$%02X,Y @ %02X = %02X", target, addrval.addr, addrval.val);
    }
    case AddressingMode::kAbsolute:
      // For some reason nestest doesn't want jmp to log the value
      if (is_jmp) {
        return string_format("$%04X", addrval.addr);
      } else {
        return string_format("$%04X = %02X", addrval.addr, addrval.val);
      }
    case AddressingMode::kAbsoluteX: {
      uint16_t target = memory_view_->Get16(program_counter_ - 2);
      return string_format("$%04X,X @ %04X = %02X", target, addrval.addr, addrval.val);
    }
    case AddressingMode::kAbsoluteY: {
      uint16_t target = memory_view_->Get16(program_counter_ - 2);
      return string_format("$%04X,Y @ %04X = %02X", target, addrval.addr, addrval.val);
    }
    case AddressingMode::kIndirectX: {
      uint8_t target = memory_view_->Get(program_counter_ - 1);
      return string_format("($%02X,X) @ %02X = %04X = %02X", target, (target + x_) % 0x100, addrval.addr, addrval.val);
    }
    case AddressingMode::kIndirectY: {
      uint8_t target = memory_view_->Get(program_counter_ - 1);
      uint16_t indirect = addrval.addr - y_;
      return string_format("($%02X),Y = %04X @ %04X = %02X", target, indirect, addrval.addr, addrval.val);
    }
    case AddressingMode::kAbsoluteIndirect: {
      uint16_t target = memory_view_->Get16(program_counter_ - 2);
      return string_format("($%04X) = %04X", target, addrval.addr);
    }
    case AddressingMode::kRelative:
      return string_format("$%04X", addrval.addr);
    case AddressingMode::kAccumulator:
      return "A";
    case AddressingMode::kNone:
      return "";
    default:
      throw std::runtime_error("Unhandled AddrMode");
  }
}

void Cpu6502::BuildInstructionSet() {
  #define ADD_INSTR(op, name, mode, cycles) instructions_[op] = {#name, [this]() { name(mode); }, cycles};
  ADD_INSTR(0x69, ADC, AddressingMode::kImmediate, 2);
  ADD_INSTR(0x65, ADC, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x75, ADC, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x6D, ADC, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x7D, ADC, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0x79, ADC, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0x61, ADC, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x71, ADC, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0x4C, JMP, AddressingMode::kAbsolute, 3);
  ADD_INSTR(0x6C, JMP, AddressingMode::kAbsoluteIndirect, 5);
  ADD_INSTR(0x00, BRK, AddressingMode::kNone, 7);
  ADD_INSTR(0x40, RTI, AddressingMode::kNone, 6);
  ADD_INSTR(0xA2, LDX, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xA6, LDX, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xB6, LDX, AddressingMode::kZeroPageY, 4);
  ADD_INSTR(0xAE, LDX, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xBE, LDX, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0x86, STX, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x96, STX, AddressingMode::kZeroPageY, 4);
  ADD_INSTR(0x8E, STX, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x20, JSR, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0xEA, NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0x38, SEC, AddressingMode::kNone, 2);
  ADD_INSTR(0xB0, BCS, AddressingMode::kRelative, 2);
  ADD_INSTR(0x18, CLC, AddressingMode::kNone, 2);
  ADD_INSTR(0x90, BCC, AddressingMode::kRelative, 2);
  ADD_INSTR(0xA9, LDA, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xA5, LDA, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xB5, LDA, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0xAD, LDA, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xBD, LDA, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0xB9, LDA, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0xA1, LDA, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0xB1, LDA, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xF0, BEQ, AddressingMode::kRelative, 2);
  ADD_INSTR(0xD0, BNE, AddressingMode::kRelative, 2);
  ADD_INSTR(0x85, STA, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x95, STA, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x8D, STA, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x9D, STA, AddressingMode::kAbsoluteX, 5);
  ADD_INSTR(0x99, STA, AddressingMode::kAbsoluteY, 5);
  ADD_INSTR(0x81, STA, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x91, STA, AddressingMode::kIndirectY, 6);
  ADD_INSTR(0x24, BIT, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x2C, BIT, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x70, BVS, AddressingMode::kRelative, 2);
  ADD_INSTR(0x50, BVC, AddressingMode::kRelative, 2);
  ADD_INSTR(0x10, BPL, AddressingMode::kRelative, 2);
  ADD_INSTR(0x60, RTS, AddressingMode::kNone, 6);
  ADD_INSTR(0x78, SEI, AddressingMode::kNone, 2);
  ADD_INSTR(0xF8, SED, AddressingMode::kNone, 2);
  ADD_INSTR(0x08, PHP, AddressingMode::kNone, 3);
  ADD_INSTR(0x68, PLA, AddressingMode::kNone, 4);
  ADD_INSTR(0x29, AND, AddressingMode::kImmediate, 2);
  ADD_INSTR(0x25, AND, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x35, AND, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x2D, AND, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x3D, AND, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0x39, AND, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0x21, AND, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x31, AND, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xC9, CMP, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xC5, CMP, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xD5, CMP, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0xCD, CMP, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xDD, CMP, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0xD9, CMP, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0xC1, CMP, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0xD1, CMP, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xD8, CLD, AddressingMode::kNone, 2);
  ADD_INSTR(0x48, PHA, AddressingMode::kNone, 3);
  ADD_INSTR(0x28, PLP, AddressingMode::kNone, 4);
  ADD_INSTR(0x30, BMI, AddressingMode::kRelative, 2);
  ADD_INSTR(0x09, ORA, AddressingMode::kImmediate, 2);
  ADD_INSTR(0x05, ORA, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x15, ORA, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x0D, ORA, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x1D, ORA, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0x19, ORA, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0x01, ORA, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x11, ORA, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xB8, CLV, AddressingMode::kNone, 2);
  ADD_INSTR(0x49, EOR, AddressingMode::kImmediate, 2);
  ADD_INSTR(0x45, EOR, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x55, EOR, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x4D, EOR, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x5D, EOR, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0x59, EOR, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0x41, EOR, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x51, EOR, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xA0, LDY, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xA4, LDY, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xB4, LDY, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0xAC, LDY, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xBC, LDY, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0xE0, CPX, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xE4, CPX, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xEC, CPX, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xC0, CPY, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xC4, CPY, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xCC, CPY, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xE9, SBC, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xE5, SBC, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0xF5, SBC, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0xED, SBC, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xFD, SBC, AddressingMode::kAbsoluteX, 4);
  ADD_INSTR(0xF9, SBC, AddressingMode::kAbsoluteY, 4);
  ADD_INSTR(0xE1, SBC, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0xF1, SBC, AddressingMode::kIndirectY, 5);
  ADD_INSTR(0xE8, INX, AddressingMode::kNone, 2);
  ADD_INSTR(0xC8, INY, AddressingMode::kNone, 2);
  ADD_INSTR(0xCA, DEX, AddressingMode::kNone, 2);
  ADD_INSTR(0x88, DEY, AddressingMode::kNone, 2);
  ADD_INSTR(0xAA, TAX, AddressingMode::kNone, 2);
  ADD_INSTR(0xA8, TAY, AddressingMode::kNone, 2);
  ADD_INSTR(0x8A, TXA, AddressingMode::kNone, 2);
  ADD_INSTR(0x98, TYA, AddressingMode::kNone, 2);
  ADD_INSTR(0x9A, TXS, AddressingMode::kNone, 2);
  ADD_INSTR(0xBA, TSX, AddressingMode::kNone, 2);
  ADD_INSTR(0x4A, LSR, AddressingMode::kAccumulator, 2);
  ADD_INSTR(0x46, LSR, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0x56, LSR, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0x4E, LSR, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0x5E, LSR, AddressingMode::kAbsoluteX, 7);
  ADD_INSTR(0x0A, ASL, AddressingMode::kAccumulator, 2);
  ADD_INSTR(0x06, ASL, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0x16, ASL, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0x0E, ASL, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0x1E, ASL, AddressingMode::kAbsoluteX, 7);
  ADD_INSTR(0x6A, ROR, AddressingMode::kAccumulator, 2);
  ADD_INSTR(0x66, ROR, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0x76, ROR, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0x6E, ROR, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0x7E, ROR, AddressingMode::kAbsoluteX, 7);
  ADD_INSTR(0x2A, ROL, AddressingMode::kAccumulator, 2);
  ADD_INSTR(0x26, ROL, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0x36, ROL, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0x2E, ROL, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0x3E, ROL, AddressingMode::kAbsoluteX, 7);
  ADD_INSTR(0x84, STY, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x94, STY, AddressingMode::kZeroPageX, 4);
  ADD_INSTR(0x8C, STY, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0xE6, INC, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0xF6, INC, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0xEE, INC, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0xFE, INC, AddressingMode::kAbsoluteX, 7);
  ADD_INSTR(0xC6, DEC, AddressingMode::kZeroPage, 5);
  ADD_INSTR(0xD6, DEC, AddressingMode::kZeroPageX, 6);
  ADD_INSTR(0xCE, DEC, AddressingMode::kAbsolute, 6);
  ADD_INSTR(0xDE, DEC, AddressingMode::kAbsoluteX, 7);

  // Unofficial
  ADD_INSTR(0x04, UN_NOP, AddressingMode::kZeroPage, 3);  // d = zero page
  ADD_INSTR(0x44, UN_NOP, AddressingMode::kZeroPage, 3);  // d
  ADD_INSTR(0x64, UN_NOP, AddressingMode::kZeroPage, 3);  // d
  ADD_INSTR(0x0C, UN_NOP, AddressingMode::kAbsolute, 4);  // probably absolute not accum
  ADD_INSTR(0x14, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x = zero page, x
  ADD_INSTR(0x34, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x
  ADD_INSTR(0x54, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x
  ADD_INSTR(0x74, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x
  ADD_INSTR(0xD4, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x
  ADD_INSTR(0xF4, UN_NOP, AddressingMode::kZeroPageX, 4);  // d,x
  ADD_INSTR(0x1C, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0x3C, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0x5C, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0x7C, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0xDC, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0xFC, UN_NOP, AddressingMode::kAbsoluteX, 4);  // a,x
  ADD_INSTR(0x80, UN_NOP, AddressingMode::kImmediate, 2);  // #i = immediate
  ADD_INSTR(0x89, UN_NOP, AddressingMode::kImmediate, 2);  // #i
  ADD_INSTR(0x82, UN_NOP, AddressingMode::kImmediate, 2);  // #i
  ADD_INSTR(0xC2, UN_NOP, AddressingMode::kImmediate, 2);  // #i
  ADD_INSTR(0xE2, UN_NOP, AddressingMode::kImmediate, 2);  // #i
  ADD_INSTR(0x1A, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0x3A, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0x5A, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0x7A, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0xDA, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0xFA, UN_NOP, AddressingMode::kNone, 2);
  ADD_INSTR(0xA3, UN_LAX, AddressingMode::kIndirectX, 6); // (d,x)
  ADD_INSTR(0xA7, UN_LAX, AddressingMode::kZeroPage, 3); // d
  ADD_INSTR(0xAF, UN_LAX, AddressingMode::kAbsolute, 4); // a
  ADD_INSTR(0xB3, UN_LAX, AddressingMode::kIndirectY, 5); // (d),Y 
  ADD_INSTR(0xB7, UN_LAX, AddressingMode::kZeroPageY, 4); // d,Y
  ADD_INSTR(0xBF, UN_LAX, AddressingMode::kAbsoluteY, 4); // a,Y
  ADD_INSTR(0x83, UN_SAX, AddressingMode::kIndirectX, 6);
  ADD_INSTR(0x87, UN_SAX, AddressingMode::kZeroPage, 3);
  ADD_INSTR(0x8F, UN_SAX, AddressingMode::kAbsolute, 4);
  ADD_INSTR(0x97, UN_SAX, AddressingMode::kZeroPageY, 4);
  ADD_INSTR(0xEB, UN_SBC, AddressingMode::kImmediate, 2);
  ADD_INSTR(0xC3, UN_DCP, AddressingMode::kIndirectX, 8); // (d,x)
  ADD_INSTR(0xC7, UN_DCP, AddressingMode::kZeroPage, 5); // d
  ADD_INSTR(0xCF, UN_DCP, AddressingMode::kAbsolute, 6); // a
  ADD_INSTR(0xD3, UN_DCP, AddressingMode::kIndirectY, 7); // (d),Y 
  ADD_INSTR(0xD7, UN_DCP, AddressingMode::kZeroPageX, 6); // d,X
  ADD_INSTR(0xDB, UN_DCP, AddressingMode::kAbsoluteY, 6); // a,Y
  ADD_INSTR(0xDF, UN_DCP, AddressingMode::kAbsoluteX, 6); // a,X
  ADD_INSTR(0xE3, UN_ISB, AddressingMode::kIndirectX, 8); // (d,x)
  ADD_INSTR(0xE7, UN_ISB, AddressingMode::kZeroPage, 5); // d
  ADD_INSTR(0xEF, UN_ISB, AddressingMode::kAbsolute, 6); // a
  ADD_INSTR(0xF3, UN_ISB, AddressingMode::kIndirectY, 7); // (d),Y 
  ADD_INSTR(0xF7, UN_ISB, AddressingMode::kZeroPageX, 6); // d,X
  ADD_INSTR(0xFB, UN_ISB, AddressingMode::kAbsoluteY, 6); // a,Y
  ADD_INSTR(0xFF, UN_ISB, AddressingMode::kAbsoluteX, 6); // a,X
  ADD_INSTR(0x03, UN_SLO, AddressingMode::kIndirectX, 8); // (d,x)
  ADD_INSTR(0x07, UN_SLO, AddressingMode::kZeroPage, 5); // d
  ADD_INSTR(0x0F, UN_SLO, AddressingMode::kAbsolute, 6); // a
  ADD_INSTR(0x13, UN_SLO, AddressingMode::kIndirectY, 8); // (d),Y 
  ADD_INSTR(0x17, UN_SLO, AddressingMode::kZeroPageX, 6); // d,X
  ADD_INSTR(0x1B, UN_SLO, AddressingMode::kAbsoluteY, 7); // a,Y
  ADD_INSTR(0x1F, UN_SLO, AddressingMode::kAbsoluteX, 7); // a,X

  VDBG("Instruction set built.\n");
}
