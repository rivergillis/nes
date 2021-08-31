#ifndef MAPPERS_NROM_MAPPER_H_
#define MAPPERS_NROM_MAPPER_H_

#include "mapper.h"
#include "mapper_id.h"
#include "common.h"
#include "ppu.h"

class NromMapper : public Mapper {
  public:
    // TODO: Move the PPU (and the rest?) to mapper.h
    NromMapper(Ppu* ppu, uint8_t* prg_rom, size_t prg_rom_size);

    uint8_t Get(uint16_t addr);
    void Set(uint16_t addr, uint8_t val);

  private:
    Ppu* ppu_;
    size_t prg_rom_size_;
};

#endif