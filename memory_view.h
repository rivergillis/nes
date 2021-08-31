#ifndef MEMORY_VIEW_H_
#define MEMORY_VIEW_H_

#include "mapper.h"
#include "common.h"

// Provides access to all system memory based on references
// This should be allocated after all references are allocated and
// freed before any references are freed.

class MemoryView {
  public:
    // TODO: move the apu_ram view into an APU class
    MemoryView(uint8_t* internal_ram, uint8_t* apu_ram, Mapper* mapper); 

    uint8_t Get(uint16_t addr);
    uint16_t Get16(uint16_t addr, bool page_wrap = false);
    void Set(uint16_t addr, uint8_t val);

  private:
    uint8_t* system_ram_; // CPU internal RAM
    uint8_t* apu_ram_;
    Mapper* mapper_;
};

#endif // MEMORY_VIEW_H_