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

    // PPUCTRL
    void CTRL(uint8_t val);
  
    void DbgChr();

  private:
    uint8_t* chr_;  // CHR_ROM or CHR_RAM
    size_t chr_size_;

    uint8_t nametable_ram_[2048] = {}; // 2kB of VRAM
    uint8_t palette_ram_[32] = {};
    uint8_t oam_[256] = {};

    // PPUCTRL registers
    uint16_t base_nametable_addr_ = 0;
    bool vram_increment_down_ = false;
    uint16_t sprite_pattern_table_addr_8x8_ = 0;
    uint16_t bg_pattern_table_addr_ = 0;
    bool sprite_size_8x16_ = false;
    bool master_ = false;
    bool generate_nmi_ = false;
};

// 0x0000 - 0x1FFF is pattern memory (CHR). Usually mapper can bank this.
// 0x2000 - 0x2FFF is nametable memory (VRAM)
//   0x2000 - 0x27FF (size 0x800) then mirrored at 0x2800 - 0x2FFF
// 0x3000 - 0x3EFF is a mirror of 0x2000 - 0x2EFF (size 0xF00)
// 0x3F00 - 0x3F1F is pallete memory
// 0x32F0 - 0x3FFF are mirros of 0x3F00 - 0x3F1F

#endif // PPU_H_