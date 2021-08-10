#!/bin/bash

make nes
./nes 2>&1 | tee test/out.log
diff --brief test/out.log test/nestest_golden.log

if [ $? ]; then
  code --diff test/out.log test/nestest_golden.log
fi
