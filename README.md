# Delta - C11 generic containers inspired by Go

The delta library provides the following containers, implemented in C89:
* A "generic" string hashmap mapping C strings (`const char*`) keys to values of any type.
* A "generic" vector.

## Howto

### Install

Clone the repository:
```sh
git clone git@github.com:trezz/delta.git
mkdir delta/build
cd delta/build
```

To install the library under `~/.local`:
```sh
mkdir -p ~/.local
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local
make install
```

### Tutorial

The program [countchars.c](test/countchars.c) takes any number of arguments and prints count of each distinct character given in descending order, using a vector and a hash map.

To compile and run it:

```sh
clang ./test/countchars.c -I ~/.local/include -L ~/.local/lib -ldelta -o countchars
./countchars hello world!
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
