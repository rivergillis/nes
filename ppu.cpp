#include "ppu.h"

#include "common.h"

namespace { 
void VDBG(const char* str, ...) {
  #ifdef VDEBUG
  va_list arglist;
  va_start(arglist, str);
  vprintf(str, arglist);
  va_end(arglist);
  #endif
}
}

Ppu::Ppu(uint8_t* chr, size_t chr_size) {
  if (chr == nullptr) {
    chr_ = nullptr;
    chr_size_ = 0;
  } else {
    chr_size_ = chr_size;
    chr_ = (uint8_t*)malloc(chr_size_ * sizeof(uint8_t));
    memcpy(chr_, chr, chr_size_);
    VDBG("Created PPU with %d byte CHR\n", chr_size);
  }
}

Ppu::~Ppu() {
  free(chr_);
}

void Ppu::DbgChr() {
  for (int i = 0x0000; i < chr_size_; i += 0x10) {
    VDBG("\nPPU[%03X]: ", i);
    for (int j = 0; j < 0x10; j++) {
      VDBG("%#04x ", chr_[i + j]);
    }
  }
  VDBG("\n");
}