#! /usr/bin/env bash

root=`git rev-parse --show-toplevel`
cd $root

clang -g -std=c89 -Wall -pedantic -ansi -Wno-long-long  -c vec.c
clang -g -std=c89 -Wall -pedantic -ansi -Wno-long-long  -c strmap.c
clang -g -std=c99 vec.o strmap.o test/test.c -o runtest
./runtest
