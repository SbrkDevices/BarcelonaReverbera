#!/bin/bash

# Define variables
PFFFT_DIR=$PWD/../vst3/external/pffft
REPO_URL="https://bitbucket.org/jpommier/pffft/get/fbc4058.tar.gz"
COMMIT_ID="fbc4058"

if [ ! -d "$PFFFT_DIR" ]; then
  curl -L "$REPO_URL" -o "$COMMIT_ID.tar.gz" || exit 1

  mkdir -p "$PFFFT_DIR" || exit 1

  tar -xzf "$COMMIT_ID.tar.gz" -C "$PFFFT_DIR" --strip-components=1 || exit 1

  rm "$COMMIT_ID.tar.gz" || exit 1
  
  echo "PFFFT downloaded and extracted into '$PFFFT_DIR'."
fi