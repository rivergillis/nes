#include "memory_view.h"

#include "common.h"
#include "mapper.h"

MemoryView::MemoryView(uint8_t* system_ram, Mapper* mapper) : system_ram_(system_ram), mapper_(mapper) {}

uint8_t MemoryView::Get(uint16_t addr) {
  if (addr < 0x2000) {
    return system_ram_[addr];
  } else {
    // TODO: Handle 0x2000 thru 0x4019
    return mapper_->Get(addr);
  }
}

void MemoryView::Set(uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {
    system_ram_[addr] = val;
  } else {
    // TODO: Handle 0x2000 thru 0x4019
    mapper_->Set(addr, val);
  }
}