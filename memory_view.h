#ifndef MEMORY_VIEW_H_
#define MEMORY_VIEW_H_

#include "mapper.h"
#include "ppu.h"
#include "common.h"

// Provides access to all system memory based on references
// This should be allocated after all references are allocated and
// freed before any references are freed.

class MemoryView {
  public:
    // TODO: Add other components like PPU and APU
    MemoryView(uint8_t* internal_ram, Ppu* ppu, Mapper* mapper); 

    uint8_t Get(uint16_t addr);
    void Set(uint16_t addr, uint8_t val);

  private:
    uint8_t* system_ram_; // CPU internal RAM
    Ppu* ppu_;
    Mapper* mapper_;
};

#endif // MEMORY_VIEW_H_