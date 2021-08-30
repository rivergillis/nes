#include "mappers/nrom_mapper.h"

#include "mapper_id.h"

NromMapper::NromMapper(uint8_t* prg_rom, size_t prg_rom_size) {
  mapper_id_ = MapperId::kNrom;
  assert(prg_rom_size == 0x4000 || prg_rom_size == 0x8000);
  prg_rom_size_ = prg_rom_size;
  prg_rom_ = (uint8_t*)malloc(prg_rom_size_ * sizeof(uint8_t));
  memcpy(prg_rom_, prg_rom, prg_rom_size_);

  prg_ram_ = (uint8_t*)calloc(0x2000, sizeof(uint8_t)); // 8k
  DBG("Created NROM mapper with %llu byte PRG_ROM and 8k PRG_RAM\n", static_cast<uint64_t>(prg_rom_size_));
}

uint8_t NromMapper::Get(uint16_t addr) {
  if (addr < 0x6000) {
    throw std::runtime_error("Invalid read addr");
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
  if (addr < 0x6000 || addr >= 0x8000) {
    throw std::runtime_error("Invalid write addr");
  } 
  prg_ram_[addr - 0x6000] = val;
}