#!/bin/bash

set -x
set -e

# DEPENDENCIES
choco install cmake llvm ninja -y
# chocolatey doesn't add cmake to PATH properly, probably because we're using
# bash. (--installargs '"ADD_CMAKE_TO_PATH=User"' had no effect)
export PATH="/c/Program Files/CMake/bin:$PATH"

# CONFIGURAITON
mkdir -p build
cd build
cmake ..

# BUILD
cmake --build . --config Release

# TEST
./source/Release/PeqBankCLI.exe ../