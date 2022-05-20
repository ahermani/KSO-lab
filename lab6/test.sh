#!/bin/sh
size=10000
file=abc.txt
set -x


cc main.c vfs.c -o out

./out c $size

ls -l

./out i

./out l

cat $file

./out v $file

./out i

./out l

rm -f $file
ls -l

./out p abc.txt

cat abc.txt

./out r abc.txt

./out l

./out i

./out d
ls -l
