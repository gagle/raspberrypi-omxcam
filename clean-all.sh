#!/bin/bash

cd $(dirname $0)
dir=$(pwd -P)

make -f Makefile-shared clean

for file in $(find $dir -name Makefile); do
  cd $(dirname $file)
  make clean
done