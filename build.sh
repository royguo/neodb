#!/bin/bash

BUILD_HOME=$PWD/build

mkdir -p $BUILD_HOME

cd $BUILD_HOME && \
  cmake ../ -DCMAKE_BUILD_TYPE=Debug -DWITH_ASAN=ON
