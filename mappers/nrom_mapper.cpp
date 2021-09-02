#include "mappers/nrom_mapper.h"

#include "mapper_id.h"

NromMapper::NromMapper(uint8_t* cpu_ram, Ppu* ppu, uint8_t* apu_ram_, uint8_t* prg_rom, size_t prg_rom_size)
      : Mapper(cpu_ram, ppu, apu_ram_, prg_rom, prg_rom_size) {
  assert(prg_rom_size_ == 0x4000 || prg_rom_size_ == 0x8000);
  DBG("Created NROM mapper with %llu byte PRG_ROM and 8k PRG_RAM\n", static_cast<uint64_t>(prg_rom_size_));
}

uint8_t NromMapper::Get(uint16_t addr) {
  if (addr < 0x2000) {
    return cpu_ram_[addr % 0x800];
  } else if (addr < 0x4000) {
    uint16_t ppu_addr = 0x2000 + (addr % 8);  // get mirror of $2000-$2007
    switch (ppu_addr) {
      case 0x2000:  // PPUCTRL
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUCTRL.");
      case 0x2001:  // PPUMASK
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read PPUMASK.");
      case 0x2002:  // PPUSTATUS
        return ppu_->GetSTATUS();
      case 0x2003:  // OAMADDR
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot read OAMADDR.");
      case 0x2004:  // OAMDATA
        return ppu_->GetOAMDATA();
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
  } else if (addr <= 0x4020) {
    return apu_ram_[addr % 0x4000];
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
    cpu_ram_[addr % 0x800] = val;
  } else if (addr < 0x4000) {
    uint16_t ppu_addr = 0x2000 + (addr % 8);  // get mirror of $2000-$2007
    switch (ppu_addr) {
      case 0x2000:  // PPUCTRL
        ppu_->SetCTRL(val);
        break;
      case 0x2001:  // PPUMASK
        ppu_->SetMASK(val);
        break;
      case 0x2002:  // PPUSTATUS
        throw std::runtime_error("Invalid CPU->PPU addr -- cannot write PPUSTATUS.");
      case 0x2003:  // OAMADDR
        ppu_->SetOAMADDR(val);
        break;
      case 0x2004:  // OAMDATA
        ppu_->SetOAMDATA(val);
        break;
      case 0x2005:  // PPUSCROLL
        ppu_->SetPPUSCROLL(val);
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
  } else if (addr <= 0x4020) {
    apu_ram_[addr % 0x4000] = val;
  } else if (addr < 0x6000 || addr >= 0x8000) {
    throw std::runtime_error("Invalid write addr");
  } 
  prg_ram_[addr - 0x6000] = val;
}