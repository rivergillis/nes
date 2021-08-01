#ifndef PPU_H_
#define PPU_H_

#include "common.h"

class Ppu {
  public:
    // Copies chr into chr_  if not null.
    Ppu(uint8_t* chr, size_t chr_size);
    ~Ppu();
  
    void DbgChr();

  private:
    uint8_t* chr_;  // CHR_ROM or CHR_RAM
    size_t chr_size_;

};

#endif // PPU_H_