#include "ppu.h"

#include "common.h"

Ppu::Ppu(uint8_t* chr, size_t chr_size) {
  if (chr == nullptr) {
    chr_ = nullptr;
    chr_size_ = 0;
  } else {
    chr_size_ = chr_size;
    chr_ = (uint8_t*)malloc(chr_size_ * sizeof(uint8_t));
    memcpy(chr_, chr, chr_size_);
    DBG("Created PPU with %llu byte CHR\n", static_cast<uint64_t>(chr_size));
  }
}

Ppu::~Ppu() {
  free(chr_);
}

void Ppu::SetPpuStatusLSBits(uint8_t val) {
  for (int i = 0; i <= 4; ++i) {
    SetBit(i, ppustatus_, Bit(i, val));
  }
}

void Ppu::SetCTRL(uint8_t val) {
  ppuctrl_ = val;
  SetPpuStatusLSBits(val);
}

void Ppu::SetMASK(uint8_t val) {
  ppumask_= val;
  SetPpuStatusLSBits(val);
}

uint8_t Ppu::GetSTATUS() {
  uint8_t res = ppustatus_;
  SetBit(7, ppustatus_, 0); // reading clears bit 7 after read.
  // TODO: Set bits 5, 6, 7. Potentially do this elsewhere.
  return res;
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