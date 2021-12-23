#! /usr/bin/env bash

root=`git rev-parse --show-toplevel`
cd $root

clang -g -std=c89 -Wall -pedantic -ansi -Wno-long-long  -c slice.c
clang -g -std=c89 -Wall -pedantic -ansi -Wno-long-long  -c sort.c
clang -g -std=c89 -Wall -pedantic -ansi -Wno-long-long  -c map.c
clang -g -std=c99 slice.o sort.o map.o test/slice_test.c -o test_slice
./test_slice
