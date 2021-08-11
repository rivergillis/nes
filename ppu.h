#ifndef PPU_H_
#define PPU_H_

#include "common.h"

// https://wiki.nesdev.com/w/images/d/d1/Ntsc_timing.png
// https://www.reddit.com/r/EmuDev/comments/7k08b9/not_sure_where_to_start_with_the_nes_ppu/

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