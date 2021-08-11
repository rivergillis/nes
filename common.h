#ifndef C8_COMMON_H_
#define C8_COMMON_H_

#include <cstdint>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <exception>
#include <stdexcept>
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
  return Bit(7, byte) == 0;
}

// Returns true if lhs and rhs do not share the same page (MSB)
// this needs to be inline otherwise linker errors sorry future me
inline bool CrossedPage(uint16_t lhs, uint16_t rhs) {
  return static_cast<uint8_t>(lhs >> 8) != static_cast<uint8_t>(rhs >> 8);
}

// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf  for C++11
template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    auto buf = std::make_unique<char[]>( size );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#endif