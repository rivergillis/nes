#ifndef MAPPERS_NROM_MAPPER_H_
#define MAPPERS_NROM_MAPPER_H_

#include "mapper.h"
#include "mapper_id.h"
#include "common.h"
#include "ppu.h"

class NromMapper : public Mapper {
  public:
    NromMapper(uint8_t* cpu_ram, Ppu* ppu, uint8_t* apu_ram_, uint8_t* prg_rom, size_t prg_rom_size);

    uint8_t Get(uint16_t addr) override;
    uint16_t Set(uint16_t addr, uint8_t val, uint64_t current_cycle) override;
};

#endif