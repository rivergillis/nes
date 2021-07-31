#ifndef MAPPER_H_
#define MAPPER_H_

#include "mapper_id.h"
#include "common.h"

// Handles memory 0x4020 - 0xFFFF
class Mapper {
  public:
    virtual uint8_t Get(uint16_t addr) = 0;
    virtual void Set(uint16_t addr, uint8_t val) = 0;

    ::MapperId MapperId() { return mapper_id_; }

    virtual ~Mapper() {
      free(prg_rom_);
      free(prg_ram_);
    }

    // For some reason private inheritence wont work here...
    ::MapperId mapper_id_ = ::MapperId::kUndefined;
    uint8_t* prg_rom_ = nullptr;
    uint8_t* prg_ram_ = nullptr;
};

#endif // MAPPER_H_
