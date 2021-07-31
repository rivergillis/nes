#include "cpu6502.h"

#include <fstream>

#include "common.h"
#include "mappers/nrom_mapper.h"
#include "mapper_id.h"
#include "memory_view.h"

Cpu6502::Cpu6502(const std::string& file_path) {
  // LoadRom
  mapper_ = std::make_unique<NromMapper>(MapperId::kNrom256);
  memory_view_ = std::make_unique<MemoryView>(internal_ram_, mapper_.get());
  LoadCartrtidgeFile(file_path);
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

}