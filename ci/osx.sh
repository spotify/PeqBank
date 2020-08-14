#!/bin/bash

set -x
set -e

# DEPENDENCIES
brew install cmake

# CONFIGURE
mkdir -p build
cd build
cmake .. -GXcode

# BUILD
xcodebuild -target PeqBankCLI -configuration Release

# TEST
./source/Release/PeqBankCLI ../