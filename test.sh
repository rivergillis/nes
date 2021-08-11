#!/bin/bash

make nes
./nes 2>&1 | tee test/out.log

## Ignore PPU:
sed 's/PPU:.*C/CYC/g' test/out.log > test/out-noppu.log
diff --brief test/out-noppu.log test/nestest_golden-noppu.log

if [ $? ]; then
  code --diff test/out-noppu.log test/nestest_golden-noppu.log
fi

## Don't ignore PPU:
# diff --brief test/out.log test/nestest_golden.log

# if [ $? ]; then
#   code --diff test/out.log test/nestest_golden.log
# fi
