#include "cpu6502.h"

#include "common.h"
#include "mappers/nrom_mapper.h"
#include "mapper_id.h"
#include "memory_view.h"

Cpu6502::Cpu6502(const std::string& rom_path) {
  // LoadRom
  mapper_ = std::make_unique<NromMapper>(MapperId::kNrom256);
  memory_view_ = std::make_unique<MemoryView>(&internal_ram_, mapper_.get());
}