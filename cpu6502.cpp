#include "cpu6502.h"

#include "common.h"

Cpu6502::Cpu6502(const std::string& rom_path) {
  // LoadRom
  mapper_ = std::make_unique<Mapper>(0);  // NROM
}