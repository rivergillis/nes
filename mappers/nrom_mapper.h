#ifndef MAPPERS_NROM_MAPPER_H_
#define MAPPERS_NROM_MAPPER_H_

#include "mapper.h"
#include "mapper_id.h"
#include "common.h"

class NromMapper : public Mapper {
  public:
    NromMapper(::MapperId type);

    uint8_t Get(uint16_t addr);
    void Set(uint16_t addr, uint8_t val);
};

#endif