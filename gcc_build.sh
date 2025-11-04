#!/bin/bash

set -xe

gcc -O2 $(find ./src -type f -name *.c)\
    -Wall -Wextra $(pkg-config --cflags --libs ncurses) $(pkg-config --cflags --libs json-c) -o lykata
