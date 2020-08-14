#!/bin/bash

set -x
set -e

# DEPENDENCIES
brew install cmake

# CONFIGURE
mkdir -p build
cd build
cmake .. -GXcode -D=IOS

# BUILD
xcodebuild -target PeqBankCLI -configuration Release

# TEST
./source/Release/IOSPeqBankCLI ../