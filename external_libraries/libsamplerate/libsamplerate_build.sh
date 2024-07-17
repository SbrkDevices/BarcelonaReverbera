#!/bin/bash

# Target platform
TARGET_PLAT="linux_x86_64"
#TARGET_PLAT="mac_x86_64"
#TARGET_PLAT="mac_apple_silicon"

LIBSAMPLERATE_VERSION="0.2.2"

printf "\n\nBUILDING LIBSAMPLERATE v$LIBSAMPLERATE_VERSION FOR TARGET PLATFORM '$TARGET_PLAT'...\n"

###############################################################################

LIBSAMPLERATE="libsamplerate-$LIBSAMPLERATE_VERSION"

OUTPUT_DIR="$PWD/output_${TARGET_PLAT}"
WORK_DIR="$PWD/work_${TARGET_PLAT}"

###############################################################################

if [ $TARGET_PLAT = "linux_x86_64" ]
then
  echo "Checking build machine to make sure it is appropriate for \"linux_x86_64\"..."
  GCC_DUMPMACHINE=$(gcc -dumpmachine)
  [[ $GCC_DUMPMACHINE != "x86_64-linux-gnu" ]] && exit 1
  echo "Build machine ok ($GCC_DUMPMACHINE)"

  export CFLAGS="-O3 -ffast-math -funroll-loops -ftree-vectorize -fomit-frame-pointer -fPIC"
  export CXXFLAGS=$CFLAGS
elif [ $TARGET_PLAT = "mac_x86_64" ]
then
  echo "Checking build machine to make sure it is appropriate for \"mac_x86_64\"..."
  GCC_DUMPMACHINE=$(gcc -dumpmachine)
  [[ $GCC_DUMPMACHINE != "x86_64-apple-darwin"* ]] && exit 1
  echo "Build machine ok ($GCC_DUMPMACHINE)"

  export CFLAGS="-O3 -ffast-math -funroll-loops -ftree-vectorize -fomit-frame-pointer -fPIC -mmacosx-version-min=10.9"
  export CXXFLAGS=$CFLAGS
elif [ $TARGET_PLAT = "mac_apple_silicon" ]
then
  echo "Checking build machine to make sure it is appropriate for \"mac_apple_silicon\"..."
  GCC_DUMPMACHINE=$(gcc -dumpmachine)
  [[ $GCC_DUMPMACHINE != "arm64-apple-darwin"* ]] && exit 1
  echo "Build machine ok ($GCC_DUMPMACHINE)"

  export CFLAGS="-O3 -ffast-math -funroll-loops -ftree-vectorize -fomit-frame-pointer -fPIC -mmacosx-version-min=10.9"
else
  echo "ERROR: Invalid TARGET_PLAT"
  exit 1
fi

rm -rf $OUTPUT_DIR
rm -rf $WORK_DIR/$LIBSAMPLERATE

mkdir -p $OUTPUT_DIR
mkdir -p $WORK_DIR
cd $WORK_DIR

if [ ! -e "$LIBSAMPLERATE.tar.gz" ]
then
  wget "https://github.com/libsndfile/libsamplerate/archive/refs/tags/$LIBSAMPLERATE_VERSION.tar.gz" -O "$LIBSAMPLERATE.tar.gz" || exit 1
fi

tar xf "$LIBSAMPLERATE.tar.gz" || exit 1

# ./configure --help can be run (from the sources directory, after running "autoreconf -vif") to get a help message with all the options
CONFIG_FLAGS="${CONFIG_FLAGS_TARGET_PLAT} --enable-static --enable-werror --disable-cpu-clip --disable-sndfile --disable-alsa --disable-fftw --prefix=${OUTPUT_DIR}"

cd $WORK_DIR/$LIBSAMPLERATE
echo "Running ./configure $CONFIG_FLAGS..."
autoreconf -vif
./configure $CONFIG_FLAGS || exit 1

echo "Running make..."
make || exit 1

#echo "Running make check..."
#make check || exit 1

echo "Running make install..."
make install || exit 1

printf "\nBUILDING LIBSAMPLERATE v$LIBSAMPLERATE_VERSION DONE!\n\n"
