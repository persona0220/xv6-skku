#!/bin/bash

sudo ctags -R 
find `pwd` -name "*.c" -o -name "*.h" | cscope -q -R -b

