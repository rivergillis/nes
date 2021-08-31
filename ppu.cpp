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

void Ppu::CTRL(uint8_t val) {
  base_nametable_addr_ = 0x2000 + (0x400 * (Bit(1, val) + Bit(0, val)));  // somehow this is X/Y's MSB
  vram_increment_down_ = Bit(2, val);
  sprite_pattern_table_addr_8x8_ = Bit(3, val) ? 0x1000 : 0x0000;
  bg_pattern_table_addr_ = Bit(4, val) ? 0x1000 : 0x0000;
  sprite_size_8x16_ = Bit(5, val);
  master_ = Bit(6, val);
  generate_nmi_ = Bit(7, val);
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