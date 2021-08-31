#include "mappers/nrom_mapper.h"

#include "mapper_id.h"

NromMapper::NromMapper(Ppu* ppu, uint8_t* prg_rom, size_t prg_rom_size) {
  ppu_ = ppu;
  mapper_id_ = MapperId::kNrom;
  assert(prg_rom_size == 0x4000 || prg_rom_size == 0x8000);
  prg_rom_size_ = prg_rom_size;
  prg_rom_ = (uint8_t*)malloc(prg_rom_size_ * sizeof(uint8_t));
  memcpy(prg_rom_, prg_rom, prg_rom_size_);

  prg_ram_ = (uint8_t*)calloc(0x2000, sizeof(uint8_t)); // 8k
  DBG("Created NROM mapper with %llu byte PRG_ROM and 8k PRG_RAM\n", static_cast<uint64_t>(prg_rom_size_));
}

// TODO: Read/write invalid PPU ports should affect the latch status...
// https://wiki.nesdev.com/w/index.php?title=PPU_registers

uint8_t NromMapper::Get(uint16_t addr) {
  if (addr < 0x2000) {
    throw std::runtime_error("Invalid read addr");  // internal RAM
  } else if (addr < 0x4000) {
    uint16_t ppu_addr = 0x2000 + (addr % 8);  // get mirror of $2000-$2007
    switch (ppu_addr) {
      case 0x2000:  // PPUCTRL
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUCTRL.");
      case 0x2001:  // PPUMASK
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUMASK.");
      case 0x2002:  // PPUSTATUS
        throw std::runtime_error("unimplemented PPUSTATUS read");
        break;
      case 0x2003:  // OAMADDR
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read OAMADDR.");
      case 0x2004:  // OAMDATA
        throw std::runtime_error("unimplemented OAMDATA read");
        break;
      case 0x2005:  // PPUSCROLL
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUSCROLL.");
      case 0x2006:  // PPUADDR
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUADDR.");
      case 0x2007:  // PPUDATA
        throw std::runtime_error("unimplemented PPUDATA read");
        break;
      default:
        throw std::runtime_error("Invalid CPU->PPU addr -- default.");
    }
  } else if (addr < 0x6000) {
    throw std::runtime_error("Invalid read addr");  // Battery Backed Save or Work RAM
  } else if (addr < 0x8000) {
    // I think providing 8k ram here is wrong, but it should be fine...
    return prg_ram_[addr - 0x6000];
  } else if (addr < 0xC000) {
    return prg_rom_[addr - 0x8000];
  } else {
    if (prg_rom_size_ == 0x4000) {
      // Provide a mirror for NROM-128
      return prg_rom_[addr - 0xC000];
    } else {
      return prg_rom_[addr - 0x8000];
    }
  }
}

void NromMapper::Set(uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {
    throw std::runtime_error("Invalid write addr");  // internal RAM
  } else if (addr < 0x4000) {
    uint16_t ppu_addr = 0x2000 + (addr % 8);  // get mirror of $2000-$2007
    switch (ppu_addr) {
      case 0x2000:  // PPUCTRL
        ppu_->CTRL(val);
        break;
      case 0x2001:  // PPUMASK
        throw std::runtime_error("unimplemented PPUMASK write");
        break;
      case 0x2002:  // PPUSTATUS
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot write PPUSTATUS.");
      case 0x2003:  // OAMADDR
        throw std::runtime_error("unimplemented OAMDDR write");
        break;
      case 0x2004:  // OAMDATA
        throw std::runtime_error("unimplemented OAMDATA write");
        break;
      case 0x2005:  // PPUSCROLL
        throw std::runtime_error("unimplemented PPUSCROLL write");
        break;
      case 0x2006:  // PPUADDR
        throw std::runtime_error("unimplemented PPUADDR write");
        break;
      case 0x2007:  // PPUDATA
        throw std::runtime_error("unimplemented PPUDATA write");
        break;
      default:
        throw std::runtime_error("Invalid PPU addr -- default.");
    }
  } else if (addr == 0x4014) {  // OAMDMA
    throw std::runtime_error("UNIMPLEMENTED OAM DMA (write $4014");
  } else if (addr < 0x6000 || addr >= 0x8000) {
    throw std::runtime_error("Invalid write addr");
  } 
  prg_ram_[addr - 0x6000] = val;
}