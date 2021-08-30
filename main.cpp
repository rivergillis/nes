#include <iostream>
#include <string>

#include "cpu6502.h"
#include "common.h"

// doc: https://www.qmtpro.com/~nes/misc/nestest.txt
// good log: https://www.qmtpro.com/~nes/misc/nestest.log
// start execution at $C000
const std::string kTestRomPath = "/Users/river/code/nes/roms/nestest.nes";

void Run() {
  Cpu6502 cpu(kTestRomPath);
  // auto start_time = Clock::now();
  for (int i = 0; i < 8991; i++) {
    cpu.RunCycle();
  }
  // std::cout << "Executed 8991 instructions in " << StringMsSince(start_time) << std::endl;
}

int main(int argc, char* args[]) {
  try {
    Run();
    // std::cout << "Exit main() success" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
