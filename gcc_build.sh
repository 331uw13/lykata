#!/bin/bash

set -xe

if gcc -O2 $(find ./src -type f -name *.c)\
    -Wall -Wextra $(pkg-config --cflags --libs ncurses) $(pkg-config --cflags --libs json-c) -o lyk ; then
    #strip lyk
    ls -alh lyk
fi
