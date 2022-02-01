# Delta - C11 generic containers inspired by Go

The delta library provides the following containers, implemented in C11:
* A "generic" vector.
* A "generic" string hashmap mapping C strings (`const char*`) keys to values of any type.

## Tutorial

The program [countchars.c](test/countchars.c) takes any number of arguments and prints count of each distinct character given in descending order, using a vector and a hash map.

To compile and run it:

```sh
cmake --build ./build  --target countchars
./build/countchars hello world!
```
```
char 'l' counted 3 time(s)
char 'o' counted 2 time(s)
char '!' counted 1 time(s)
char 'd' counted 1 time(s)
char 'r' counted 1 time(s)
char 'w' counted 1 time(s)
char 'e' counted 1 time(s)
char 'h' counted 1 time(s)
```
