#include "memory_view.h"

#include "common.h"
#include "mapper.h"

MemoryView::MemoryView(uint8_t* system_ram, uint8_t* apu_ram, Mapper* mapper) : 
    system_ram_(system_ram), apu_ram_(apu_ram), mapper_(mapper) {}

uint8_t MemoryView::Get(uint16_t addr) {
  if (addr < 0x2000) {
    return system_ram_[addr % 0x800];
  } else if (addr < 0x4000) {
    return mapper_->Get(addr);  // mapper handles CPU->PPU
  } else if (addr <= 0x4020) {
    return apu_ram_[addr % 0x4000];
  } else {
    return mapper_->Get(addr);
  }
}

uint16_t MemoryView::Get16(uint16_t addr, bool page_wrap) {
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

void MemoryView::Set(uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {
    system_ram_[addr % 0x800] = val;
  } else if (addr < 0x4000) {
    mapper_->Set(addr, val);  // mapper handles CPU->PPU
  } else if (addr < 0x4020) {
    apu_ram_[addr % 0x4000] = val;
  } else { 
    mapper_->Set(addr, val);
  }
}