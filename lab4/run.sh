#!/bin/sh
#./out > output.txt
grep 'Buffer 0' output.txt | tee buffer1.txt
grep 'Buffer 1' output.txt | tee buffer2.txt
grep 'Buffer 2' output.txt | tee buffer3.txt

