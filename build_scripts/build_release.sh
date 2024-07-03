#!/bin/sh

BUILD_DIR=../build/
VST3_SRC_DIR=../vst3

./get_pffft.sh

[ ! -d $BUILD_DIR ] && mkdir -p $BUILD_DIR
cmake -B$BUILD_DIR -DCMAKE_BUILD_TYPE="Release" $VST3_SRC_DIR
cmake --build $BUILD_DIR