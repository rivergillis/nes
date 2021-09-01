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

    uint8_t GetMMAP(uint16_t addr);
    void SetMMAP(uint16_t addr, uint8_t val);

    void SetCTRL(uint8_t val);
    void SetMASK(uint8_t val);
    uint8_t GetSTATUS();
    void SetOAMADDR(uint8_t val);
    uint8_t GetOAMDATA();
    void SetOAMDATA(uint8_t val);
    // Write twice. Horizontal then vertical.
    void SetPPUSCROLL(uint8_t val);
    // Write twice. MSB then LSB.
    void SetPPUADDR(uint8_t val);
    void SetPPUDATA(uint8_t val);
    uint8_t GetPPUDATA();
  
    void DbgChr();

  private:
    void SetPpuStatusLSBits(uint8_t val); // sets bits 0-4 of ppustatus

    uint8_t* chr_;  // CHR_ROM or CHR_RAM
    size_t chr_size_;

    uint8_t nametable_ram_[2048] = {}; // 2kB of VRAM
    uint8_t palette_ram_[32] = {};
    uint8_t oam_[256] = {};

    // TODO: Emulate the latch.

    bool next_ppuscroll_write_is_x_ = true; // otherwise y
    bool next_ppuaddr_write_is_msb_ = true; // otherwise lsb

    uint8_t ppuctrl_ = 0;
    uint8_t ppumask_ = 0;
    uint8_t ppustatus_ = 0;
    uint8_t oamaddr_ = 0;
    uint8_t ppuscroll_x_ = 0;
    uint8_t ppuscroll_y_ = 0;
    uint16_t ppuaddr_ = 0;
};

// 0x0000 - 0x1FFF is pattern memory (CHR). Usually mapper can bank this.
// 0x2000 - 0x2FFF is nametable memory (VRAM)
//   0x2000 - 0x27FF (size 0x800) then mirrored at 0x2800 - 0x2FFF
// 0x3000 - 0x3EFF is a mirror of 0x2000 - 0x2EFF (size 0xF00)
// 0x3F00 - 0x3F1F is pallete memory
// 0x32F0 - 0x3FFF are mirros of 0x3F00 - 0x3F1F

// ppuctrl
  // base_nametable_addr_ = 0x2000 + (0x400 * (Bit(1, val) + Bit(0, val)));  // somehow this is X/Y's MSB
  // vram_increment_down_ = Bit(2, val);
  // sprite_pattern_table_addr_8x8_ = Bit(3, val) ? 0x1000 : 0x0000;
  // bg_pattern_table_addr_ = Bit(4, val) ? 0x1000 : 0x0000;
  // sprite_size_8x16_ = Bit(5, val);
  // master_ = Bit(6, val);
  // generate_nmi_ = Bit(7, val);

#endif // PPU_H_