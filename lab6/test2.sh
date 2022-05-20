#!/bin/sh
size=10000
file=big.txt
set -x


gcc main.c vfs.c -o out

./out c $size

./out v $file

./out i

./out l

./out d

ls -l
