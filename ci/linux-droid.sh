#!/bin/bash

set -x
set -e

# DEPENDENCIES
apt-get update
apt-get install -y curl default-jre ninja-build build-essential cmake unzip zip wget

# Gradle
# print location:
# $ update-alternatives --config java
# echo 'JAVA_HOME="/usr/lib/jvm/java-11-openjdk-amd64/bin/java"' >> /etc/environment
# curl -s "https://get.sdkman.io" | bash
# source "/root/.sdkman/bin/sdkman-init.sh"
# if sdk current | grep -q gradle;  then
#   echo 'gradle already installed'
# else
#   sdk install gradle 6.6
# fi;

# NDK
NDK_ZIP=android-ndk-r21d-linux-x86_64.zip
NDK_FOLDER=android-ndk-r21d
ANDROID_NDK="${PWD}/${NDK_FOLDER}"
if wget --no-clobber "https://dl.google.com/android/repository/${NDK_ZIP}" -O "${PWD}/${NDK_ZIP}"; then
  unzip -o -q "${NDK_ZIP}"
  chmod +x -R "${ANDROID_NDK}"
fi;

# CONFIGURE
export CC=gcc
export CXX=g++
mkdir -p build
cd build
cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID=1 \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
  -DANDROID_NDK="${ANDROID_NDK}" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NATIVE_API_LEVEL=21 \
  -DANDROID_TOOLCHAIN_NAME=arm64-llvm \
  -DANDROID_STL=c++_shared

# BUILD
ninja PeqBankCLI