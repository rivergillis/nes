#include "mappers/nrom_mapper.h"

#include "mapper_id.h"

  // public:
  //   NromMapper(::MapperId type);

  //   uint8_t Get(uint16_t addr);
  //   void Set(uint16_t addr, uint8_t val);

  //   ::MapperId MapperId() const;
  // private:
  //   const ::MapperId type_;

NromMapper::NromMapper(::MapperId type) {
  mapper_id_ = type;

  if (mapper_id_ == MapperId::kNrom128) {
    prg_rom_ = (uint8_t*)calloc(0x4000, sizeof(uint8_t)); // 16k
  } else if (mapper_id_ == MapperId::kNrom256) {
    prg_rom_ = (uint8_t*)calloc(0x8000, sizeof(uint8_t)); // 32k
  } else {
    throw std::runtime_error("Invalid mapper for nrom");
  }

  prg_ram_ = (uint8_t*)calloc(0x2000, sizeof(uint8_t)); // 8k
}

uint8_t NromMapper::Get(uint16_t addr) {
  if (addr < 0x6000) {
    throw std::runtime_error("Invalid read addr");
  } else if (addr < 0x8000) {
    // I think providing 8k ram here is wrong, but it should be fine...
    return prg_ram_[addr - 0x6000];
  } else if (addr < 0xC000) {
    return prg_rom_[addr - 0x8000];
  } else {
    if (mapper_id_ == MapperId::kNrom128) {
      // Provide a mirror
      return prg_rom_[addr - 0xC000];
    } else {
      return prg_rom_[addr - 0x8000];
    }
  }
}

void NromMapper::Set(uint16_t addr, uint8_t val) {
  if (addr < 0x6000 || addr >= 0x8000) {
    throw std::runtime_error("Invalid write addr");
  } 
  prg_ram_[addr - 0x6000] = val;
}