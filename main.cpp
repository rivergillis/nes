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
      #ifdef DEBUG
      auto start_time = Clock::now();
      #endif
  for (int i = 0; i < 8991; i++) {
    cpu.RunCycle();
  }
  DBG( "Executed 8991 instructions in %s\n", StringMsSince(start_time).c_str());
}

int main(int argc, char* args[]) {
  try {
    Run();
    DBG("Exit main() success\n");
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
