#!/bin/bash
rm -f LOG
gcc -O3 radix_sort.c
max_key=1
while [ $max_key -lt 1073741824 ]
do
./a.out $max_key >> LOG
max_key=$(($max_key*2))
done