#ifndef MAPPER_H_
#define MAPPER_H_

#include "mapper_id.h"
#include "ppu.h"
#include "common.h"

// Handles memory 0x4020 - 0xFFFF
class Mapper {
  public:
    Mapper(uint8_t* cpu_ram, Ppu* ppu, uint8_t* apu_ram, uint8_t* prg_rom, size_t prg_rom_size);

    virtual uint8_t Get(uint16_t addr) = 0;
    virtual void Set(uint16_t addr, uint8_t val) = 0;
    uint16_t Get16(uint16_t addr, bool page_wrap = false);

    ::MapperId MapperId() { return mapper_id_; }

    virtual ~Mapper() {
      free(prg_rom_);
      free(prg_ram_);
    }

    // For some reason private inheritence wont work here...
    ::MapperId mapper_id_ = ::MapperId::kUndefined;

    uint8_t* cpu_ram_ = nullptr;  // points to start of the 2kb internal ram.
    Ppu* ppu_ = nullptr;
    uint8_t* apu_ram_ = nullptr;  // 32-byte APU ram owned by CPU
    uint8_t* prg_rom_ = nullptr;
    size_t prg_rom_size_ = 0;
    uint8_t* prg_ram_ = nullptr;
};

#endif // MAPPER_H_
