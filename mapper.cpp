#include "mapper.h"

#include "common.h"
#include "ppu.h"

Mapper::Mapper(uint8_t* cpu_ram, Ppu* ppu, uint8_t* apu_ram, uint8_t* prg_rom, size_t prg_rom_size) {
  cpu_ram_ = cpu_ram;
  ppu_ = ppu;
  apu_ram_ = apu_ram;
  mapper_id_ = MapperId::kNrom;
  prg_rom_size_ = prg_rom_size;
  // TODO: Maybe the subclass should do this part?
  prg_rom_ = (uint8_t*)malloc(prg_rom_size_ * sizeof(uint8_t));
  memcpy(prg_rom_, prg_rom, prg_rom_size_);

  prg_ram_ = (uint8_t*)calloc(0x2000, sizeof(uint8_t)); // 8k

  assert(cpu_ram_ && ppu_ && apu_ram_ && prg_rom_);
}

uint16_t Mapper::Get16(uint16_t addr, bool page_wrap) {
  if (!page_wrap) {
    return static_cast<uint16_t>(Get(addr + 1)) << 8 | Get(addr);
  } else {
    uint16_t msb_addr = addr + 1;
    if (CrossedPage(msb_addr, addr)) {
      msb_addr -= 0x100;
    }
    return static_cast<uint16_t>(Get(msb_addr)) << 8 | Get(addr);
  }
}