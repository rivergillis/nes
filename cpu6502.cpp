#include "cpu6502.h"

#include <fstream>

#include "common.h"
#include "mappers/nrom_mapper.h"
#include "mapper_id.h"
#include "ppu.h"
#include "memory_view.h"

namespace { 
void DBG(const char* str, ...) {
  #ifdef DEBUG
  va_list arglist;
  va_start(arglist, str);
  vprintf(str, arglist);
  va_end(arglist);
  #endif
}
}

Cpu6502::Cpu6502(const std::string& file_path) {
  LoadCartrtidgeFile(file_path);
  memory_view_ = std::make_unique<MemoryView>(internal_ram_, ppu_.get(), mapper_.get());
  assert(ppu_);
  assert(mapper_);
  assert(memory_view_);
}

void Cpu6502::LoadCartrtidgeFile(const std::string& file_path) {
  std::ifstream input(file_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
  if (bytes.size() <= 0) {
    throw std::runtime_error("No file or empty file.");
  } else if (bytes.size() < 8) {
    throw std::runtime_error("Invalid file format.");
  }

  bool is_ines = false;
  if (bytes[0] == 'N' && bytes[1] == 'E' && bytes[2] == 'S' && bytes[3] == 0x1A) {
    is_ines = true;
  }

  bool is_nes2 = false;
  if (is_ines == true && (bytes[7]&0x0C)==0x08) {
    is_nes2 = true;
  }

  if (is_nes2) {
    std::cout << "Found " << bytes.size() << " byte NES 2.0 file." << std::endl;
    // TODO: Load NES 2.0 specific
    // Back-compat with ines 1.0
    LoadNes1File(bytes);
  } else if (is_ines) {
    std::cout << "Found " << bytes.size() << " byte iNES 1.0 file." << std::endl;
    LoadNes1File(bytes);
  } else {
    throw std::runtime_error("Rom file is not iNES format.");
  }
}

void Cpu6502::LoadNes1File(std::vector<uint8_t> bytes) {
  if (bytes.size() < 16) {
    throw std::runtime_error("Incomplete iNes header.");
  }

  uint32_t prg_rom_size = static_cast<uint32_t>(bytes[4]) * 0x4000;
  uint32_t chr_rom_size = static_cast<uint32_t>(bytes[5]) * 0x2000;

  if (bytes.size() < (prg_rom_size + chr_rom_size + 16)) {
    throw std::runtime_error("Rom file size less than header suggests.");
  }

  // TODO: Handle flags as needed.
  uint8_t flags6 = bytes[6];  // msb are lower nybble of mapper num
  if (flags6 & 0b0010'0000) {
    throw std::runtime_error("Rom has a trainer!");
  }
  uint8_t flags7 = bytes[7];  // lsb are upper nybble of mapper num
  uint8_t mapper_number = ((flags7 >> 4) << 4) | (flags6 >> 4);

  uint8_t prg_ram_size = bytes[8] == 0x0 ? static_cast<uint8_t>(0x2000) : bytes[8] * 0x2000;
  DBG("Mapper ID %d PRG_ROM sz %d CHAR_ROM sz %d PRG_RAM sz %d\n",
      mapper_number, prg_rom_size, chr_rom_size, prg_ram_size);
  
  if (chr_rom_size > 0) {
    ppu_ = std::make_unique<Ppu>(bytes.data() + 16 + prg_rom_size, chr_rom_size);
  } else {
    ppu_ = std::make_unique<Ppu>(nullptr, 0);
  }

  // memcpy(prg_rom_dst, bytes.data() + 16, prg_rom_size);
  mapper_ = std::make_unique<NromMapper>(bytes.data() + 16, prg_rom_size);
}