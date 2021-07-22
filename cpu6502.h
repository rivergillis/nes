#ifndef NES_CPU6502_H_
#define NES_CPU6502_H_

// Implements the NES's MOS 6502 CPU.
class Cpu6502 {

};

// notes:
// NES lacks decimal mode.
// little-endian - lsbyte first in memory
// registers:
// A: byte-wide, supports using the status register for carrying, overflow detection, etc
// X,Y: byte-wide indexes for several addressing modes.
//  only X can be used to get a copy of the stack pointer or change its value 
// PC: 2-bytes. Points to next instr. PC is modified automatically during
//  instr execution. Interrupts, jumps, branches also modify.
// S: byte-wide stack pointer. Points to the low 8 bits (00-FF) of the next free location
//  on the stack at $0100 - $01FF. Decr on push, incr on pull. No overflow detection.
// P: 6 bits but byte-wide status register. C,Z,I,D (no effect),V,N,B

// mem notes:
// $0000-$07FF SIZE $0800 : 2KB internal RAM 
// $0800-$0FFF SIZE $0800 : mirror of $0000 - $07FF
// $1000-$17FF SIZE $0800 : mirror of $0000 - $07FF
// $1800-$1FFF SIZE $0800 : mirror of $0000 - $07FF
// $2000-$2007 SIZE $0008 : NES PPU registers
// $2008-$3FFF SIZE $1FF8 : mirrors of $2000 - $2007 (repeats every 8 bytes)
// $4000-$4017 SIZE $0018 : NES APU and I/O registers
// $4018-$401F SIZE $0008 : APU and I/o functionality that is normally disabled (CPU test mode)
// $4020-$FFFF SIZE $BFE0 : Cartidge space: PRG ROM, PRG RAM, and mapper registers

// Think of 256-byte "page". So internal RAM $0000-$07FF has 8 pages. 
// The optional PRG RAM chip on some carts is 32 pages of SRAM at $6000-$7FFF
// Indirect addressing relies on zero/direct page at $0000-$00FF.
// Stack instructions always access stack page at $0100-$01FF.


#endif