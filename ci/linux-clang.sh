#!/bin/bash

set -x
set -e

# DEPENDENCIES
apt-get update
apt-get install -y ninja-build build-essential cmake clang

# CONFIGURATION
export CC=clang
export CXX=clang++
mkdir -p build
cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release

# BUILD
ninja PeqBankCLI

# TEST
./source/PeqBankCLI ../