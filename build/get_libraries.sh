#!/bin/sh

printf "\n\nGETTING EXTERNAL LIBRARIES...\n"

JUCE_VERSION="8.0.1"
PFFFT_COMMIT_ID="fbc4058" # using specific commit tag instead of version number

SRC_DIR="$PWD/../src"
WORK_DIR="$PWD/libraries_work_tmp"

JUCE_DIR="$SRC_DIR/juce"
PFFFT_DIR="$SRC_DIR/pffft"

rm -rf $WORK_DIR

mkdir -p $WORK_DIR
cd $WORK_DIR

if [ ! -d "$JUCE_DIR" ]
then
  wget "https://github.com/juce-framework/JUCE/archive/refs/tags/$JUCE_VERSION.zip" -O "juce-$JUCE_VERSION.zip" || exit 1
  mkdir -p "$JUCE_DIR" || exit 1
  unzip -qq "juce-$JUCE_VERSION.zip" -d "$JUCE_DIR" || exit 1
fi

if [ ! -d "$PFFFT_DIR" ]
then
  wget "https://bitbucket.org/jpommier/pffft/get/$PFFFT_COMMIT_ID.tar.gz" -O "pffft-$PFFFT_COMMIT_ID.tar.gz" || exit 1
  mkdir -p "$PFFFT_DIR" || exit 1
  tar -xzf "pffft-$PFFFT_COMMIT_ID.tar.gz" -C "$PFFFT_DIR" --strip-components=1 || exit 1
fi

rm -rf $WORK_DIR

printf "DONE!\n"