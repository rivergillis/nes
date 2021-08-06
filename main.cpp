#include <iostream>
#include <string>

#include "cpu6502.h"

// doc: https://www.qmtpro.com/~nes/misc/nestest.txt
// good log: https://www.qmtpro.com/~nes/misc/nestest.log
// start execution at $C000
const std::string kTestRomPath = "/Users/river/code/nes/roms/nestest.nes";

void Run() {
  Cpu6502 cpu(kTestRomPath);
  for (int i = 0; i < 1000; i++) {
    cpu.RunCycle();
  }
}

int main(int argc, char* args[]) {
  try {
    Run();
    std::cout << "Exit main() success" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
