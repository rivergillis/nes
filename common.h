#ifndef C8_COMMON_H_
#define C8_COMMON_H_

#include <cstdint>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <vector>

template <typename T>
uint8_t Bit(uint8_t bit_pos, T val) {
  return (val >> bit_pos) & 1;
}

// Always checks bit 7 (for bytes).
template <typename T>
bool Pos(T byte) {
  return Bit(7, byte) == 1;
}

#endif