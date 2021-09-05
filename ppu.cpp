#include "ppu.h"

#include "common.h"
#include "image.h"

namespace {

bool IsVisibleScanline(uint16_t scanline) {
  return scanline <= 239;
}

} // namespace

Ppu::Ppu(uint8_t* chr, size_t chr_size) {
  if (chr == nullptr) {
    chr_ = nullptr;
    chr_size_ = 0;
  } else {
    chr_size_ = chr_size;
    chr_ = (uint8_t*)malloc(chr_size_ * sizeof(uint8_t));
    memcpy(chr_, chr, chr_size_);
  }
  frame_buffer_ = std::make_unique<Image>(kFrameX, kFrameY);
  DBG("Created PPU with %llu byte CHR\n", static_cast<uint64_t>(chr_size));
}

Ppu::~Ppu() {
  free(chr_);
}

void Ppu::Update() {
  scanline_++;
  cycle_ += 341;

  if (IsVisibleScanline(scanline_)) {
    RenderScanline(scanline_);
  }
}


uint8_t Ppu::GetMMAP(uint16_t addr) {
  if (addr < 0x2000) {
    if (addr >= chr_size_) {
      throw std::runtime_error("PPU[addr] access outside of CHR. Is this legal?");
    }
    return chr_[addr];
  } else if (addr < 0x3000) {
    return nametable_ram_[addr - 0x2000];
  } else if (addr < 0x3F00) {
    return nametable_ram_[addr - 0x3000];
  } else if (addr < 0x3F20) {
    return palette_ram_[addr - 0x3F00];
  } else if (addr < 0x4000) {
    return palette_ram_[0x3F00 + (addr % 0x20)];
  }
  throw std::runtime_error("PPU[addr] outside memory map range.");
}

void Ppu::SetMMAP(uint16_t addr, uint8_t val) {
  if (addr < 0x2000) {
    if (addr >= chr_size_) {
      throw std::runtime_error("PPU[addr] access outside of CHR. Is this legal?");
    }
    chr_[addr] = val;
  } else if (addr < 0x3000) {
    nametable_ram_[addr - 0x2000] = val;
  } else if (addr < 0x3F00) {
    nametable_ram_[addr - 0x3000] = val;
  } else if (addr < 0x3F20) {
    palette_ram_[addr - 0x3F00] = val;
  } else if (addr < 0x4000) {
    palette_ram_[0x3F00 + (addr % 0x20)] = val;
  }
  throw std::runtime_error("PPU[addr] outside memory map range.");
}

void Ppu::SetPpuStatusLSBits(uint8_t val) {
  for (int i = 0; i <= 4; ++i) {
    SetBit(i, ppustatus_, Bit(i, val));
  }
}

void Ppu::SetCTRL(uint8_t val) {
  ppuctrl_ = val;
  SetLatch(val);
}

void Ppu::SetMASK(uint8_t val) {
  ppumask_= val;
  SetLatch(val);
}

uint8_t Ppu::GetSTATUS() {
  SetPpuStatusLSBits(latch_);
  uint8_t res = ppustatus_;
  SetBit(7, ppustatus_, 0); // reading clears bit 7 after read.
  // TODO: Set bits 5, 6, 7. Potentially do this elsewhere.
  SetLatch(res);
  return res;
}

void Ppu::SetOAMADDR(uint8_t val) {
  oamaddr_ = val;
  SetLatch(val);
}

uint8_t Ppu::GetOAMDATA() {
  // re: https://wiki.nesdev.com/w/index.php?title=PPU_registers
  // > reads during vertical or forced blanking return the value from OAM at that address but do not increment
  // so should we increment otherwise?
  uint8_t val = oam_[oamaddr_];
  SetLatch(val);
  return val;
}

void Ppu::SetOAMDATA(uint8_t val) {
  // I don't think this counts as a register for ppustatus.
  // TODO: ignore writes/increments during rendering
  //  (on the pre-render line and the visible lines 0-239, provided either sprite or background rendering is enabled) 
  oam_[oamaddr_++] = val;
  SetLatch(val);
}

void Ppu::SetPPUSCROLL(uint8_t val) {
  if (next_ppuscroll_write_is_x_) {
    ppuscroll_x_ = val;
  } else {
    ppuscroll_y_ = val;
  }
  next_ppuscroll_write_is_x_ = !next_ppuscroll_write_is_x_;
  SetLatch(val);
}

void Ppu::SetPPUADDR(uint8_t val) {
  if (next_ppuaddr_write_is_msb_) {
    ppuaddr_ &= 0x00FF;
    ppuaddr_ |= (static_cast<uint16_t>(val) << 8);
  } else {
    ppuaddr_ &= 0xFF00;
    ppuaddr_ |= val;
  }
  next_ppuaddr_write_is_msb_ = !next_ppuaddr_write_is_msb_;
  SetLatch(val);
}

void Ppu::SetPPUDATA(uint8_t val) {
  SetMMAP(ppuaddr_, val);
  uint8_t inc_amt = Bit(2, GetMMAP(0x2000));
  ppuaddr_ += inc_amt;
  SetLatch(val);
}

uint8_t Ppu::GetPPUDATA() {
  uint8_t res = GetMMAP(ppuaddr_);
  uint8_t inc_amt = Bit(2, GetMMAP(0x2000));
  ppuaddr_ += inc_amt;
  SetLatch(res);
  return res;
}

void Ppu::SetOAMDMA(uint8_t* data) {
  // Upload arbitrary data to the PPU.
  assert(data);
  memcpy(oam_, data, 256);
}

void Ppu::RenderScanline(int line) {
  throw std::runtime_error("Cannot render yet!");
}

void Ppu::DbgChr() {
  for (int i = 0x0000; i < chr_size_; i += 0x10) {
    DBG("\nPPU[%03X]: ", i);
    for (int j = 0; j < 0x10; j++) {
      DBG("%#04x ", chr_[i + j]);
    }
  }
  DBG("\n");
}