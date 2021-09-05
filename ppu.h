#ifndef PPU_H_
#define PPU_H_

#include "common.h"
#include "image.h"

// https://wiki.nesdev.com/w/images/d/d1/Ntsc_timing.png
// https://www.reddit.com/r/EmuDev/comments/7k08b9/not_sure_where_to_start_with_the_nes_ppu/

// Per-scanline rendering engine.
// PPU render 262 scanlines per frame. 240 scanlines are visible (224 after overscan).

constexpr int kFrameX = 256;
constexpr int kFrameY = 240;

class Ppu {
  public:
    // Copies chr into chr_  if not null.
    Ppu(uint8_t* chr, size_t chr_size);
    ~Ppu();

    // Runs a scanline's worth of cycles.
    // TODO: Figure out HBlank
    void Update();

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
    void SetOAMDMA(uint8_t* data);

    // Set the contents of the latch.
    void SetLatch(uint8_t val) { latch_ = val; }
    // Returns the contents of the latch. Used when reading write-only ports.
    uint8_t GetLatch() { return latch_; }

    Image* FrameBuffer() { return frame_buffer_.get(); }
  
    void DbgChr();

  private:
    void SetPpuStatusLSBits(uint8_t val); // sets bits 0-4 of ppustatus

    // Writes to the frame_buffer_.
    void RenderScanline(int line);

    uint8_t* chr_;  // CHR_ROM or CHR_RAM -> pattern tables?
    size_t chr_size_;

    uint8_t nametable_ram_[2048] = {}; // 2kB of VRAM
    uint8_t palette_ram_[32] = {};
    uint8_t oam_[256] = {};

    uint8_t latch_ = 0;

    bool next_ppuscroll_write_is_x_ = true; // otherwise y
    bool next_ppuaddr_write_is_msb_ = true; // otherwise lsb

    uint8_t ppuctrl_ = 0;
    uint8_t ppumask_ = 0;
    uint8_t ppustatus_ = 0;
    uint8_t oamaddr_ = 0;
    uint8_t ppuscroll_x_ = 0;
    uint8_t ppuscroll_y_ = 0;
    uint16_t ppuaddr_ = 0;

    // Current cycle number. Cycle 7 means 7 cycles have elapsed.
    // 1 CPU cycle = 3 PPU cycles. Each scanline is 341 PPU cycles (113.667 CPU cycles).
    uint64_t cycle_ = 0;
    // Current scanline. May not be a visible scanline.
    uint16_t scanline_ = 0;

    // 256x240 RGB24 frame buffer. We render to this, then upload to the GPU for display.
    // After overscan we crop to 256x224 for display.
    std::unique_ptr<Image> frame_buffer_;
};

// 0x0000 - 0x1FFF is pattern memory (CHR). Usually mapper can bank this.
// 0x2000 - 0x2FFF is nametable memory (VRAM)
//   0x2000 - 0x27FF (size 0x800) then mirrored at 0x2800 - 0x2FFF
// 0x3000 - 0x3EFF is a mirror of 0x2000 - 0x2EFF (size 0xF00)
// 0x3F00 - 0x3F1F is pallete memory
// 0x32F0 - 0x3FFF are mirrors of 0x3F00 - 0x3F1F


// PPU render 262 scanlines per frame. 240 scanlines are visible (224 after overscan).
// Scanline: 256 px wide. Each cycle is a px [0..255].
//    Hblank from [256..340]. A "scanline" includes hblank.
//    So each scanline is 341 PPU cycles and 113.667 CPU cycles.

#endif // PPU_H_