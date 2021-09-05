#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color


make clean
make nes2x TEST_DEFINES="-U DEBUG -D NESTEST"
./nes2x /Users/river/code/nes/roms/nestest.nes 2>&1 | tee test/out.log

## Ignore PPU:
sed 's/PPU:.*C/CYC/g' test/out.log > test/out-noppu.log
diff --brief test/out-noppu.log test/nestest_golden-noppu.log

if [[ $? -eq 1 ]]; then
  code --diff test/out-noppu.log test/nestest_golden-noppu.log
else
  echo -e "${GREEN}PASSED${NC} -- nes2x output matches nestest.nes golden (cpu-only)"
fi

## Don't ignore PPU:
# diff --brief test/out.log test/nestest_golden.log

# if [ $? ]; then
#   code --diff test/out.log test/nestest_golden.log
# fi
